#include <iostream>
#include <stdio.h>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <UsageEnvironment.hh>
#include "includes/H264LiveServerMediaSession.h"
#include "includes/x264Encoder.h" 
#include "includes/LiveSourceWithx264.h"
int main()
{  
	TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
	BasicUsageEnvironment* usageEnvironment = BasicUsageEnvironment::createNew(*taskSchedular);
	RTSPServer* rtspServer = RTSPServer::createNew(*usageEnvironment, 8554, NULL);
	if(rtspServer == NULL)
	{
		*usageEnvironment << "Failed to create rtsp server ::" << usageEnvironment->getResultMsg() <<"\n";
		exit(1);
	}
	std::string streamName = "webcam";
	ServerMediaSession* sms = ServerMediaSession::createNew(*usageEnvironment, streamName.c_str(), streamName.c_str(), "Live H264 Stream");
	H264LiveServerMediaSession *liveSubSession; 
	liveSubSession = H264LiveServerMediaSession::createNew(*usageEnvironment, true);
	sms->addSubsession(liveSubSession);
	rtspServer->addServerMediaSession(sms);
	char* url = rtspServer->rtspURL(sms);
	*usageEnvironment << "Play the stream using url "<<url << "\n";
	delete[] url;
	taskSchedular->doEventLoop();
	return 0;
}
