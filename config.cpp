#include "config.h"
#include "stdlib.h"
#include "unistd.h"

Config::Config ()
{
    // Default: 9527
    PORT = 9527;

    // Default: 0--synchronization
    LOGWrite = 0;

    // Listen fd triggermode, default: 0--Level Trigger
    LISTENTrigMode = 0;

    // Connecting fd trigger mode, default: 0--LT
    CONNTrigMode = 0;

    // 优雅关闭链接，默认不适用 0
    OPT_LINGER = 0;

    // number of sql connection poll
    sql_num = 8;

    // number of thread in pool
    thread_num = 8;

    // Close/Open log, default: 0--not close
    close_log = 0;

    // Choose the model of concurrent, default: 0--proactor
    actor_model = 0;
}

void
Config::parse_arg (int argc, char **argv)
{
    int opt;
    const char *str = "p:l:m:o:s:t:c:a:";

    while ((opt = getopt (argc, argv, str)) != -1)
        {
            switch (opt)
                {
                case 'p':
                    {
                        PORT = atoi (optarg);
                        break;
                    }
                case 'l':
                    {
                        LOGWrite = atoi (optarg);
                        break;
                    }
                case 'm':
                    {
                        TRIGMode = atoi (optarg);
                    }
                case 'o':
                    {
                        OPT_LINGER = atoi (optarg);
                    }
                case 's':
                    {
                        sql_num = atoi (optarg);
                        break;
                    }
                case 't':
                    {
                        thread_num = atoi (optarg);
                        break;
                    }
                case 'c':
                    {
                        close_log = atoi (optarg);
                        break;
                    }
                case 'a':
                    {
                        actor_model = atoi (optarg);
                        break;
                    }
                default:
                    break;
                }
        }
}
