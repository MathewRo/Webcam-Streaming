INCLUDES = -I/usr/local/include -I/usr/local/include/liveMedia -I/usr/local/include/BasicUsageEnvironment -I/usr/local/include/UsageEnvironment -I/usr/local/include/groupsock 
OBJS = webcam.o H264LiveServerMediaSession.o LiveSourceWithx264.o x264Encoder.o
CC = g++ $(INCLUDES) 
DEBUG = -g
CFLAGS =  -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)
LDLIB = -L/usr/local/lib
LDFLAGS = $(LDLIB) -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock -lx264 -lpthread
webcam : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o webcam $(LDFLAGS)
IDIR = includes/

webcam.o : webcam.cpp $(IDIR)x264Encoder.h $(IDIR)LiveSourceWithx264.h $(IDIR)H264LiveServerMediaSession.h 
	$(CC) $(CFLAGS) webcam.cpp

H264LiveServerMediaSession.o : H264LiveServerMediaSession.cpp $(IDIR)LiveSourceWithx264.h 
	$(CC) $(CFLAGS) H264LiveServerMediaSession.cpp

LiveSourceWithx264.o : LiveSourceWithx264.cpp $(IDIR)LiveSourceWithx264.h $(IDIR)x264Encoder.h
	$(CC) $(CFLAGS) LiveSourceWithx264.cpp

x264Encoder.o : x264Encoder.cpp $(IDIR)x264Encoder.h
	$(CC) $(CFLAGS) x264Encoder.cpp

clean:
	\rm *.o webcam
