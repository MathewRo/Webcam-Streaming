#ifndef __X264ENCODER_H_INCLUDED__
#define __X264ENCODER_H_INCLUDED__

#include <iostream>
#include <queue>
#include <stdint.h>
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>         
#include <fcntl.h>              
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <stdint.h>
#include <math.h>
#include <x264.h>
}

class x264Encoder
{
	public:
		x264Encoder(void);
		~x264Encoder(void);

	public:
		void initilize();
		void unInitilize();
		void encodeFrame();
		void open_device();
		void init_device();
		void init_mmap();
		void start_capturing();
		void mainloop();
		void stop_capturing();
		void uninit_device();
		void close_device();
		void yuyv(const void * p, int size);
		void process_image(const void * p, int size);
		void errno_exit(const char *s);
		int xioctl(int fh, int request, void *arg);
		int read_frame(void);
		bool isNalsAvailableInOutputQueue();
		x264_nal_t * getNalUnit();
	public:
		std::queue<x264_nal_t *> outputQueue;
		x264_param_t param;
		x264_picture_t pic;
		x264_picture_t pic_out;
		x264_t *h;
}; 

#endif
