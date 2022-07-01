CXX = g++

DEBUG = 1

ifeq ($(DEBUG),1)
	CXXFLAGS += -g
else 
	CXXFLAGS += -O2
endif




server:  main.cpp config.cpp webserver.cpp log/log.cpp  mysql_conn/sql_connection_pool.cpp http_conn/http_conn.cpp timer/list_timer.cpp 
	$(CXX)   $^ $(CXXFLAGS) -lpthread -lmysqlclient -Ihttp_conn -o server 
clean:
	rm -r server 
