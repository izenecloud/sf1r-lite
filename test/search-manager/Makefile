##################################################
# include c-compiler environment makefile
##################################################
CXX = g++-4.1
CXXFLAGS = -g -Wall -c -Wno-deprecated 

##################################################
# definition for target
##################################################

INC_PATH = -I/usr/local/include/boost-1_39 \
           -I../ \
           -I$(WISELIB)/include \
           -I$(IZENELIB)/include \
           -I$(LAMANAGER)/include \
           -I../../source/index-process/index-manager/include \
           -I../../source/core/include/ \

LIB_PATH = -L/usr/local/lib \
           -L../ \
           -L$(WISELIB)/lib \
           -L$(IZENELIB)/lib \
           -L$(LA_PATH)/lib \
           -L../../lib \

LIBS =     	$(ILPLIB)/lib/liblangid.a \
		-lmock_index_manager \
		-lindex_manager \
		-lboost_date_time-gcc41-mt-1_39 \
		-lboost_serialization-gcc41-mt-1_39 \
		-lboost_system-gcc41-mt-1_39 \
		-lboost_program_options-gcc41-mt-1_39 \
		-lboost_thread-gcc41-mt-1_39 \
		-lboost_unit_test_framework-gcc41-mt-1_39 \
		-lprocmeminfo \
		-lwiselib \
		-lfebird \
		-lbz2 \
		-lz \
		-ludt3 \
		-lizene_util \
		-lglog \

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp))

%.o : %.cpp
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c -o $@ $? 

%.o : %.cc
	$(CXX) $(INC_PATH) $(CXXFLAGS) -c -o $@ $? 

TARGET = test

all: $(TARGET) 

$(TARGET) : $(OBJECTS) 
	$(CXX) -o $@ $^ $(LIB_PATH) $(LIBS)

clean:
	rm -f *.o  $(TARGET) \

