CXXFLAGS =	-O2 -g -Wall -fmessage-length=0

OBJS =		wcserver.o

LIBS =

TARGET =	wcftpd

$(TARGET):	$(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
