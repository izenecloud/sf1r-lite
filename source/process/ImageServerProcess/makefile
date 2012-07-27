CFLAGS = -ggdb 
LIBS = `pkg-config --libs opencv`
INCLUDE = `pkg-config --cflags opencv`
TARGET = ColorProcessMain
OBJECTS = ColorProcessMain.o ImageProcess.o ImageProcessTask.o
cc = g++
PWD = `pwd`

$(TARGET):$(OBJECTS)
	$(cc) -o $(TARGET) $(OBJECTS) $(LIBS)
	#ln -s -f $(PWD)/$(TARGET) $(PWD)/../$(TARGET)

ImageProcess.o:ImageProcess.cc
	$(cc) $(CFLAGS) -c Log.h TimeEstimate.h MessageQueue.h Request.h ScopeLock.h ImageProcess.h ImageProcess.cc $(INCLUDE)

ImageProcessTask.o:ImageProcessTask.cc
	$(cc) $(CFLAGS) -c Log.h TimeEstimate.h MessageQueue.h Request.h ScopeLock.h ImageProcess.h ImageProcess.cc ImageProcessTask.cc ImageProcessTask.h $(INCLUDE)

ColorProcessMain.o:ColorProcessMain.cc  
	$(cc) $(CFLAGS) -c ColorProcessMain.cc Log.h TimeEstimate.h MessageQueue.h Request.h ScopeLock.h ImageProcess.h ImageProcess.cc $(INCLUDE)

all:$(TARGET)

clean:
	rm -rf $(TARGET) *.o *.gch
	rm -rf $(PWD)/../$(TARGET)

install:
	#ln -s $(PWD)/$(TARGET) $(PWD)/../$(TARGET)
