#makefile#

LIB_DIRS = 
CC=g++

CDEFS=
CFLAGS= -O0 -pg -g $(INCLUDE_DIRS) $(CDEFS)
LIBS= -lrt
CPPLIBS= -L/usr/lib -lopencv_core -lopencv_flann -lopencv_video -lpthread

HFILES= 
CFILES= 
CPPFILES= server.cpp

SRCS= ${HFILES} ${CFILES}
CPPOBJS= ${CPPFILES:.cpp=.o}

all:	server 

clean:
	-rm -f *.o *.d *.ppm
	-rm -f server

distclean:
	-rm -f *.o *.d

server: server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $@.o `pkg-config --libs opencv` $(CPPLIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

.cpp.o:
	$(CC) $(CFLAGS) -c $<
