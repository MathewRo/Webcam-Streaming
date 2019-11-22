/**
 * @file 	webcam.cpp
 *
 * @brief	Application main. Application
 *		can be used for streaming web- 
 *		-cam to different clients
 *
 * @Note  	None
 * 
 *
 * @author 	Rohit Philip Mathew
 * @Contact	rohitpmathew@yahoo.co.in
 *
 */


#include <iostream>
#include <stdio.h>
#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <UsageEnvironment.hh>
#include "H264LiveServerMediaSession.h"
#include "x264Encoder.h" 
#include "LiveSourceWithx264.h"

#define DEFAULT_PORT 8554


int main(int argc, char ** argv)
{  
	// Basic declarations
	std::string * stream_name = NULL;
	TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
	BasicUsageEnvironment* usageEnvironment = BasicUsageEnvironment::createNew(*taskSchedular);

	RTSPServer* rtspServer = RTSPServer::createNew(*usageEnvironment, 8554, NULL);

	if (!rtspServer) {
		*usageEnvironment << "Failed to create rtsp server ::" << usageEnvironment->getResultMsg() <<"\n";
		return EXIT_FAILURE;
	}

	stream_name = new std::string("webcam");
	ServerMediaSession* sms = ServerMediaSession::createNew(*usageEnvironment, 
								stream_name->c_str(), 
								stream_name->c_str(), 
								"Live H264 Stream");

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
