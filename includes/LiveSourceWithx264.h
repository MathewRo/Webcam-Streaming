#ifndef __LIVESOURCEWITHX264_H_INCLUDED__
#define __LIVESOURCEWITHX264_H_INCLUDED__

#include <queue>
#include "x264Encoder.h"
#include "H264LiveServerMediaSession.h"

class LiveSourceWithx264:public FramedSource
{
	public:
		static LiveSourceWithx264* createNew(UsageEnvironment& env);
		static EventTriggerId eventTriggerId;
	protected:
		LiveSourceWithx264(UsageEnvironment& env);
		virtual ~LiveSourceWithx264(void);
	private:
		virtual void doGetNextFrame();
		static void deliverFrame0(void* clientData);
		void deliverFrame();
		void encodeNewFrame();
		static unsigned referenceCount;
		std::queue<x264_nal_t *> nalQueue;
		timeval currentTime;
		x264Encoder *encoder;
}; 
#endif
