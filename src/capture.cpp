#include "capture.h"
#include <chrono>
#include <thread>
#include <csignal>

#define DILATE_SIZE 5

Capture::Capture(std::string _capName, std::string _source) : VideoCapture(_source){
    capName = _capName;
    source = _source;
    processedFrameNum = -1; // frame that is being processed
    readyToRetrive = false;
    //std::fill(area, area+FRAME_BUFFER, 0); // init array
    area = 0;
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
    cv::Mat currentFrame, previousFrame, originalFrame, differenceFrame;
    cv::Point centroid, previousCentroid;
    while(1){
        active = true;
        if(!read(originalFrame)) break;
        //Crop the frame
        currentFrame = originalFrame.clone();
        currentFrame = currentFrame(cv::Range(0, originalFrame.rows), cv::Range(1000, 3200));
        
        if(stopSignalReceived){
            readyToRetrive = true;
            break;
        } 
        cvtColor(currentFrame, currentFrame, cv::COLOR_BGR2GRAY); //gray scale
        GaussianBlur(currentFrame, currentFrame, cv::Size(5,5), 2); //gussian blur
        cv::waitKey(10);
        //if((processedFrameNum + 1) > 2 && !cv::getWindowProperty(capName, cv::WND_PROP_VISIBLE)) break;
        if(processedFrameNum + 1) {
            absdiff(previousFrame, currentFrame, differenceFrame);
            // make the areas bigger
            dilate(differenceFrame, differenceFrame, cv::getStructuringElement( cv::MORPH_ELLIPSE,
                       cv::Size( DILATE_SIZE*DILATE_SIZE + 1, DILATE_SIZE*DILATE_SIZE+1 ),
                       cv::Point( 2, 2 )));
            //Threshold (black and white)
            threshold(differenceFrame, differenceFrame, 20, 255, cv::THRESH_BINARY);
            //Find contours
            std::vector<std::vector<cv::Point>> contours;
            findContours(differenceFrame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            //drawContours(originalFrame, contours, -1, cv::Scalar(0, 255, 0), 20);
            //Calculate areas
            int xValueMax = 0, xValueMin = 2*get(cv::CAP_PROP_FRAME_WIDTH);
            int yValueMax = 0, yValueMin = 2*get(cv::CAP_PROP_FRAME_HEIGHT);

            double totalArea = 0;
            for(int i = 0; i < contours.size(); i++){
                double area = contourArea(contours[i]);
                if (area < 50){
                    continue;
                }
                //std::cout << "AREA VALE: " << area << std::endl;

                // find moments of the image
                // cv::Moments m = cv::moments(differenceFrame,true);
                // centroid = cv::Point(m.m10/m.m00, m.m01/m.m00);
                // cv::Rect rect = boundingRect(contours[i]);
                //if(rect.x < TOP_CORNER_X || rect.x > BOTTOM_CORNER_X || rect.y < TOP_CORNER_Y || rect.y > BOTTOM_CORNER_Y) continue;
                //std::cout<< cv::Mat(centroid)<< std::endl;

                //Find the min and max x, y.
                // int x = (rect.x + rect.width/2);
                // int y = (rect.y + rect.height/2);
                // if(xValueMax < x) xValueMax = x;
                // if(yValueMax < y) yValueMax = y;
                // if(xValueMin > x) xValueMin = x;
                // if(yValueMin > y) yValueMin = y;
                // rectangle(currentFrame, rect, cv::Scalar(0, 255, 0), 1);
                totalArea += area;
                
            }
            
            
            //acquire lock
            std::unique_lock lk(mx);
            condVar.wait(lk, [this] {return !readyToRetrive;});
            area = totalArea;
            frame.release();
            frame = originalFrame.clone();
            readyToRetrive = true;
            // Unlock and notify
            lk.unlock();
            condVar.notify_one();
            
            //circle(currentFrame, cv::Point((xValueMax-xValueMin)/2, (yValueMax-yValueMin)/2), 10, cv::Scalar(255,0,0));
            // circle(currentFrame, centroid, 10, cv::Scalar(255,0,0));
            // cv::resize(currentFrame, currentFrame, cv::Size((int)(ratio*400), 400), 0.0, 0.0, cv::INTER_AREA);
            // imshow(capName, currentFrame);
        }
        //imshow("Processed", processedFrame);
        previousFrame.release();
        previousFrame = currentFrame.clone(); //Save the previous frame
        //processedFrame.release();
        currentFrame.release();
        originalFrame.release();
        ++processedFrameNum;
    }
    active = false;
}