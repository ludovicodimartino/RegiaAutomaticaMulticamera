#ifndef __CAPTURE__
#define __CAPTURE__

#include <opencv2/opencv.hpp>
#include <string.h>
#include <iostream>
#include <mutex>
#include <condition_variable>

#define FRAME_BUFFER 250

class Capture : public cv::VideoCapture{
public:
    static bool stopSignalReceived;
    std::string capName;
    std::string source;
    unsigned int processedFrameNum;
    cv::Mat frame;
    double area;
    double ratio;
    bool active;
    bool readyToRetrive;
    std::mutex mx;
    std::condition_variable condVar;
    Capture(std::string _capName, std::string _source);
    friend std::ostream& operator <<(std::ostream& os, const Capture& cap);
    void display();
    void motionDetection();
    bool operator==(const Capture& cap)const;
};
#endif
