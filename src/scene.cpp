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
    smoothing = 20;

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
        std::cout << "Reading configuration file..." << std::endl;
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
            if(line == "[CROP_COORDS]"){
                currentParsing = CROP_COORDS;
                continue;
            }
            if(line[0] == '['){ //Undefined label
                std::cerr << "Undefined label: " << line << std::endl;
                return -1;
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
        
            // Setting crop values for analysis
            if(currentParsing == CROP_COORDS){
                int cropValues[4];
                std::size_t pos = 1;
                for(int i = 0; i < 3; i++){
                    std::size_t nextPos = value.find(",", pos);
                    cropValues[i] = std::stoi(value.substr(pos, nextPos-pos));
                    pos = nextPos + 1;
                }
                cropValues[3] = std::stoi(value.substr(pos, value.find(")") - pos));
                for(const auto& cap : topCaps){
                    if(cap->capName == key){
                        cap->setCrop(cropValues);
                    } 
                }
            }

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
                if(key == "smoothing") smoothing = std::stoi(value);
            } 
        }
        configFile.close();
        std::cout << "Configuration read!" << std::endl;
    } else return -1; //open file error
    return 0;
}

void Scene::cameraSwitch(){
    // Start threads
    for(auto& cap : topCaps){
         threads.push_back(std::thread(&Capture::motionDetection, std::ref(*cap)));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Threads started" << std::endl;
    
    // read captures parameters to determine which cameras are active.
    uint frameNum = 0;
    int64 last_frame;
    int selectedFrames[topCaps.size()] = { 0 }; // Save the selected frame index as if the swithing were happening every frame
    int shownCaptureIndex = 0;
    while(1){
        // Check if at least one camera is active
        bool atLeastOneActive = false;
        for(auto& cap : topCaps){ 
            if(cap->active){
                atLeastOneActive = true;
                break;
            } 
        }
        if(!atLeastOneActive) break;


        // Select the frame to show based on the frame that has the max momentum.
        double maxMomentum = 0;
        int slectedCapture = -1;
        cv::Mat frameToshow;
        
        
        for(int i = 0; i < topCaps.size(); i++){
            // Wait for the processed frame to be ready for this thread
            std::unique_lock lk(topCaps[i]->mx);
            topCaps[i]->condVar.wait(lk, [&] {return topCaps[i]->readyToRetrive;});

            // Stop signal received
            if(Capture::stopSignalReceived){
                topCaps[i]->readyToRetrive = false;
                lk.unlock();
                topCaps[i]->condVar.notify_one();
                break;
            } 

            if(i == shownCaptureIndex) frameToshow = topCaps[i]->frame.clone();

            if(topCaps[i]->active){
                if(topCaps[i]->momentum > maxMomentum){
                    //std::cout << "topCaps[i]->momentum > maxMomentum" << topCaps[i]->momentum << " > " << maxMomentum << std::endl;
                    maxMomentum = topCaps[i]->momentum;
                    slectedCapture = i;
                }
                if(slectedCapture == -1 && i == topCaps.size()-1){ // Last reached without a max momentum: force a frame
                    slectedCapture = i;
                    std::cout << "Forced" << std::endl;
                } 
            } 
            topCaps[i]->readyToRetrive = false;
            lk.unlock();
            topCaps[i]->condVar.notify_one();
        }

        //Increment the selectedFrame count
        selectedFrames[slectedCapture]++;

        // Every "smooth" frames, the frame to display changes: update shownCaptureIndex
        // ShownCaptureIndex is updated taking into account what has happened since the last update
        if(frameNum % smoothing == 0){
            shownCaptureIndex = std::distance(selectedFrames, std::max_element(selectedFrames, selectedFrames + topCaps.size()));
            std::fill(selectedFrames, selectedFrames + topCaps.size(), 0);
        }

        // Check if a stop signal has been received
        if(Capture::stopSignalReceived){
            for(auto& cap : topCaps){
                cap->readyToRetrive = false;
                cap->condVar.notify_one();
            }
            break;
        }

        // size_t sizeInBytes = frameToshow.total() * frameToshow.elemSize(); //calculate the mat size in byte
        outputFrame(&frameToshow);
        double fps = cv::getTickFrequency()/(cv::getTickCount() - last_frame);
        std::cout << "FPS: " << fps << std::endl;
        last_frame = cv::getTickCount();
        frameNum++;
    }


    // Join the threads
    for(auto& th : threads){
        th.join();
    }
    std::cout << "Thread join" << std::endl;
}

void Scene::outputFrame(cv::Mat* frame){
    //Resize
    cv::resize(*frame, *frame, cv::Size((frame->cols/(double)(frame->rows))*outHeight, outHeight), 0.0, 0.0, cv::INTER_AREA);
    //Crop to out dimensions
    (*frame) = (*frame)(cv::Range(0, outHeight), cv::Range(frame->cols/2 - outWidth/2, frame->cols/2 + outWidth/2));
    // Write to the stream
    outVideo.write(*frame);
    if(displayOutput){
        cv::namedWindow("OUT", cv::WND_PROP_OPENGL);
        cv::setWindowProperty("OUT", cv::WND_PROP_OPENGL, cv::WINDOW_OPENGL);
        cv::waitKey(1);
        if(!getWindowProperty("OUT", cv::WND_PROP_VISIBLE)) displayOutput = false;
        else cv::imshow("OUT", *frame);
    }
}