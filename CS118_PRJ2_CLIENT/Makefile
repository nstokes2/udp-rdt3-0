CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		CS118_PRJ2_CLIENT.o

LIBS =

TARGET =	wcftp

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
