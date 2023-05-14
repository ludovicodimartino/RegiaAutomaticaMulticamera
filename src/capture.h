#ifndef __CAPTURE__
#define __CAPTURE__

#include <opencv2/opencv.hpp>
#include <string.h>
#include <iostream>
#include <mutex>
#include <condition_variable>

#define DILATE_SIZE 2

class Capture : public cv::VideoCapture{
private:
    unsigned int processedFrameNum;
    double ratio;
    int cropCoords[4];
    int weight;
    std::map<std::string, std::string> paramToDisplay;
    bool isdisplayAnalysis;
    double getArea(const std::vector<std::vector<cv::Point>>& contours)const;
    double getAvgVelocity(const cv::Mat& currFrameGray, const cv::Mat& prevFrameGray, const std::vector<std::vector<cv::Point>>& contours);
    void displayAnalysis(const cv::Mat& diffFrame, const cv::Mat& croppedFrame, const std::vector<std::vector<cv::Point>>& contours, const double area, const double avgVel);
    //DA RIMUOVERE
    cv::VideoWriter analysisOut;
public:
    static bool stopSignalReceived;
    static double alpha;
    std::string capName;
    std::string source;
    cv::Mat frame;
    double score;
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
    void setDisplayAnalysis(const bool da);
    bool operator==(const Capture& cap)const;
};
#endif
