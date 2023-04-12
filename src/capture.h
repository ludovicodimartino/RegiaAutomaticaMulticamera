#ifndef __CAPTURE__
#define __CAPTURE__

#include <opencv2/opencv.hpp>
#include <string.h>
#include <iostream>
#include <mutex>
#include <condition_variable>

class Capture : public cv::VideoCapture{
private:
    double getArea(const std::vector<std::vector<cv::Point>>& contours)const;
    double getAvgVelocity(const cv::Mat& currFrameGray, const cv::Mat& prevFrameGray, const std::vector<std::vector<cv::Point>>& contours);

public:
    static bool stopSignalReceived;
    std::string capName;
    std::string source;
    unsigned int processedFrameNum;
    cv::Mat frame;
    double momentum;
    double ratio;
    bool active;
    bool readyToRetrive;
    std::mutex mx;
    std::condition_variable condVar;
    Capture(std::string _capName, std::string _source);
    friend std::ostream& operator <<(std::ostream& os, const Capture& cap);
    void display();
    void motionDetection();
    cv::Mat crop(const cv::Rect cropRect, const cv::Mat& uncuttedFrame)const;
    bool operator==(const Capture& cap)const;
};
#endif
