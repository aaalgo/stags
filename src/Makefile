CC=g++
CFLAGS += -O3 -g 
CXXFLAGS += -std=c++11 -O3 -fopenmp -g
LDFLAGS += -static -fopenmp
LDLIBS += -lxgboost /usr/local/lib/dmlc_simple.o /usr/local/lib/caffex.o -lrabit -Wl,--whole-archive -lcaffe -Wl,--no-whole-archive -lproto -lprotobuf -lsnappy -lgflags -lglog -lunwind -lleveldb -llmdb -lhdf5_hl -lhdf5 -lopencv_objdetect -lopencv_features2d -lopencv_imgproc -lopencv_imgcodecs -lopencv_flann -lopencv_core -lopencv_hal -lIlmImf -lippicv -lmitie -lmagic -lserved -lre2 -ljson11 -lboost_timer -lboost_chrono -lboost_program_options -lboost_log -lboost_log_setup -lboost_thread -lboost_filesystem -lboost_system -lopenblas -ljpeg -ltiff -lpng -ljasper -lwebp -lpthread -lz -lm -lrt -ldl

PROGS = server

all:	$(PROGS)

server:	server.o stags.o logging.o image-face.o

clean:
	rm $(PROGS) *.o
