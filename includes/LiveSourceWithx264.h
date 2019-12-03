#ifndef __LIVESOURCEWITHX264_H_INCLUDED__
#define __LIVESOURCEWITHX264_H_INCLUDED__

#include <queue>
#include "x264Encoder.h"
#include "H264LiveServerMediaSession.h"

typedef enum {
CS_UNINITIALIZED,
CS_FREE_MEM,
CS_CLOSE_FILE,
CS_MUNMAP_FILE,
CS_CAPTURE_STOP
} clean_state_t;


class LiveSourceWithx264:public FramedSource
{
	public:
		static LiveSourceWithx264* createNew(UsageEnvironment& env);
		static EventTriggerId eventTriggerId;
		bool Livex264Setup();
	protected:
		LiveSourceWithx264(UsageEnvironment& env);
		virtual ~LiveSourceWithx264(void);
	private:
		void recursiveClean(clean_state_t cleanState);
		virtual void doGetNextFrame();
		static void deliverFrame0(void* clientData);
		void deliverFrame();
		void encodeNewFrame();
		static unsigned referenceCount;
		std::queue <x264_nal_t *> nalQueue;
		timeval currentTime;
		x264Encoder *encoder;
}; 
#endif
