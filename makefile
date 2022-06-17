CXX ?= g++

DEBUG = 1

ifeq ($(DEBUG),1)
	CXXFLAGS += -g
else 
	CXXFLAGS += -O2
endif

server: main.cpp  config.cpp log/log.cpp  mysql/sql_connection_pool.cpp
	$(CXX) -o server $^ $(CXXFLAGS)

clean:
	rm -r server 
