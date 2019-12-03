/**
 * @file 	webcam.cpp
 *
 * @brief	Application main. Application
 *			can be used for streaming web-
 *			-cam to different clients
 *
 * @Note  	None
 * 
 *
 * @author 	Rohit Philip Mathew
 * @Contact	rohitpmathew@yahoo.co.in
 *
 */


#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <UsageEnvironment.hh>
#include "H264LiveServerMediaSession.h"
#include "x264Encoder.h" 
#include "LiveSourceWithx264.h"

#define DEFAULT_PORT 	8554
#define BYTE_SIZE 		8
#define SERVER_NAME 	"webcam"

int main(int argc, char ** argv)
{  
    // Basic declarations
    const char * stream_name = SERVER_NAME;
    H264LiveServerMediaSession *live_subsession = NULL;
    ServerMediaSession* server_media_session = NULL;
    Port * port = NULL;
    char* url = NULL;
    RTSPServer* rtsp_server = NULL;

    // Set up the environment and event handler
    TaskScheduler* task_scheduler = BasicTaskScheduler::createNew();
    // For Linux, it is fine to use the subclass from the library
    BasicUsageEnvironment* usage_environment = BasicUsageEnvironment::createNew(*task_scheduler);

    // check for arguments if any
    if (argc < 2) {
        *usage_environment << "port not specified, using default port \n";
        port = new Port(DEFAULT_PORT);
    } else {
        int temp_port = atoi(argv[1]);
        //check if we exceed 16 bits
        if ((temp_port >> (sizeof(short)*BYTE_SIZE) & 0xFFFFFFFF)) {
            *usage_environment << "please provide a 16 bit parameter for the port! exiting \n";
            return EXIT_FAILURE;
        }
        port = new Port((portNumBits)temp_port);
    }

    // Create RTSP server
    rtsp_server = RTSPServer::createNew(*usage_environment, *port, NULL);

    if (!rtsp_server) {
        *usage_environment << "Failed to create rtsp server : " << usage_environment->getResultMsg() << "\n";
        return EXIT_FAILURE;
    }

    // Create Media session
    server_media_session = ServerMediaSession::createNew(*usage_environment,
            stream_name,
            stream_name,
            "streaming web-camera live");

    // Select encoder related session
    live_subsession = H264LiveServerMediaSession::createNew(*usage_environment, true);
    server_media_session->addSubsession(live_subsession);
    // use the above session for the rtsp server
    rtsp_server->addServerMediaSession(server_media_session);
    url = rtsp_server->rtspURL(server_media_session);

    *usage_environment << "Play stream using url "<< url << "\n";

    task_scheduler->doEventLoop();
    return EXIT_SUCCESS;
}
