#include <LiveSourceWithx264.h>
#include <stdio.h>


EventTriggerId LiveSourceWithx264::eventTriggerId = 0;
unsigned LiveSourceWithx264::referenceCount = 0;


LiveSourceWithx264* LiveSourceWithx264::createNew(UsageEnvironment& env)
{
	return new LiveSourceWithx264(env);
}

LiveSourceWithx264::LiveSourceWithx264(UsageEnvironment& env):FramedSource(env)
{
	++referenceCount;
	encoder = new x264Encoder(env);
}

void LiveSourceWithx264::recursiveClean(clean_state_t cleanState) {
	switch(cleanState) {
	case CS_CAPTURE_STOP:
		encoder->stop_capturing();
	case CS_MUNMAP_FILE:
		encoder->uninit_device();
	case CS_CLOSE_FILE:
		encoder->close_device();
	case CS_FREE_MEM:
		encoder->unInitilize();
	case CS_UNINITIALIZED:
		break;
	}
}

bool LiveSourceWithx264::Livex264Setup(void) {

	if (!encoder->initilize()) {
		recursiveClean(CS_FREE_MEM);
		return false;
	}

	if(!encoder->open_device()) {
		/* If we fail here, we still don't have the file opened */
		recursiveClean(CS_FREE_MEM);
		return false;
	}

	if (!encoder->init_device()) {
		recursiveClean(CS_CLOSE_FILE);
		return false;
	}

	if (!encoder->init_mmap()) {
		/* If we fail here, we still don't have the memory map set */
		recursiveClean(CS_CLOSE_FILE);
		return false;
	}

	if(!encoder->start_capturing()) {
		recursiveClean(CS_MUNMAP_FILE);
		return false;
	}

	encoder->mainloop();
	if(eventTriggerId == 0)
	{
		eventTriggerId = envir().taskScheduler().createEventTrigger(deliverFrame0);
	}
	return true;
}

LiveSourceWithx264::~LiveSourceWithx264(void)
{
	--referenceCount;
	recursiveClean(CS_CAPTURE_STOP);
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
	if (nalQueue.empty() == true) {
		encodeNewFrame(); 
		gettimeofday(&currentTime,NULL);
		deliverFrame();

	}
	else {
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

