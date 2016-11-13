#include "includes/x264Encoder.h"
#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define M 640
#define N 480
#include <stdio.h>
struct buffer {
	void   *start;
	size_t  length;
};



const char              *dev_name = "/dev/video0";
static int              fd = -1;
struct buffer           *buffers;
static unsigned int     n_buffers;
long row = 		M*N/2;
void 			yuyv(const void * , int);	
static x264_nal_t 	*nalc0;
static x264_nal_t	*nalc1;
static x264_nal_t       *nalc2;
static x264_nal_t       *nalc3;

x264Encoder::x264Encoder(void)
{

}

x264Encoder::~x264Encoder(void)
{

}

void x264Encoder::unInitilize()
{
	free(nalc0->p_payload);
	free(nalc1->p_payload);
	free(nalc2->p_payload);
	free(nalc3->p_payload);
}

void x264Encoder::encodeFrame()
{

}

bool x264Encoder::isNalsAvailableInOutputQueue()
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

x264_nal_t * x264Encoder::getNalUnit()
{

	x264_nal_t * nal;
	nal = outputQueue.front();
	outputQueue.pop();
	return nal;
}
void x264Encoder:: open_device()
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
				dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
				dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void x264Encoder:: init_device()
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;


	if (-1 == this->xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n",
					dev_name);
			exit(EXIT_FAILURE);
		} else {
			this->errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n",
				dev_name);
		exit(EXIT_FAILURE);
	}


	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		fprintf(stderr, "%s does not support streaming i/o\n",
				dev_name);
		exit(EXIT_FAILURE);
	}


	/* Select video input, video standard and tune here. */


	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == this->xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					/* Cropping not supported. */
					break;
				default:
					/* Errors ignored. */
					break;
			}
		}
	} else {
		/* Errors ignored. */
	}


	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = M;
	fmt.fmt.pix.height      = N;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;

	if (-1 ==  this->xioctl(fd, VIDIOC_S_FMT, &fmt))
		this->errno_exit("VIDIOC_S_FMT");

}

void x264Encoder:: init_mmap()
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == this->xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
					"memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			this->errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
				dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = (buffer *)calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == this->xioctl(fd, VIDIOC_QUERYBUF, &buf))
			this->errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL /* start anywhere */,
					buf.length,
					PROT_READ | PROT_WRITE /* required */,
					MAP_SHARED /* recommended */,
					fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			this->errno_exit("mmap");
	}
}

void x264Encoder:: start_capturing()
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			this->errno_exit("VIDIOC_QBUF");
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == this->xioctl(fd, VIDIOC_STREAMON, &type))
		this->errno_exit("VIDIOC_STREAMON");

}
void x264Encoder:: mainloop()
{
	unsigned int count;
	int frame_count=1;
	count = frame_count;
	while (count-- > 0) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			/* Timeout. */
			tv.tv_sec = 10;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;
				this->errno_exit("select [h264]");
			}

			if (0 == r) {
				fprintf(stderr, "v4l2 select timeout\n");
				exit(EXIT_FAILURE);
			}
			
			if (this->read_frame())
				break;
			/* EAGAIN - continue select loop. */
		}
	}
}

void x264Encoder:: stop_capturing()
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 ==this->xioctl(fd, VIDIOC_STREAMOFF, &type))
		this->errno_exit("VIDIOC_STREAMOFF");
}
void x264Encoder:: uninit_device()
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap(buffers[i].start, buffers[i].length))
			this->errno_exit("munmap");		
	free(buffers);
}

void x264Encoder:: close_device()
{
	if (-1 == close(fd))
		this->errno_exit("close");

	fd = -1;
}
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
		exit(1);


	param.i_csp = X264_CSP_I420;
	param.i_width  = M;
	param.i_height = N;
	param.b_vfr_input = 1;
	param.b_repeat_headers = 1;
	param.b_annexb = 1;
	param.i_fps_num = 24;
   	param.i_fps_den = 1;

	
	if( x264_param_apply_profile( &param, "baseline" ) < 0 )
		exit(1);

	if( x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height ) < 0 )
		exit(1);

	h = x264_encoder_open( &param );
	if( !h )
		exit(1);


	typedef struct
	{
		uint8_t YUYVSource[M * N * 2];
	}__attribute__((__packed__)) frame_t;
	frame_t *frame3;

	frame3 =(frame_t *)p;

	for (i = 0; i != M * N ; ++i)
	{	
		pic.img.plane[0][i] = frame3->YUYVSource[2*i];
	}
	for (j = 0; j != M * N / 4 ; j++)
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
			memcpy(nalc0->p_payload,nal[0].p_payload,nal[0].i_payload);
			nalc0->i_payload=nal[0].i_payload;
			outputQueue.push(nalc0);
			memcpy(nalc1->p_payload,nal[1].p_payload,nal[1].i_payload);
                        nalc1->i_payload=nal[1].i_payload;
                        outputQueue.push(nalc1);
			memcpy(nalc2->p_payload,nal[2].p_payload,nal[2].i_payload);
                        nalc2->i_payload=nal[2].i_payload;
                        outputQueue.push(nalc2);
			memcpy(nalc3->p_payload,nal[3].p_payload,nal[3].i_payload);
                        nalc3->i_payload=nal[3].i_payload;
                        outputQueue.push(nalc3);
		
	}
	while( x264_encoder_delayed_frames( h ) )
	{
		i_frame_size = x264_encoder_encode( h, &nal, &i_nal, NULL, &pic_out );

		if(i_frame_size > 0)
		{
				
	                memcpy(nalc0->p_payload,nal[0].p_payload,nal[0].i_payload);
                        nalc0->i_payload=nal[0].i_payload;
                        outputQueue.push(nalc0);
                        memcpy(nalc1->p_payload,nal[1].p_payload,nal[1].i_payload);
                        nalc1->i_payload=nal[1].i_payload;
                        outputQueue.push(nalc1);
                        memcpy(nalc2->p_payload,nal[2].p_payload,nal[2].i_payload);
                        nalc2->i_payload=nal[2].i_payload;
                        outputQueue.push(nalc2);
                        memcpy(nalc3->p_payload,nal[3].p_payload,nal[3].i_payload);
                        nalc3->i_payload=nal[3].i_payload;
                        outputQueue.push(nalc3);

		}

	}
	
x264_encoder_close( h );
x264_picture_clean( &pic );

}

void x264Encoder:: process_image(const void * p, int size)
{
	yuyv(p,size);
}
void x264Encoder:: errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}
int x264Encoder::read_frame(void)
{
	struct v4l2_buffer buf;
	//	unsigned int i;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == this->xioctl(fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				this->errno_exit("VIDIOC_DQBUF");
		}
	}

	assert(buf.index < n_buffers);

	this->process_image(buffers[buf.index].start, buf.bytesused);

	if (-1 == this->xioctl(fd, VIDIOC_QBUF, &buf))
		this->errno_exit("VIDIOC_QBUF");

	return 1;
}


int x264Encoder::xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

void x264Encoder::initilize()
{
	nalc0 = (x264_nal_t *)malloc(sizeof(x264_nal_t));
        nalc0->p_payload =(uint8_t*)malloc(60000);
        nalc1 = (x264_nal_t *)malloc(sizeof(x264_nal_t));
        nalc1->p_payload =(uint8_t*)malloc(60000);
        nalc2 = (x264_nal_t *)malloc(sizeof(x264_nal_t));
        nalc2->p_payload =(uint8_t*)malloc(60000);
        nalc3 = (x264_nal_t *)malloc(sizeof(x264_nal_t));
        nalc3->p_payload =(uint8_t*)malloc(60000);

}

