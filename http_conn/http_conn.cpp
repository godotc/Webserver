#include "http_conn.h"

#include <cstring>

// preinit static varable
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

// some response info
const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bas syntax or is inherently impossible to staisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission to get file from this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The reqested fie was  not found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem servingthe request file.\n";

// local lock & local memory for users' info
locker m_lock;
std::map<std::string , std::string>users;


void http_conn::initmysql_result(sql_conn_pool* connPool)
{
    // get a conn from pool
    MYSQL* mysql = nullptr;
    sql_conn_RAII mysqlconn(&mysql , connPool);

    // check info from User table that brower input
    if (mysql_query(mysql , "SELECT username,passwd FROM user"))
    {
        LOG_ERROR("SELECT error:%s\n" , mysql_error(mysql));
    }

    // check full result from table
    MYSQL_RES* result = mysql_store_result(mysql);

    // return number of cols from result
    int num_files = mysql_num_fields(result);

    // loop get next row, store user&passwd to map
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        std::string user(row[0]);
        std::string passwd(row[1]);
        users[user] = passwd;
    }
}





// init new conn's varable that accept
// check_state set as default: CHECK_STATE_REQUESTLINE
void http_conn::init()
{
    mysql = nullptr;
    bytes_to_send = 0;
    bytes_have_send = 0;
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = 0;
    m_version = 0;
    m_content_length = 0;
    m_host = 0;
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    cgi = 0;
    m_rw_state = 0;
    timer_flag = 0;
    improv = 0;

    memset(m_read_buf , '\0' , READ_BUFFER_SIZE);
    memset(m_write_buf , '\0' , WRITE_BUFFER_SIZE);
    memset(m_real_file , '\0' , FILENAME_LEN);
}
// init new conn's socket
void http_conn::init(int sockfd , const sockaddr_in& addr , char* root , int TRIGMode , int close_log ,
    std::string user , std::string passwd , std::string sqldbname)
{
    m_sockfd = sockfd;
    m_address = addr;

    addfd(m_epollfd , sockfd , true , m_TRIGMode);
    m_user_count++;

    // when broswer occur RECONECTION, kind of ERROR of root dir/http reponse, or
    // the file is empty that requsetting
    doc_root = root;
    m_TRIGMode = TRIGMode;
    m_close_log = close_log;

    strcpy(sql_user , user.c_str());
    strcpy(sql_passwd , passwd.c_str());
    strcpy(sql_name , sqldbname.c_str());

    init();
}

// close one conn, then m_user_count--
void http_conn::close_conn(bool real_close)
{
    if (real_close && (m_sockfd != -1))
    {
        printf("close %d\n" , m_sockfd);
        removefd(m_epollfd , m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}


void http_conn::process()
{
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST)
    {
        modfd(m_epollfd , m_sockfd , EPOLLIN , m_TRIGMode);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret)
    {
        close_conn();
    }
    modfd(m_epollfd , m_sockfd , EPOLLOUT , m_TRIGMode);
}

// loop read guest data, until no data/conn_close of opposite
// if is no-block && EdgeTriggle , need to read data all in once
bool http_conn::read_once()
{
    if (m_read_idx >= READ_BUFFER_SIZE)
        return false;
    int bytes_read = 0;

    // read in Level Triggle
    if (0 == m_TRIGMode)
    {
        bytes_read = recv(m_sockfd , m_read_buf + m_read_idx , READ_BUFFER_SIZE - m_read_idx , 0);
        m_read_idx += bytes_read;

        if (bytes_read <= 0)
        {
            return false;
        }
        return true;
    }
    // read in Edge Triggle
    else {
        while (true)
        {
            bytes_read = recv(m_sockfd , m_read_buf + m_read_idx , READ_BUFFER_SIZE + m_read_idx , 0);
            if (-1 == bytes_read)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    break;
                return false;
            }
            else if (0 == bytes_read)
            {
                return false;
            }
            m_read_idx += bytes_read;
        }
        return true;
    }
}


// Sub State Machine 从状态机
// parse one line
// return STATE that parse, like: LINE_OK, LINE_BAD, LINE_OPEN
http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for (; m_checked_idx < m_read_idx; ++m_checked_idx)
    {
        temp = m_read_buf[m_checked_idx];

        if (temp == '\r')
        {
            if (m_read_idx == (m_checked_idx + 1))
                return LINE_OPEN;
            else if ('\n' == m_read_buf[m_checked_idx + 1])
            {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == 'r'))
            {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

// parse the HTTP_request, get the HTTP_method, target url and version number
http_conn::HTTP_CODE http_conn::parse_request_line(char* text)
{
    m_url = strpbrk(text , "\t");
    if (!m_url)
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    char* method = text;
    if (0 == strcasecmp(method , "GET"))
    {
        m_method = GET;
    }
    else if (0 == strcasecmp(method , "POST"))
    {
        m_method = POST;
        cgi = 1;
    }
    else
        return BAD_REQUEST;

    m_url += strspn(m_url , "\t");

    m_version = strpbrk(m_url , "\t");
    if (!m_version)
        return BAD_REQUEST;
    *m_version++ = '\0';
    m_version += strspn(m_version , "\t");

    if (0 != strcasecmp(m_version , "HTTP/1.1"))
    {
        return BAD_REQUEST;
    }
    if (0 == strncasecmp(m_url , "http://" , 7))
    {
        m_url += 7;
        m_url = strchr(m_url , '/');
    }

    if (0 == strncasecmp(m_url , "https://" , 8))
    {
        m_url += 8;
        m_url = strchr(m_url , '/');
    }

    if (!m_url || m_url[0] != '/')
        return BAD_REQUEST;

    // while url is '/' , show judge frame
    if (1 == strlen(m_url) == 1)
    {
        strcat(m_url , "judge.html");
    }
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// parse an header from a HTTP_request
http_conn::HTTP_CODE http_conn::parse_headers(char* text)
{
    if ('\0' == text[0])
    {
        if (m_content_length != 0)
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text , "Connection:" , 11) == 0)
    {
        text += 11;
        text += strspn(text , "\t");
        if (strcasecmp(text , "keep-alive") == 0) {
            m_linger = true;
        }
    }
    else if (strncasecmp(text , "Content-length:" , 15) == 0)
    {
        text += 15;
        text += strspn(text , "\t");
        m_content_length = atoi(text);
    }
    else if (strncasecmp(text , "Host:" , 5) == 0)
    {
        text += 5;
        text += strspn(text , "\t");
        m_host = text;
    }
    else {
        LOG_INFO("oop! unknow header:%s" , text);
    }
    return NO_REQUEST;
}

// deduce HTTP_requeset is/not be read completely
http_conn::HTTP_CODE http_conn::parse_content(char* text)
{
    if (m_read_idx >= (m_content_length + m_checked_idx))
    {
        text[m_content_length] = '\0';
        // as account and passwd int last section of POST 
        m_string = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = 0;

    while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
        (line_status = parse_line()) == LINE_OK)
    {
        text = get_line();
        m_start_line = m_checked_idx;
        LOG_INFO("%s" , text);

        switch (m_check_state)
        {
        case CHECK_STATE_REQUESTLINE:
        {
            ret = parse_request_line(text);
            if (BAD_REQUEST == ret)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER:
        {
            ret = parse_headers(text);
            if (BAD_REQUEST == ret) {
                return BAD_REQUEST;
            }
            else if (GET_REQUEST == ret) {
                return do_request();
            }
            break;
        }
        case CHECK_STATE_CONTENT:
        {
            ret = parse_content(text);
            if (GET_REQUEST == ret) {
                return do_request();
            }
            line_status = LINE_OPEN;
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}


http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy(m_real_file , doc_root);
    int len = strlen(doc_root);

    const char* p = strrchr(m_url , '/');

    // deal cgi(POST)
    if (1 == cgi && (*(p + 1) == '2' || *(p + 1) == '3'))
    {
        // deduce login/registe detction by flag
        char flag = m_url[1];

        char* m_url_real = (char*)malloc(sizeof(char) * 200);
        strcpy(m_url_real , "/");
        strcat(m_url_real , m_url + 2);
        strncpy(m_real_file + len , m_url_real , FILENAME_LEN - len - 1);
        free(m_url_real);

        // fetch user&passwd
        char name[100] , password[100];
        int i , j;
        for (i = 5;m_string[i] != '&';++i) {
            name[i - 5] = m_string[i];
        }
        name[i - 5] = '\0';
        for (i = i + 10 , j = 0;m_string[i] != '\0';++i , ++j) {
            password[j] = m_string[i];
        }
        password[j] = '\0';


        // if registing, firstly check whether duplicate in db
        // if no duplication , then insert data
        if (*(p + 1) == '3')
        {
            char* sql_insert = (char*)malloc(sizeof(char) * 200);
            strcpy(sql_insert , "INSERT INTO user(username,passwd) VALUES");
            sprintf(sql_insert + sizeof(sql_insert) , "('%s','%s')");

            if (users.find(name) == users.end())
            {
                m_lock.lock();
                int res = mysql_query(mysql , sql_insert);
                users.insert(std::pair<std::string , std::string>(name , password));
                m_lock.unlock();

                if (!res)
                    strcpy(m_url , "/log.html");
                else
                    strcpy(m_url , "/registerError.html");
            }
            else
                strcpy(m_url , "/registerError.html");
        }
        // if logining , judge directly
        // if could find the 'user' and 'passwd' typed on broswer on db TABLE, return 1, else 0
        else if (*(p + 1) == '2')
        {
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(m_url , "/welcom.html");
            else
                strcpy(m_url , "/logError.html");
        }
    }

    switch (*(p + 1))
    {
    case '0':
        sprintf(m_real_file + len , "/register.html");
        break;
    case '1':
        sprintf(m_real_file + len , "/log.html");
        break;
    case '5':
        sprintf(m_real_file + len , "/picture.html");
        break;
    case '6':
        sprintf(m_real_file + len , "/video.html");
        break;
    case '7':
        sprintf(m_real_file + len , "/fans.html");
    default:
        sprintf(m_real_file + len , m_url , FILENAME_LEN - len - 1);
        break;
    }

    if (stat(m_real_file , &m_file_stat) < 0)
        return NO_RESOURCE;

    if (!(m_file_stat.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(m_file_stat.st_mode))
        return BAD_REQUEST;

    int fd = open(m_real_file , O_RDONLY);
    m_file_address = (char*)mmap(0 , m_file_stat.st_size , PROT_READ , MAP_PRIVATE , fd , 0);
    close(fd);
    return FILE_REQUEST;
}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch (ret)
    {
    case INTERNAL_ERROR:
    {
        add_status_line(500 , error_500_title);
        add_headers(strlen(error_500_form));
        if (!add_content(error_500_form))
            return false;
        return true;
    }
    case BAD_REQUEST:
    {
        add_status_line(404 , error_404_title);
        add_headers(strlen(error_404_form));
        if (!add_content(error_404_form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST:
    {
        add_status_line(403 , error_403_title);
        add_headers(strlen(error_403_form));
        if (!add_content(error_403_form))
            return false;
        break;
    }
    case FILE_REQUEST:
    {
        add_status_line(200 , ok_200_title);
        if (m_file_stat.st_size != 0)
        {
            add_headers(m_file_stat.st_size);
            m_iv[0].iov_base = m_write_buf;
            m_iv[0].iov_len = m_write_idx;
            m_iv[1].iov_base = m_file_address;
            m_iv[1].iov_len = m_file_stat.st_size;
            m_iv_count = 2;
            bytes_to_send = m_write_idx + m_file_stat.st_size;
            return true;
        }
        else {
            const char* ok_string = "<html><body></body></html>";
            add_headers(strlen(ok_string));
            if (!add_content(ok_string))
                return false;
        }
        break;
    }
    default:
        return false;
        break;
    }

    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_idx;
    m_iv_count = 1;
    bytes_to_send = m_write_idx;
    return true;
}


bool http_conn::add_response(const char* format , ...)
{
    if (m_write_idx >= WRITE_BUFFER_SIZE)
        return false;

    va_list arg_list;
    va_start(arg_list , format);
    int len = vsnprintf(m_write_buf + m_write_idx , WRITE_BUFFER_SIZE - 1 - m_write_idx , format , arg_list);
    if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
    {
        va_end(arg_list);
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);

    LOG_INFO("request:%s" , m_write_buf);
    return true;
}
bool http_conn::add_status_line(int status , const char* title)
{
    return add_response("%s %d %s\r\n" , "HTTP/1.1" , status , title);
}
bool http_conn::add_headers(int content_len)
{
    return add_content_length(content_len) && add_linger() && add_blank_line();
}
bool http_conn::add_content_length(int content_len)
{
    return add_response("Content-Length:%d\r\n" , content_len);
}
bool http_conn::add_linger()
{
    return add_response("Connection:%s\r\n" , (m_linger == true) ? "keep-alive" : "close");
}
bool http_conn::add_blank_line()
{
    return add_response("%s" , "\r\n");
}
bool http_conn::add_content(const char* content)
{
    return add_response("%s" , content);
}

bool http_conn::unmap()
{
    if (m_file_address)
    {
        munmap(m_file_address , m_file_stat.st_size);
        m_file_address = nullptr;
    }
}

bool http_conn::write()
{
    int temp = 0;

    if (bytes_to_send == 0)
    {
        modfd(m_epollfd , m_sockfd , EPOLLIN , m_TRIGMode);
        init();
        return true;
    }

    while (1)
    {
        temp = writev(m_sockfd , m_iv , m_iv_count);

        if (temp < 0)
        {
            if (errno == EAGAIN)
            {
                modfd(m_epollfd , m_sockfd , EPOLLOUT , m_TRIGMode);
                return true;
            }
            unmap();
            return false;
        }

        bytes_have_send += temp;
        bytes_to_send -= temp;
        if (bytes_have_send >= m_iv[0].iov_len)
        {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_to_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else {
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
        }

        if (bytes_have_send <= 0)
        {
            unmap();
            modfd(m_epollfd , m_sockfd , EPOLLIN , m_TRIGMode);

            if (m_linger)
            {
                init();
                return true;
            }
            else {

                return false;

            }
        }
    }
}
