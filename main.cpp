#include "config.h"
#include "webserver.h"
#include <string>

int
main (int argc, char **argv)
{
    // The info need to be modified: mysql account,passwd,databasename
    std::string user = "godot";
    std::string passwd = "gloria";
    std::string databasename = "webserver";

    // parse command line args
    Config config;
    config.parse_arg (argc, argv);

    Webserver server;

    // init
    server.init (config.PORT, user, passwd, databasename, config.LOGWrite,
                 config.OPT_LINGER, config.TRIGMode, config.sql_num,
                 config.thread_num, config.close_log, config.actor_model);

    // Log
    server.log_write ();

    // DB
    server.sql_pool ();

    // thread_pool()
    server.thread_pool ();

    // trigger mode
    server.trig_mode ();

    // listen
    server.eventListen ();

    // run
    server.eventLoop ();

    return 0;
}
