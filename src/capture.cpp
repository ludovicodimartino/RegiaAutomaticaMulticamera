#include "capture.h"
#include <chrono>
#include <thread>
#include <csignal>

#define DILATE_SIZE 5

Capture::Capture(std::string _capName, std::string _source) : VideoCapture(_source){
    if(!isOpened()){
        std::cerr << "Video file opening error" << std::endl;
        exit(1);
    }
    capName = _capName;
    source = _source;
    processedFrameNum = -1; // frame that is being processed
    readyToRetrive = false;
    momentum = 0;
    active = false;
    ratio = get(cv::CAP_PROP_FRAME_WIDTH)/get(cv::CAP_PROP_FRAME_HEIGHT);
}

bool Capture::stopSignalReceived = false;

std::ostream& operator <<(std::ostream& os, const Capture& cap){
    os << "CAPTURE NAME: " << cap.capName << " CAPTURE PATH: " << cap.source << " RATIO: " << cap.ratio;
    return os;
}

bool Capture::operator==(const Capture& cap)const{
    return capName == cap.capName;
}

cv::Mat Capture::crop(const cv::Rect cropRect, const cv::Mat& uncuttedFrame)const{
    return uncuttedFrame(cropRect).clone();
}

void Capture::display(){
    unsigned int frameNum = 0;
    cv::Mat currentFrame, resized;
    while(1){
        //std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if(!read(currentFrame)) break;
        if(frameNum > 2 && !getWindowProperty(capName, cv::WND_PROP_VISIBLE)) break; // close the window
        if(stopSignalReceived) break;
        cv::resize(currentFrame, resized, cv::Size((int)(ratio*400), 400));
        cv::namedWindow(capName, cv::WINDOW_NORMAL);
        imshow(capName, resized);
        cv::waitKey(1);
        frameNum++;
        //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        //std::cout << "FPS = " << 1000/(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()) << "[fps]" << std::endl;
    }
}

void Capture::motionDetection(){
    std::cout << "thread ID: " << std::this_thread::get_id() << " Name: " << capName << std::endl;
    cv::Mat currentFrame, previousFrame, originalFrame, currDiffFrame;
    // cv::Point centroid, previousCentroid;
    while(1){
        active = true;
        if(!read(originalFrame)) break;
        
        // Copy the original frame
        currentFrame = originalFrame.clone();
        // Crop the frame in order to consider just the playground
        currentFrame = currentFrame(cv::Range(0, originalFrame.rows), cv::Range(350, 1570));
        
        // Check if a stop signal has arrived
        if(stopSignalReceived){
            readyToRetrive = true;
            break;
        }

        // Processing
        cvtColor(currentFrame, currentFrame, cv::COLOR_BGR2GRAY); //gray scale
        GaussianBlur(currentFrame, currentFrame, cv::Size(5,5), 2); //gussian blur
        
        if(processedFrameNum + 1) {
            absdiff(previousFrame, currentFrame, currDiffFrame);
            // make the areas bigger
            dilate(currDiffFrame, currDiffFrame, cv::getStructuringElement( cv::MORPH_ELLIPSE,
                       cv::Size( DILATE_SIZE*DILATE_SIZE + 1, DILATE_SIZE*DILATE_SIZE+1 ),
                       cv::Point( 2, 2 )));
            //Threshold (black and white)
            threshold(currDiffFrame, currDiffFrame, 20, 255, cv::THRESH_BINARY);
        
            //GET MOMENTUM
            std::vector<std::vector<cv::Point>> contours;
            findContours(currDiffFrame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            //drawContours(originalFrame, contours, -1, cv::Scalar(0, 255, 0), 20);
            double area = getArea(contours);
            double avgVel = getAvgVelocity(currentFrame, previousFrame, contours);
            double m = area*avgVel;
            //acquire lock
            std::unique_lock lk(mx);
            condVar.wait(lk, [this] {return !readyToRetrive;});
            momentum = m; // update the area
            frame.release();
            frame = originalFrame.clone(); // update the frame
            readyToRetrive = true;
            // Unlock and notify
            lk.unlock();
            condVar.notify_one();  
        }

        previousFrame.release();
        previousFrame = currentFrame.clone(); // Save the previous frame
        currentFrame.release();
        currDiffFrame.release();
        originalFrame.release();
        ++processedFrameNum;
    }
    active = false;
}

double Capture::getArea(const std::vector<std::vector<cv::Point>>& contours)const{
    // Calculate the area
    double totalArea = 0;
    for(int i = 0; i < contours.size(); i++){
        double area = contourArea(contours[i]);
        if (area < 50){ // Do not include obj with a small area
            continue;
        }
        totalArea += area;
    }
    return totalArea;
}

double Capture::getAvgVelocity(const cv::Mat& currFrameGray, const cv::Mat& prevFrameGray, const std::vector<std::vector<cv::Point>>& contours){

    if(contours.size() == 0) return 0;

    std::vector<cv::Moments> m; //Moments
    for(const auto& contour : contours){
        m.push_back(cv::moments(contour, true));
    }

    std::vector<cv::Point2f> mc; //Centroids
    for(const auto& moment : m){
        mc.push_back(cv::Point2f( moment.m10/moment.m00 , moment.m01/moment.m00 ));
    }

    // Optical Flow Lucas-Kanade method
    std::vector<uchar> status;
    std::vector<float> err;
    std::vector<cv::Point2f> calcPoints;
    cv::TermCriteria criteria = cv::TermCriteria((cv::TermCriteria::COUNT) + (cv::TermCriteria::EPS), 10, 0.03);
    cv::calcOpticalFlowPyrLK(prevFrameGray, currFrameGray, mc, calcPoints, status, err, cv::Size(15,15), 2, criteria);

    std::vector<cv::Point2f> good_new;
    int good = 0;
    double velocitySum = 0;
    for(int i = 0; i < mc.size(); i++){
        // Select good points
        if(status[i] == 1) {
            good++;
            cv::Point2f diff = mc[i] - calcPoints[i];    
            velocitySum += cv::sqrt((diff.x*diff.x) + (diff.y*diff.y));
            good_new.push_back(calcPoints[i]);
        }
    }
    return (velocitySum/good)*100; // *100 to avoid sub 1 values
}