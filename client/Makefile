CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		wcclient.o

LIBS =

TARGET =	wcftp

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
