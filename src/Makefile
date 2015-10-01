CC=g++
CFLAGS += -O3 -g 
CXXFLAGS += -std=c++11 -O3 -fopenmp -g
LDFLAGS += -static
LDLIBS += -lmagic -lserved -lre2 -ljson11 -lboost_timer -lboost_chrono -lboost_program_options -lboost_log -lboost_log_setup -lboost_thread -lboost_filesystem -lboost_system -lpthread -lz -lrt -ldl

PROGS = server

all:	$(PROGS)

server:	server.o stags.o logging.o
