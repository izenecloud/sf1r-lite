CC:=g++
CXXFLAGS:= -g -O2 -DNDEBUG -I$(TBLIB_ROOT)/include -I$(TBLIB_ROOT)/include/tbsys \
	 -I$(TBLIB_ROOT)/include/tbnet -I$(HOME)/tfs_bin/include
LDFLAGS:=-L$(TBLIB_ROOT)/lib -L$(HOME)/tfs_bin/lib \
	-ltbnet -ltbsys -ltfsclient  
all:
	$(CC) -o image_process.fcgi image_process.cpp $(CXXFLAGS) `pkg-config --libs --cflags fastcgi++` `GraphicsMagick++-config --cppflags --cxxflags --ldflags --libs` $(LDFLAGS)
