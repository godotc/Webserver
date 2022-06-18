#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include "../mysql/sql_connection_pool.h"
#include "encapsulated_epoll.h"
#include <arpa/inet.h>
#include <map>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include<sys/uio.h>

class http_conn
{
public:
    http_conn() {}
    ~http_conn() {}

public:
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 2048;

    enum METHOD
    {
        GET = 0 ,
        POST ,
        HEAD ,
        PUT ,
        DELETE ,
        TRACE ,
        OPTIONS ,
        CONNECTI ,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0 , // checking request line
        CHECK_STATE_HEADER ,          // checking header lines
        CHECK_STATE_CONTENT          // checking content
    };
    enum HTTP_CODE
    {
        NO_REQUEST ,
        GET_REQUEST ,
        BAD_REQUEST ,
        NO_RESOURCE ,
        FORBIDDEN_REQUEST ,
        FILE_REQUEST ,
        INTERNAL_ERROR ,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0 ,
        LINE_BAD ,
        LINE_OPEN
    };

public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL* mysql;
    int m_rw_state; // read-0, write-1

private:
    int m_sockfd;
    sockaddr_in m_address;

    char m_read_buf[READ_BUFFER_SIZE];
    int m_read_idx;
    int m_checked_idx;
    int m_start_line;
    CHECK_STATE m_check_state;
    METHOD m_method;

    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_write_idx;

    char m_real_file[FILENAME_LEN];
    char* doc_root;
    char* m_url;
    char* m_version;
    char* m_host;
    int m_content_length;
    bool m_linger;  // keep-alive property?
    int cgi;        // 是否启用的POST
    char* m_string; //存储请求头数据

    char* m_file_address;
    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;

    int bytes_to_send;
    int bytes_have_send;

    std::map<std::string , std::string> m_users;
    char sql_user[100];
    char sql_passwd[100];
    char sql_name[100];

    int m_TRIGMode;
    int m_close_log;

public:
    int timer_flag;
    int improv;
    void init(int sockfd , const sockaddr_in& addr , char* , int , int , std::string user , std::string passwd , std::string sqlname);
    void close_conn(bool real_close = true);
    void process();
    bool read_once();
    bool write();
    sockaddr_in* get_address()
    {
        return &m_address;
    }

private:
    void init();
    HTTP_CODE process_read();
    LINE_STATUS parse_line();
    char* get_line() { return m_read_buf + m_start_line; }
    HTTP_CODE parse_request_line(char* text);
    HTTP_CODE parse_headers(char* text);
    HTTP_CODE parse_content(char* text);

    HTTP_CODE do_request();
    bool process_write(HTTP_CODE ret);
    bool add_response(const char* format , ...);
    bool add_status_line(int status , const char* title);
    bool add_headers(int content_len);
    bool add_content_length(int content_len);
    bool add_linger();
    bool add_blank_line();
    bool add_content(const char* content);

    bool unmap();
};
#endif
