#ifndef __H264LIVESERVERMEDIASESSION_H_INCLUDED__
#define __H264LIVESERVERMEDIASESSION_H_INCLUDED__

#include "liveMedia.hh"
#include "LiveSourceWithx264.h"
class OnDemandServerMediaSubSession;
class H264LiveServerMediaSession:public OnDemandServerMediaSubsession
{
	public:
		static H264LiveServerMediaSession* createNew(UsageEnvironment& env, bool reuseFirstSource);
		void checkForAuxSDPLine1();
		void afterPlayingDummy1();
	protected:
		H264LiveServerMediaSession(UsageEnvironment& env, bool reuseFirstSource);
		virtual ~H264LiveServerMediaSession(void);
		void setDoneFlag() { fDoneFlag = ~0; }
	protected:
		virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
		virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
		virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
	private:
		char* fAuxSDPLine;
		char fDoneFlag;
		RTPSink* fDummySink;
};  

#endif
