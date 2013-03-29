CC:=g++
CXXFLAGS:= -g -O2 -DNDEBUG -I$(IZENELIB)/include 
LDFLAGS:=-L$(IZENELIB)/lib -L/usr/lib64/mysql -lsf1r -lboost_system -lboost_thread -lglog
all:
	$(CC) -o sf1r_process.fcgi sf1r_process.cpp $(CXXFLAGS) `pkg-config --libs --cflags fastcgi++` $(LDFLAGS)
