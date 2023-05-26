#ifndef __SCENE__
#define __SCENE__

#include "capture.h"
#include <iostream>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <memory>
#include <mutex>
#include <thread>

#define MONITOR_BORDER 5

typedef enum CameraType{
    TOP = 1,
    LATERAL = 2
}CameraType;

class Scene{
public:
    Scene(const std::string configFilePath);
    ~Scene();
    friend std::ostream& operator <<(std::ostream& os, const Scene& scene);
    void displayCaptures(const int cameraType)const; //@param TOP, LATERAL, TOP | LATERAL
    void cameraSwitch();
private:
    std::vector<std::shared_ptr<Capture>> topCaps; // ceiling mounted cameras
    std::vector<std::shared_ptr<Capture>> lateralCaps; // wall mounted cameras
    std::vector<std::thread> threads; // threads used for doing the capture computations 
    std::string outPath; // Path of the out stream
    int outWidth;
    int outHeight;
    bool displayOutput;
    cv::VideoWriter outVideo;
    cv::VideoWriter outGeneralMonitor;
    int smoothing;
    bool fpsToFile;
    bool displayAllCaptures;
    cv::Mat generalMonitor;
    std::string fpsFilePath;
    std::ofstream fpsStream;
    bool isAtLeastOneActive(const std::vector<std::shared_ptr<Capture>>& caps)const;
    void readConfigFile(const std::string& configFilePath);
    void releaseCaps()const;
    void clearGeneralMonitor();
    void assembleGeneralMonitor(const std::shared_ptr<Capture>& cap, const int frameNum, const bool isLive, const int capNum, const cv::Mat& frameToShow);
    void outputGeneralMonitor(cv::Mat* frame, int fps);
    void outputFrame(cv::Mat* frame, int fps);
};

#endif
