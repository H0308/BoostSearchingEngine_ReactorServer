CC=g++
CFLAGS=-std=c++17 -O3 -DNDEBUG -march=native -flto=auto
# CFLAGS=-std=c++17
# INCLUDES=-I项目目录
# 例如：INCLUDES=-I/home/epsda/BoostSearchingEngine_ReactorServer/
# LDFLAGS=-lpthread -lfmt -lspdlog -lboost_system -fsanitize=address -g -ljsoncpp
LDFLAGS=-lpthread -lfmt -lspdlog -lboost_system -flto=auto -ljsoncpp

all: parse server

parse: parse.cc
	$(CC) -o parse parse.cc $(CFLAGS) $(INCLUDES) $(LDFLAGS)

server: server.cc
	$(CC) -o server server.cc $(CFLAGS) $(INCLUDES) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f server parse