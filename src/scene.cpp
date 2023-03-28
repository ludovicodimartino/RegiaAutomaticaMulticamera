#include "scene.h"
#include "capture.h"
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

Scene::Scene(const std::string configFilePath){
    // Default config values
    outWidth = 1920;
    outHeight = 1080;
    outPath = "./out/out.mp4";
    displayOutput = false;

    // Reading config File
    if(readConfigFile(configFilePath)) std::cerr << "Configuration file error!";

    // outVideo init
    outVideo = cv::VideoWriter(outPath, cv::VideoWriter::fourcc('m','p','4','v'),25, cv::Size(outWidth,outHeight));
}

Scene::~Scene(){
    releaseCaps();
    outVideo.release();
    cv::destroyAllWindows();
}

void Scene::signalHandler(int signum){
    std::cout << "SIGNAL RECEIVED" << std::endl;
}

std::ostream& operator <<(std::ostream& os, const Scene& scene){
    os << "TOP CAPTURES" << std::endl;
    for(auto& cap : scene.topCaps) os << "[" << *cap << "]" << std::endl;
    os << "LATERAL CAPTURES" << std::endl;
    for(auto& cap : scene.lateralCaps) os << "[" << *cap << "]" << std::endl;
    return os;
}

void Scene::releaseCaps() const{
    for(auto& cap : topCaps) cap->release();
    for(auto& cap : lateralCaps) cap->release();  
}

void Scene::displayCaptures(const int cameraType) const{
    std::vector<std::shared_ptr<Capture>> capsToDisplay;
    switch (cameraType)
    {
    case TOP:
        capsToDisplay = topCaps;
        break;
    case LATERAL:
        capsToDisplay = lateralCaps;
        break;
    case (TOP | LATERAL):
        capsToDisplay.reserve( topCaps.size() + lateralCaps.size() ); // preallocate memory
        capsToDisplay.insert( capsToDisplay.end(), topCaps.begin(), topCaps.end() );
        capsToDisplay.insert( capsToDisplay.end(), lateralCaps.begin(), lateralCaps.end() );
        break;
    default:
        throw "NOT A VALID ARGUMENT";
        return;
    }
    std::vector<std::thread> threads; // Each cap is shown in a new thread
    for(auto& cap : capsToDisplay){
        threads.push_back(std::thread(&Capture::display, std::ref(*cap)));
    }
    for(auto& th : threads){ // Wait for each thread
        th.join();
    }
}

int Scene::readConfigFile(const std::string& configFilePath){
    ConfigFileLabels currentParsing; // What the program is parsing
    std::ifstream configFile(configFilePath);
    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) { // read the configFile line by line 
            line.erase(std::remove_if(line.begin(), line.end(), isspace),line.end()); // Erasing the spaces
            if(line[0] == '#' || line.empty()) continue; // Skip empty line or comments
            if(line == "[TOP_CAMERAS]"){
                currentParsing = TOP_CAMERAS;
                continue;
            }
            if(line == "[LATERAL_CAMERAS]"){
                currentParsing = LATERAL_CAMERAS;
                continue;
            }
            if(line == "[OUT]"){
                currentParsing = OUT;
                continue;
            }
            if(line == "[GENERAL]"){
                currentParsing = GENERAL;
                continue;
            }
            std::size_t delimiterPos = line.find("="); // find the = sign
            std::string key = line.substr(0, delimiterPos); // key, before the = sign
            std::string value = line.substr(delimiterPos + 1); // value, after the = sign
            
            // Check if the file exists
            if(currentParsing == TOP_CAMERAS || currentParsing == LATERAL_CAMERAS){
                std::ifstream file(value); 
                if(!file.good()) return -1;
            }

            // Create the caps
            if(currentParsing == TOP_CAMERAS) topCaps.push_back(std::make_shared<Capture>(key, value));
            if(currentParsing == LATERAL_CAMERAS) lateralCaps.push_back(std::make_shared<Capture>(key, value));
        
            // Setting output parameters
            if(currentParsing == OUT){
                if(key == "outPath") outPath = value;
                if(key == "width") outWidth = std::stoi(value);
                if(key == "height") outHeight = std::stoi(value);
            }

            //Setting general parameters
            if(currentParsing == GENERAL){
                if(key == "displayOutput"){
                    if(value == "true") displayOutput = true;
                }
            } 
        }
        configFile.close();
    } else return -1; //open file error
    return 0;
}

void Scene::cameraSwitch(){
    //topCaps[0].motionDetection();
    for(auto& cap : topCaps){
         threads.push_back(std::thread(&Capture::motionDetection, std::ref(*cap)));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Threads started" << std::endl;
    // read captures parameters to determine which camera is the active one.
    unsigned int frameNum = 0;
    while(1){
        bool atLeastOneActive = false;
        for(auto& cap : topCaps){
            if(cap->active){
                atLeastOneActive = true;
                break;
            } 
        }
        if(!atLeastOneActive) break;

        double maxArea = 0;
        std::shared_ptr<Capture> shownCamera;
        cv::Mat frameToshow; 
        for(auto& cap : topCaps){
            // Wait for the processed frame to be ready for this thread
            std::unique_lock lk(cap->mx);
            cap->condVar.wait(lk, [cap] {return cap->readyToRetrive;});
            if(Capture::stopSignalReceived){
                cap->readyToRetrive = false;
                lk.unlock();
                cap->condVar.notify_one();
                break;
            } 
            if(cap->active && cap->area > maxArea){
                maxArea = cap->area;
                shownCamera = cap;
                frameToshow = cap->frame.clone();
            } 
            cap->readyToRetrive = false;
            lk.unlock();
            cap->condVar.notify_one();
        }

        if(Capture::stopSignalReceived){
            for(auto& cap : topCaps){
                cap->readyToRetrive = false;
                cap->condVar.notify_one();
            }
            break;
        } 

        //CenterCamera if none camera is selected
        if(shownCamera == nullptr){
            std::cout << "CAMERA FORZATA" << std::endl;
            shownCamera = topCaps[1];
        } 

        
        // Write to the stream
        cv::resize(frameToshow, frameToshow, cv::Size(outWidth, outHeight), 0.0, 0.0, cv::INTER_AREA);
        outVideo.write(frameToshow);
        if(displayOutput){
            cv::namedWindow("OUT", cv::WND_PROP_OPENGL);
            cv::setWindowProperty("OUT", cv::WND_PROP_OPENGL, cv::WINDOW_OPENGL);
            cv::waitKey(1);
            if(!getWindowProperty("OUT", cv::WND_PROP_VISIBLE)) displayOutput = false;
            else cv::imshow("OUT", frameToshow);
        }
        std::cout << "Shown camera " << shownCamera->capName << std::endl;
        frameNum++;
    }
    
    for(auto& th : threads){
        th.join();
    }
    std::cout << "Thread join" << std::endl;
}