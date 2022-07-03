/**
 * @file main.cpp
 * @author godot (disyourself@qq.com)
 * @brief Webserver project on C++14 standard
 * @version 0.1
 * @date 2022-07-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include "server/webserver.h"

int
main ()
{
    WebServer server (9527, 3, 60000, false,
                      3306, "godot", "gloria", "webserver",
                      12, 6,
                      true, 1, 1024);


    // server.init ();

    return 0;
}