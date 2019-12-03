#include <x264Encoder.h>
// Macro for clearing
#define CLEAR(x) memset(&(x), 0, sizeof(x))

// defines required for V4L2 and x264
#define VIDEO_WIDTH 		640
#define VIDEO_HEIGHT 		480
#define NAL_PAYLOAD_SIZE 	60000
#define INVALID_FILE_DESC	-1
#define XIOCTL_FAIL			-1

// buffer for getting data out of v4l2
struct buffer {
	void   *start;
	size_t  length;
};

// Name of video device. Have to see if we can
// avoid hardcoding it. Want to avoid calling system
// api (system())
const char *dev_name = "/dev/video0";
struct buffer *buffers;
long row = 		VIDEO_WIDTH*VIDEO_HEIGHT/2;

void yuyv(const void * , int);

x264Encoder::x264Encoder(UsageEnvironment & env): usage_env(env), h(NULL)
{
	int i = 0;
	for (; i < MAX_NAL_COUNT; i++) {
		nal_references[i] = NULL;
	}
	video_dev_desc = INVALID_FILE_DESC;
	mmap_buffer_cnt = 0;
}

x264Encoder::~x264Encoder(void)
{

}

/**
 * Cleanup all the memory allocated with
 * nal buffers
 *
 * @in  - void
 * @out	- void
 *
 **/
void x264Encoder::unInitilize()
{
	int i = 0;
	for (; i < MAX_NAL_COUNT; i++) {
		if (nal_references[i]) {
			if (nal_references[i]->p_payload) {
			free(nal_references[i]->p_payload);
			}
		free(nal_references[i]);
		}
	}
}

void x264Encoder :: encodeFrame()
{

}

bool x264Encoder :: isNalsAvailableInOutputQueue()
{
	if(outputQueue.empty() == true)
	{
		return false;
	}
	else
	{
		return true;
	}
}

x264_nal_t * x264Encoder :: getNalUnit()
{

	x264_nal_t * nal;
	nal = outputQueue.front();
	outputQueue.pop();
	return nal;
}

/**
 * This routine would check for the video dev file on
 * Linux and opens the same if found
 *
 * @in 	- None
 * @out - true  - if file opened
 * 		- false - for failure
 * */
bool x264Encoder :: open_device()
{
	struct stat st;

	/* get file stats filled */
	if (-1 == stat(dev_name, &st)) {
		usage_env << "Cannot identify '%s': %d, %s\n" <<
				dev_name << errno << strerror(errno);
		return false;
	}

	/* check if its a valid char device */
	if (!S_ISCHR(st.st_mode)) {
		usage_env << "%s is no device\n" << dev_name;
		return false;
	}

	/* open the video dev file */
	video_dev_desc = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (INVALID_FILE_DESC == video_dev_desc) {
		usage_env << "Cannot open '%s': %d, %s\n" <<
				dev_name << errno << strerror(errno);
		return false;
	}

	return true;
}

/**
 * This method would query the video capture device,
 * based on the capabilities, set the cropping, format
 * and resolution.
 * */
bool x264Encoder :: init_device()
{
	struct v4l2_capability v4_capability;
	struct v4l2_cropcap v4_cropcap;
	struct v4l2_crop v4_crop;
	struct v4l2_format v4_format;


	/* query the device capabilities */
	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_QUERYCAP, &v4_capability)) {
		if (EINVAL == errno) {
			usage_env << "%s is no V4L2 device\n" << dev_name;
		} else {
			x264_print_err("VIDIOC_QUERYCAP");
		}
		return false;
	}

	/* check if device can capture video */
	if (!(v4_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		usage_env << "%s is not a video capture device\n" << dev_name;
		return false;
	}

	/* check if the device is a stream i/o */
	if (!(v4_capability.capabilities & V4L2_CAP_STREAMING)) {
		usage_env << "%s does not support streaming i/o\n" << dev_name;
		return false;
	}

	CLEAR(v4_cropcap);

	v4_cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* get details on the video crop capability */
	if (0 == x264_ioctl(video_dev_desc, VIDIOC_CROPCAP, &v4_cropcap)) {
		v4_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		v4_crop.c = v4_cropcap.defrect; /* reset to default */

		if (-1 == x264_ioctl(video_dev_desc, VIDIOC_S_CROP, &v4_crop)) {
			/* ignore error */
			usage_env << "cropping operation failed on device."
					"continuing with default \n";
		}
	} else {
		/* Errors ignored. */
			usage_env << "crop capabilities failed. Can't crop video\n";
	}


	CLEAR(v4_format);

	v4_format.type 				  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4_format.fmt.pix.width       = VIDEO_WIDTH;
	v4_format.fmt.pix.height      = VIDEO_HEIGHT;
	// TODO check if we can set other pixel formats and get this working
	v4_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	v4_format.fmt.pix.field       = V4L2_FIELD_NONE;

	/* send the format data */
	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_S_FMT, &v4_format)) {
		x264_print_err("VIDIOC_S_FMT ioctl failed");
		return false;
	}

	return true;
}

/**
 * This method sets up memory mapping for the
 * video devices file descriptor
 * */
bool x264Encoder :: init_mmap()
{
	struct v4l2_requestbuffers v4_request_buffers;

	CLEAR(v4_request_buffers);

	v4_request_buffers.count  = 4;
	v4_request_buffers.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4_request_buffers.memory = V4L2_MEMORY_MMAP;

	/* request buffers in V4L2 space which would hold the data */
	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_REQBUFS, &v4_request_buffers)) {
		if (EINVAL == errno) {
			usage_env << "%s does not support memory mapping\n" << dev_name;
		} else {
			x264_print_err("VIDIOC_REQBUFS");
		}
		return false;
	}

	if (v4_request_buffers.count < 2) {
		usage_env << "Insufficient buffer memory on %s\n" <<
				dev_name;
		return false;
	}

	mmap_buffer_cnt = v4_request_buffers.count;
	/* allocate user space buffer object*/
	buffers = (buffer *)calloc(mmap_buffer_cnt, sizeof(*buffers));

	if (!buffers) {
		usage_env << "Out of memory\n";
		return false;
	}

	for (int i = 0; i < mmap_buffer_cnt; i++) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		/* Query the buffer descriptions */
		if (-1 == x264_ioctl(video_dev_desc, VIDIOC_QUERYBUF, &buf)) {
			x264_print_err("VIDIOC_QUERYBUF failed");
			return false;
		}

		buffers[i].length = buf.length;
		buffers[i].start =
			mmap(NULL /* start anywhere */,
					buf.length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */,
					video_dev_desc,
					buf.m.offset);

		if (MAP_FAILED == buffers[i].start) {
			x264_print_err("mmap failed");
			return false;
		}
	}
	return true;
}

bool x264Encoder:: start_capturing()
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < mmap_buffer_cnt; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;
		/* Queue data into the mmap'd buf*/
		if (-1 == x264_ioctl(video_dev_desc, VIDIOC_QBUF, &buf)) {
			x264_print_err("VIDIOC_QBUF failed");
			return false;
		}
	}
	/* start streaming IO */
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_STREAMON, &type)) {
		x264_print_err("VIDIOC_STREAMON failed");
		return false;
	}

	return true;

}
void x264Encoder:: mainloop()
{
	unsigned int temp_count;
	unsigned int frame_count = 1;
	temp_count = frame_count;
	while (temp_count-- > 0) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int ret;

			FD_ZERO(&fds);
			FD_SET(video_dev_desc, &fds);
			/* Timeout. */
			tv.tv_sec = 10;
			tv.tv_usec = 0;

			ret = select(video_dev_desc + 1, &fds, NULL, NULL, &tv);

			if (-1 == ret) {
				if (EINTR == errno)
					continue;
				x264_print_err("select [h264]");
				return;
			}

			if (0 == ret) {
				usage_env << "v4l2 select timeout\n";
				return;
			}

			if (read_frame())
				break;
			/* EAGAIN - continue select loop. */
		}
	}
}

/**
 *
 * */
void x264Encoder:: stop_capturing()
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_STREAMOFF, &type)) {
		x264_print_err("VIDIOC_STREAMOFF");
		return;
	}
}

/**
 *
 * */
void x264Encoder:: uninit_device()
{
	unsigned int i;

	for (i = 0; i < mmap_buffer_cnt; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length)) {
			x264_print_err("munmap");
			return;
		}
	free(buffers);
}

/**
 * Close the video dev file opened for capturing
 * video data
 *
 * @in  - void
 * @out - void
 * */
void x264Encoder:: close_device()
{
	if (-1 == close(video_dev_desc)) {
		x264_print_err("close failed on video_dev file");
		return;
	}

	video_dev_desc = INVALID_FILE_DESC;
}

/**
 * */
void x264Encoder:: yuyv(const void * p, int size)
{
	int i,j;
	static int a=0;
	a++;
	x264_param_t param;
	x264_picture_t pic;
	x264_picture_t pic_out;
	x264_t *h;
	int i_frame = 0;
	int i_frame_size = -1;
	x264_nal_t *nal;
	int i_nal;
	if( x264_param_default_preset( &param, "medium", NULL ) < 0 )
		return;


	param.i_csp = X264_CSP_I420;
	param.i_width  = VIDEO_WIDTH;
	param.i_height = VIDEO_HEIGHT;
	param.b_vfr_input = 1;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.i_fps_num = 24;
	param.i_fps_den = 1;
	param.i_log_level = X264_LOG_NONE;

	if( x264_param_apply_profile( &param, "baseline" ) < 0 )
		return;

	if( x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height ) < 0 )
		return;

	h = x264_encoder_open( &param );
	if( !h )
		return;


	typedef struct
	{
		uint8_t YUYVSource[VIDEO_WIDTH * VIDEO_HEIGHT * 2];
	}__attribute__((__packed__)) frame_t;
	frame_t *frame3;

	frame3 =(frame_t *)p;

	for (i = 0; i != VIDEO_WIDTH * VIDEO_HEIGHT ; ++i)
	{	
		pic.img.plane[0][i] = frame3->YUYVSource[2*i];
	}
	for (j = 0; j != VIDEO_WIDTH * VIDEO_HEIGHT / 4 ; j++)
	{          	

		{
			pic.img.plane[1][j]     =  (frame3->YUYVSource[4*j + 1] + frame3->YUYVSource[row + 4*j + 1]) /2;

			pic.img.plane[2][j]     =  (frame3->YUYVSource[4*j + 3] + frame3->YUYVSource[row + 4*j + 3]) /2;
		}
	}
	pic.i_pts = i_frame;
	i_frame_size = x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out );
	if(i_frame_size > 0)
	{
		int i = 0;
		for (; i < MAX_NAL_COUNT; i++) {
			memcpy(nal_references[i]->p_payload, nal[i].p_payload, nal[i].i_payload);
			nal_references[i]->i_payload = nal[i].i_payload;
			outputQueue.push(nal_references[i]);
		}
	}

	while( x264_encoder_delayed_frames( h ) )
	{
		i_frame_size = x264_encoder_encode( h, &nal, &i_nal, NULL, &pic_out );

		if(i_frame_size > 0)
		{
			int i = 0;
			for (; i < MAX_NAL_COUNT; i++) {
				memcpy(nal_references[i]->p_payload, nal[i].p_payload, nal[i].i_payload);
				nal_references[i]->i_payload = nal[i].i_payload;
				outputQueue.push(nal_references[i]);
			}

		}

	}

	x264_encoder_close( h );
	x264_picture_clean( &pic );

}

/**
 * */
void x264Encoder:: process_image(const void * p, int size)
{
	yuyv(p,size);
}

/**
 * */
void x264Encoder:: x264_print_err(const char *s)
{
	usage_env << s <<" "<< errno <<"\n"<< strerror(errno);
}

/**
 * */
int x264Encoder::read_frame(void)
{
	struct v4l2_buffer buf;
	//	unsigned int i;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				x264_print_err("VIDIOC_DQBUF failed");
				return 0;
		}
	}

	assert(buf.index < mmap_buffer_cnt);

	process_image(buffers[buf.index].start, buf.bytesused);

	if (-1 == x264_ioctl(video_dev_desc, VIDIOC_QBUF, &buf))
		x264_print_err("VIDIOC_QBUF");

	return 1;
}


/*
 * wrapper around ioctl. we should normally get
 * the ioctl done in one go. If we fail due to a
 * signal, retry
 * @in  - fh 	  - the file on which we need the ioctl
 * 	    - request - ioctl code
 * 	    - arg     - arg for ioctl
 *
 * @out - ret	  - ioctl return status (-1 = fail)
 * */
int x264Encoder:: x264_ioctl(int fh, unsigned long int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fh, request, arg);
	} while (-1 == ret && EINTR == errno);

	return ret;
}

/**
 * Allocate the memory for nal and their payload
 * */
bool x264Encoder::initilize()
{
	int i = 0;
	for (; i < MAX_NAL_COUNT; i++) {
		nal_references[i] = (x264_nal_t *)malloc(sizeof(x264_nal_t));
		if (!nal_references[i]) {
			usage_env << "malloc failed for nal %d \n" << i;
			return false;
		}
		nal_references[i]->p_payload = (uint8_t*)malloc(NAL_PAYLOAD_SIZE);
		if (!nal_references[i]->p_payload) {
			usage_env << "malloc failed for nal payload %d \n" << i;
			return false;
		}
	}

	return true;
}

