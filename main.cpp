#include<string>
#include"config.h"

int main(int argc , char** argv)
{

    // The info need to be modified: mysql account,passwd,databasename 
    std::string user = "godot";
    std::string passwd = "gloria";
    std::string databasename = "webserver";

    // parse command line args
    Config config;
    config.parse_arg(argc , argv);



    // init 


    return 0;
}

