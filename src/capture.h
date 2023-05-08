#ifndef __CAPTURE__
#define __CAPTURE__

#include <opencv2/opencv.hpp>
#include <string.h>
#include <iostream>
#include <mutex>
#include <condition_variable>

#define DILATE_SIZE 5

class Capture : public cv::VideoCapture{
private:
    unsigned int processedFrameNum;
    double ratio;
    int cropCoords[4];
    int weight;
    double getArea(const std::vector<std::vector<cv::Point>>& contours)const;
    double getAvgVelocity(const cv::Mat& currFrameGray, const cv::Mat& prevFrameGray, const std::vector<std::vector<cv::Point>>& contours);

public:
    static bool stopSignalReceived;
    std::string capName;
    std::string source;
    cv::Mat frame;
    double momentum;
    bool active;
    bool readyToRetrieve;
    std::mutex mx;
    std::condition_variable condVar;
    Capture(std::string _capName, std::string _source);
    friend std::ostream& operator <<(std::ostream& os, const Capture& cap);
    void display();
    void motionDetection();
    void setCrop(const int cropArray[]);
    void setWeight(const int w);
    bool operator==(const Capture& cap)const;
};
#endif
