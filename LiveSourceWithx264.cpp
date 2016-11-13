#include "includes/LiveSourceWithx264.h"
#include <stdio.h>

LiveSourceWithx264* LiveSourceWithx264::createNew(UsageEnvironment& env)
{
	return new LiveSourceWithx264(env);
}

EventTriggerId LiveSourceWithx264::eventTriggerId = 0;

unsigned LiveSourceWithx264::referenceCount = 0;

LiveSourceWithx264::LiveSourceWithx264(UsageEnvironment& env):FramedSource(env)
{
	if(referenceCount == 0)
	{

	}
	++referenceCount;
	encoder = new x264Encoder();
	encoder->initilize();
	encoder->open_device();
	encoder->init_device();
	encoder->init_mmap();
	encoder->start_capturing();	
	encoder->mainloop();
	if(eventTriggerId == 0)
	{
		eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
	}
}


LiveSourceWithx264::~LiveSourceWithx264(void)
{
	--referenceCount;
	encoder->stop_capturing();
	encoder->uninit_device();
	encoder->close_device();
	envir().taskScheduler().deleteEventTrigger(eventTriggerId);
	eventTriggerId = 0;
}

void LiveSourceWithx264::encodeNewFrame()
{	
	encoder->mainloop();
	while(encoder->isNalsAvailableInOutputQueue() == true)
	{
		x264_nal_t * nal = encoder->getNalUnit();
		nalQueue.push(nal);
	}
}

void LiveSourceWithx264::deliverFrame0(void* clientData)
{
	((LiveSourceWithx264*)clientData)->deliverFrame();
}

void LiveSourceWithx264::doGetNextFrame()
{
	if(nalQueue.empty() == true)
	{
		encodeNewFrame(); 
		gettimeofday(&currentTime,NULL);
		deliverFrame();

	}
	else
	{
		deliverFrame();

	}
}

void LiveSourceWithx264::deliverFrame()
{  
	if(!isCurrentlyAwaitingData()) 
	return;
	x264_nal_t * nal = nalQueue.front();
	nalQueue.pop();
	assert(nal->p_payload != NULL);
	int truncate = 0;
	if (nal->i_payload >= 4 && nal->p_payload[0] == 0 && nal->p_payload[1] == 0 && nal->p_payload[2] == 0 && nal->p_payload[3] == 1 )
	{
		truncate = 4;
	}
	else
	{
		if(nal->i_payload >= 3 && nal->p_payload[0] == 0 && nal->p_payload[1] == 0 && nal->p_payload[2] == 1 )
		{
			truncate = 3;
		}
	}

	if(nal->i_payload-truncate >(signed int)fMaxSize)
	{
		fFrameSize = fMaxSize;
		fNumTruncatedBytes = nal->i_payload-truncate - fMaxSize;
	}
	else
	{
		fFrameSize = nal->i_payload-truncate;
	}
	fPresentationTime = currentTime;
	memmove(fTo,nal->p_payload + truncate,fFrameSize);
	FramedSource::afterGetting(this);           
        


}  

