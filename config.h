#ifndef CONFIG_H
#define CONFIG_H


class Config
{
public:
    Config ();
    ~Config (){};

    void parse_arg (int argc, char** argv);

private:

    int PORT;

    // log write mode
    int LOGWrite;

    // trigger combinenation mode
    int TRIGMode;

    // listen fd trigger mode
    int LISTENTrigMode;

    // connection fd trigger mode
    int CONNTrigMode;

    // 优雅关闭链接
    int OPT_LINGER;

    // number of sql connection poll   
    int sql_num;

    // number of thread in pool
    int thread_num;

    // close/open log
    int close_log;

    // choose the model of concurrent
    int actor_model;
};



#endif
