cmake_minimum_required(VERSION 2.8)

get_filename_component(RUNTIME_PATH  . ABSOLUTE)
message(STATUS "Runtime path is " ${RUNTIME_PATH})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++0x -rdynamic -ggdb -Wall")


include_directories(
	${RUNTIME_PATH}/includes
        ${RUNTIME_PATH}/libraries/live/liveMedia/include
        ${RUNTIME_PATH}/libraries/live/BasicUsageEnvironment/include
        ${RUNTIME_PATH}/libraries/live/UsageEnvironment/include
        ${RUNTIME_PATH}/libraries/live/groupsock/include
        ${RUNTIME_PATH}/libraries/x264
	)

link_directories(	
        ${RUNTIME_PATH}/libraries/live/liveMedia
        ${RUNTIME_PATH}/libraries/live/BasicUsageEnvironment
        ${RUNTIME_PATH}/libraries/live/UsageEnvironment
        ${RUNTIME_PATH}/libraries/live/groupsock
        ${RUNTIME_PATH}/libraries/x264
	)

add_executable(webcam
	${RUNTIME_PATH}/source/H264LiveServerMediaSession.cpp
	${RUNTIME_PATH}/source/LiveSourceWithx264.cpp
	${RUNTIME_PATH}/source/x264Encoder.cpp

	${RUNTIME_PATH}/webcam.cpp
	)

set(EXECUTABLE_OUTPUT_PATH  ../bin/)
target_link_libraries(webcam -lliveMedia -lBasicUsageEnvironment -lUsageEnvironment -lgroupsock -lx264 -lpthread -ldl)

