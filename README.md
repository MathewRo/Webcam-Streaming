# Webcam-Streaming

### Webcam Based Video server implementation(C++)

Webcam based live video streaming server- to N number of clients. Please Do report Bugs if you find any. Feel free to distribute!

# Installation

Webcam Streamer is an application developed for streaming webcam video using a combo of V4l2, x264 and LIVE555. The output Video streamed can be streamed live or maybe stored as per the use.

## Pre-requisites

Webcam Streamer depends on 2 libraries, One for encoding the raw video captured from webcam and one for RTSP streaming.

## libx264

H.264 video encoder. See the H.264 Encoding Guide for more information and usage example. The library is available within the library directory. For building the library, navigate to  `` 
```bash 
chmod -R 777 libraries/x264/.
cd libraries/x264 
./configure --bindir="bin" --enable-static --enable-shared --disable-asm 
make  
```
  You can disable shared if you want to link the application to a static build. In any case you want to upgrade the library from the current version present in the repository to a newer/older version, get the library by

 ```bash 
cd libraries/ 
 wget http://download.videolan.org/pub/x264/snapshots/last_x264.tar.bz2 
tar xjvf last_x264.tar.bz2 
mv x264-snapshot-<date> x264  
 ``` 
  Now proceed with the same step for building

## Live555

LIVE555 Streaming Media[1] is a set of open source (LGPL) C++ libraries for multimedia streaming. The libraries support open standards such as RTP/RTCP and RTSP for streaming, and can also manage video RTP payload formats such as H.264, MPEG, VP8, and DV, and audio RTP payload formats such as MPEG, AAC, AMR, AC-3 and Vorbis. The software distribution also includes a complete RTSP server application,and a RTSP proxy server. For building the library, navigate to

```bash
chmod -R 777 libraries/live/.
cd libraries/live
 ./genMakefiles `<os name>`
 eg:
 ./genMakefiles "linux" 
make
```

If you are working on some other OS, change the  `os name`  accordingly.

## V4L2

Video4Linux, V4L for short, is a collection of device drivers and an API for supporting real time video capture on Linux systems.[1] It supports many USB webcams, TV tuners, and related devices, standardizing their output so programmers can easily add video support to their applications. MythTV, tvtime and TVHeadend are typical applications that use the V4L framework.

If V4l2 library has not been installed in the workstation, try using  
 ```bash 
sudo apt-get install v4l-utils  
``` 
Give password when prompted.

## Run the App

Once the libraries have been installed, navigate to  
```bash 
cd build cmake.. && make -j4 
cd ../bin 
./webcam  
```
