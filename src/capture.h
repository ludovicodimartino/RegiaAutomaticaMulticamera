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
    std::map<std::string, std::string> paramToDisplay;
    bool isdisplayAnalysis;
    double getArea(const std::vector<std::vector<cv::Point>>& contours);
    double getAvgSpeed(const cv::Mat& currFrameGray, const cv::Mat& prevFrameGray, const std::vector<std::vector<cv::Point>>& contours);
    void displayAnalysis(const cv::Mat& diffFrame, const cv::Mat& croppedFrame, const std::vector<std::vector<cv::Point>>& contours, const double area, const double avgVel);
    void preProcessing(cv::Mat* f)const;
    void frameDifferencing(cv::Mat* dst, cv::Mat* f1, cv::Mat* f2)const;
    cv::VideoWriter analysisOut;
public:
    static bool stopSignalReceived;
    static double alpha;
    std::string capName;
    std::string source;
    cv::Mat frame;
    bool analysis; // If the score will be calculated
    int weight;
    int area_n;
    double score;
    double area;
    double vel;
    bool active;
    bool readyToRetrieve;
    std::mutex mx;
    std::condition_variable condVar;
    Capture(std::string _capName, std::string _source, bool _analysis);
    friend std::ostream& operator <<(std::ostream& os, const Capture& cap);
    void display();
    void FrameDiffAreaAndVel();
    void FrameDiffAreaOnly();
    void grabFrame();
    void setCrop(const int cropArray[]);
    void setWeight(const int w);
    void setDisplayAnalysis(const bool da);
    bool operator==(const Capture& cap)const;
};
#endif
