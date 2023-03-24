#ifndef __CAPTURE__
#define __CAPTURE__

#include <opencv2/opencv.hpp>
#include <string.h>
#include <iostream>
#include <mutex>

#define FRAME_BUFFER 250

class Capture : public cv::VideoCapture{
public:
    std::string capName;
    std::string source;
    unsigned int processedFrameNum;
    cv::Mat frameBuffer[FRAME_BUFFER];
    double area[FRAME_BUFFER];
    double ratio;
    bool active;
    std::mutex mx;
    Capture(std::string _capName, std::string _source);
    friend std::ostream& operator <<(std::ostream& os, const Capture& cap);
    void display();
    void motionDetection();
    bool operator==(const Capture& cap)const;
};

#endif