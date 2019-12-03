#ifndef __X264ENCODER_H_INCLUDED__
#define __X264ENCODER_H_INCLUDED__

#include <iostream>
#include <queue>
#include <stdint.h>
#include <BasicUsageEnvironment.hh>

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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

#define MAX_NAL_COUNT 4

class x264Encoder
{
	public:
		x264Encoder(UsageEnvironment & env);
		~x264Encoder(void);
		bool initilize();
		void unInitilize();
		void encodeFrame();
		bool open_device();
		bool init_device();
		bool init_mmap();
		bool start_capturing();
		void mainloop();
		void stop_capturing();
		void uninit_device();
		void close_device();
		void yuyv(const void * p, int size);
		void process_image(const void * p, int size);
		void x264_print_err(const char *s);
		int x264_ioctl(int fh, unsigned long int request, void *arg);
		int read_frame(void);
		bool isNalsAvailableInOutputQueue();
		x264_nal_t * getNalUnit();

	private:
		UsageEnvironment & usage_env;
		int video_dev_desc;
		x264_nal_t * nal_references[MAX_NAL_COUNT];
		std::queue<x264_nal_t *> outputQueue;
		x264_param_t param;
		x264_picture_t pic;
		x264_picture_t pic_out;
		x264_t *h;
		size_t mmap_buffer_cnt;
}; 

#endif
