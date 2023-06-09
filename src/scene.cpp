#include "scene.h"
#include "capture.h"
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <ctime>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

Scene::Scene(const std::string configFilePath){
    // Default config values
    outWidth = 1920;
    outHeight = 1080;
    outPath = "./out/out.mp4";
    displayOutput = false;
    smoothing = 20;
    fpsToFile = false;
    displayAllCaptures=false;
    fpsFilePath = "../out/FPS.csv";
    camToAnalyzeCount = 0;


    // Reading config File
    try{
        readConfigFile(configFilePath);
    }catch (const std::exception& e){
        std::cerr << "[CONFIG FILE ERROR]: " << e.what() << std::endl;
        exit(1);
    }catch(...){
        std::cerr << "[CONFIG FILE ERROR]: Unknown Error while reading the file!" << std::endl;
        exit(1);
    }

    // Open fps file if the fpsToFile is true in the config file
    if(fpsToFile){
        fpsStream.open(fpsFilePath, std::ofstream::out | std::ofstream::trunc);
        if(!fpsStream.is_open()){
            std::cerr << "[FPS FILE OPENING ERROR]" << std::endl;
            exit(1);
        }
        fpsStream << "fps"; // Insert top label in the file
    }

    //Init the general monitor with the right dimensions
    if(displayAllCaptures){
        int height = 224 + 112*((captures.size()-1)/4);
        generalMonitor = cv::Mat::zeros(cv::Size(1350, height),CV_8UC3);
        outGeneralMonitor = cv::VideoWriter("../out/monitor.mp4", cv::VideoWriter::fourcc('m','p','4','v'),25, cv::Size(1350,height));
        cv::namedWindow("General Monitor", cv::WINDOW_NORMAL);
        cv::resizeWindow("General Monitor", 1350, height);
    }

    // outVideo init
    outVideo = cv::VideoWriter(outPath, cv::VideoWriter::fourcc('m','p','4','v'),25, cv::Size(outWidth,outHeight));
}

Scene::~Scene(){
    if(fpsToFile) fpsStream.close();
    releaseCaps();
    outVideo.release();
    outGeneralMonitor.release();
    cv::destroyAllWindows();
}

std::ostream& operator <<(std::ostream& os, const Scene& scene){
    os << "CAPTURES" << std::endl;
    for(auto& cap : scene.captures) os << "[" << *cap << "]" << std::endl;
    return os;
}

void Scene::releaseCaps() const{
    for(auto& cap : captures) cap->release();
}

void Scene::displayCaptures(){
    cv::destroyWindow("General Monitor");
    for(auto& cap : captures){
        threads.push_back(std::thread(&Capture::display, std::ref(*cap)));
    }
    for(auto& th : threads){ // Wait for each thread
        th.join();
    }
}

void Scene::readConfigFile(const std::string& configFilePath){
    std::string_view currentParsing; // What the program is parsing
    const std::vector<std::string_view> configFileLabels = {"[CAM_TO_ANALYZE]", "[CAM_TO_SHOW]","[ASSOCIATIONS]", "[OUT]", 
                                                            "[GENERAL]", "[CROP_COORDS]", "[WEIGHTS]", 
                                                            "[DISPLAY_ANALYSIS]"}; 
    std::ifstream configFile(configFilePath);
    if (configFile.is_open()) {
        std::cout << "Reading configuration file..." << std::endl;
        std::string line;
        while (std::getline(configFile, line)) { // read the configFile line by line 
            line.erase(std::remove_if(line.begin(), line.end(), isspace),line.end()); // Erasing the spaces
            if(line[0] == '#' || line.empty()) continue; // Skip empty line or comments
            
            if(line[0] == '['){ //It's a label
                bool found = false;
                for (const auto& label : configFileLabels){
                    if(line == label){
                        currentParsing = label;
                        found = true;
                        break;
                    }
                }
                if(found)continue;
                else throw std::invalid_argument("Undefined label " + line); //Undefined label
            }
        
            std::size_t delimiterPos = line.find("="); // find the = sign
            std::string key = line.substr(0, delimiterPos); // key, before the = sign
            std::string value = line.substr(delimiterPos + 1); // value, after the = sign
            
            // Check if the file exists
            if(currentParsing == "[CAM_TO_ANALYZE]" || currentParsing == "[CAM_TO_SHOW]"){
                std::ifstream file(value); 
                if(!file.good()) throw std::invalid_argument("Error opening video stream " + value);
            }

            // Create the caps
            if(currentParsing == "[CAM_TO_ANALYZE]"){
                captures.push_back(std::make_shared<Capture>(key, value, true));
                camToAnalyzeCount++;
            } 
            if(currentParsing == "[CAM_TO_SHOW]"){
                bool exists = false;
                for(const auto& cap : captures){
                    if(cap->capName == key) exists = true;
                }
                if(!exists){
                    captures.push_back(std::make_shared<Capture>(key, value, false));
                    camToShowCount++;
                }
            } 

            if(currentParsing == "[ASSOCIATIONS]"){
                // Check if all cameras exists
                int camAnalysisIndex = -1;
                int camToShowIndex = -1;
                for(int i =  0; i< captures.size(); i++){
                    if(captures[i]->analysis && captures[i]->capName == key) camAnalysisIndex = i;
                    if(captures[i]->capName == value) camToShowIndex = i;
                }
                if(camAnalysisIndex == -1) throw std::invalid_argument("The camera '" + key + "' provided in the [ASSOCIATIONS] list must exist in the [CAM_TO_ANALYZE] section.");
                if(camToShowIndex == -1) throw std::invalid_argument("The camera '" + value + "' provided in the [ASSOCIATIONS] list must exist in the [CAM_TO_SHOW] section.");
                // init associations array
                if(associations.size() == 0) associations = std::vector<std::vector<int>>(camToAnalyzeCount);
                associations[camAnalysisIndex].push_back(camToShowIndex); // insert the association
            }
        
            // Setting crop values for analysis
            if(currentParsing == "[CROP_COORDS]"){
                int cropValues[4];
                std::size_t pos = 1;
                for(int i = 0; i < 3; i++){
                    std::size_t nextPos = value.find(",", pos);
                    cropValues[i] = std::stoi(value.substr(pos, nextPos-pos));
                    pos = nextPos + 1;
                }
                cropValues[3] = std::stoi(value.substr(pos, value.find(")") - pos));
                for(const auto& cap : captures){
                    if(cap->capName == key){
                        cap->setCrop(cropValues);
                    } 
                }
                continue;
            }

            if(currentParsing == "[WEIGHTS]"){
                //Check if in range
                const int w = std::stoi(value);
                if(w < 1 || w > 5) throw std::invalid_argument("The weight value '" + value + "' in '" + line + "' is not included in the [1-5] interval");
                for(const auto& cap : captures) if(cap->capName == key) cap->setWeight(w);
                continue;
            }

            if(currentParsing == "[DISPLAY_ANALYSIS]"){
                for(const auto& cap : captures) if(cap->capName == key && value == "true") cap->setDisplayAnalysis(true);
                continue;
            }

            // Setting output parameters
            if(currentParsing == "[OUT]"){
                if(key == "outPath") outPath = value;
                if(key == "width") outWidth = std::stoi(value);
                if(key == "height") outHeight = std::stoi(value);
                continue;
            }

            //Setting general parameters
            if(currentParsing == "[GENERAL]"){
                if(key == "displayOutput" && value == "true") displayOutput = true;
                if(key == "smooth"){
                    int tmp = std::stoi(value);
                    if(tmp <= 0)throw std::invalid_argument("The smooth value '" + value + "' in '" + line + "' must be greater than 0");
                    smoothing = tmp;
                } 
                if(key == "fpsToFile" && value == "true") fpsToFile = true;
                if(key == "displayAllCaptures" && value == "true") displayAllCaptures=true;
                if(key == "fpsFilePath") fpsFilePath = value;
                if(key == "alpha"){
                    double a = std::stod(value);
                    if(a <= -1 || a >= 1) throw std::invalid_argument("The alpha value '" + value + "' in '" + line + "' is not included in the ]-1,1[ interval");
                    else Capture::alpha = a;
                } 
                continue;
            } 
        }
        configFile.close();
        checkAssociationsIntegrity();
        std::cout << "Configuration read!" << std::endl;
    } else throw std::invalid_argument("Error while opening the config file. Check the config file name and path.\n--help for help.");
}

void Scene::checkAssociationsIntegrity()const{
    if(associations.size() == 0) throw std::invalid_argument("You must include some camera associations (under the [ASSOCIATIONS] label) in the config file.");
    for(int i = 0; i < camToAnalyzeCount; i++){
        if(associations[i].size() == 0) throw std::invalid_argument("Every analyzed camera must be associated with at least one camera, it can also be the analyzed camera itself.");
    }
}

void Scene::cameraSwitch(){
    // Start threads
    for(const auto& cap : captures){
        if(cap->analysis) threads.push_back(std::thread(&Capture::motionDetection, std::ref(*cap)));
        else threads.push_back(std::thread(&Capture::grabFrame, std::ref(*cap))); // just grab frames for camera that are not analyzed
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Threads started\nPress Ctrl+C to stop" << std::endl;
    
    uint frameNum = 0; // keep record of the processed frame number
    std::chrono::time_point<std::chrono::system_clock> last_frame = std::chrono::system_clock::now();
    int selectedFrames[captures.size()] = { 0 }; // Save the selected frame index as if the switching were happening every frame
    int shownCaptureIndex = 0; // Index of the analyzed winning camera
    int fpsToDisplay = 0, fps = 0;
    
    while(1){
        if(!isAtLeastOneActive(captures)) break;
        if(displayAllCaptures && !(frameNum%15))clearGeneralMonitor();

        // Select the caps to show based on the cap that has the max score.
        double maxScore = 0;
        int slectedCapture = -1;
        cv::Mat frameToshow;

        for(int i = 0; i < captures.size(); i++){
            if(captures[i]->active){
                // Wait for the processed frame to be ready for this thread
                std::unique_lock lk(captures[i]->mx);
                captures[i]->condVar.wait(lk, [&] {return (captures[i]->readyToRetrieve || !captures[i]->active);});
                
                // Stop signal received
                if(Capture::stopSignalReceived){
                    captures[i]->readyToRetrieve = false;
                    lk.unlock();
                    captures[i]->condVar.notify_one();
                    break;
                } 

                if(captures[i]->active && captures[i]->analysis){
                    if(captures[i]->score > maxScore){ // find the maximum score
                        maxScore = captures[i]->score;
                        slectedCapture = i;
                    }
                    if(slectedCapture == -1 && i == captures.size()-1){ // Last reached without a max score: force a frame
                        slectedCapture = i;
                    } 
                } 
                // Change the selected capture based on the associations matrix
                // Copy the frame to show based on the associations
                if(i == shownCaptureIndex) frameToshow = captures[i]->frame.clone();
                
                // Set the general monitor
                if(displayAllCaptures) assembleGeneralMonitor(captures[i], frameNum, i == shownCaptureIndex, i, frameToshow);
                // std::cout << "SCENE: " + std::to_string(camToAnalyze[i]->area) + " " +  std::to_string(camToAnalyze[i]->vel) + " " + std::to_string(camToAnalyze[i]->weight) + " " + std::to_string(camToAnalyze[i]->area_n) + " " + std::to_string(camToAnalyze[i]->score)<< std::endl;
                captures[i]->readyToRetrieve = false;
                lk.unlock();
                captures[i]->condVar.notify_one();
            }
            
        }

        //Increment the selectedFrame count
        selectedFrames[slectedCapture]++;

        // Every "smooth" frames, the frame to display changes: update shownCaptureIndex
        // ShownCaptureIndex is updated taking into account what has happened since the last update
        if(frameNum % smoothing == 0){
            shownCaptureIndex = std::distance(selectedFrames, std::max_element(selectedFrames, selectedFrames + captures.size()));
            std::fill(selectedFrames, selectedFrames + captures.size(), 0);
        }

        // Check if a stop signal has been received
        if(Capture::stopSignalReceived){
            for(auto& cap : captures){
                cap->readyToRetrieve = false;
                cap->condVar.notify_one();
            }
            break;
        }

        //Calculate the fps
        std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now()-last_frame;
        fps = (int)(1/(elapsed_seconds.count()));
        last_frame = std::chrono::system_clock::now();

        //Update fpsToDisplay every 15 frames
        if(!(frameNum % 15)) fpsToDisplay = fps;
        if(fpsToFile && fps > 0 && fps < 3000) fpsStream << "\n" + std::to_string(fps);

        //Output frame 
        try{
            //std::cout << "TEST " + std::to_string(frameNum) + "\n";
            if(!frameToshow.empty())outputFrame(&(frameToshow), fpsToDisplay);
            outputGeneralMonitor(&generalMonitor, fpsToDisplay);
        } catch(const cv::Exception& e){
            std::cerr << "[OUTPUT FRAME EXCEPTION]: " << e.what() << std::endl;
            Capture::stopSignalReceived = true;
        } catch(...){
            std::cerr << "[OUTPUT FRAME EXCEPTION]: Unknown exception" << std::endl;
            Capture::stopSignalReceived = true;
        }
        frameNum++;
    }

    std::cout << "Waiting for threads to stop..." << std::endl;
    // Join the threads
    for(auto& th : threads){
        th.join();
    }
    std::cout << "Threads joined" << std::endl;
}

bool Scene::isAtLeastOneActive(const std::vector<std::shared_ptr<Capture>>& caps)const{
    // Check if at least one camera is active
    bool atLeastOneActive = false;
    for(const auto& cap : caps){ 
        if(cap->active){
            atLeastOneActive = true;
            break;
        } 
    }
    return atLeastOneActive;
}

void Scene::assembleGeneralMonitor(const std::shared_ptr<Capture>& cap, const int frameNum, const bool isLive, const int capNum, const cv::Mat& frameToShow){
    cv::resize(cap->frame, cap->frame, cv::Size((cap->frame.cols/(double)(cap->frame.rows))*112, 112), 0.0,0.0, cv::INTER_AREA);
    cap->frame = cap->frame(cv::Range(0, 112), cv::Range(cap->frame.cols/(double)2 - 99.5, cap->frame.cols/(double)2 + 99.5));
    
    if(isLive) cv::rectangle(cap->frame, cv::Rect(0,0, cap->frame.cols, cap->frame.rows), cv::Scalar(0,255,0), 4,8);
    
    // update text in the monitor
    if(!(frameNum%15)){
        std::stringstream stream;
        stream << std::fixed << std::setprecision(1) << cap->area;
        std::vector<std::string> stats = {std::to_string(capNum + 1), 
                                        std::to_string(cap->area_n),
                                        stream.str()};
        stream.str(std::string()); //clear the stream
        stream << std::fixed << std::setprecision(2) << cap->vel;
        stats.push_back(stream.str());
        stats.push_back(std::to_string(cap->weight));
        stream.str(std::string()); //clear the stream
        stream << std::fixed << std::setprecision(1) << cap->score;
        stats.push_back(stream.str());
        for(int j = 0; j < stats.size(); j++){
            cv::putText(generalMonitor, stats[j], cv::Point(810 + 90*j, 60 + capNum*30), cv::FONT_HERSHEY_PLAIN, 1.3, CV_RGB(230, 230, 230), 1, cv::LINE_AA);
        }
    }
    cv::putText(cap->frame, std::to_string(capNum+1), cv::Point(6,20), cv::FONT_HERSHEY_PLAIN, 1.3, CV_RGB(230, 230, 230), 1, cv::LINE_AA);
    int xOffset = capNum < 4 ? 398 + 199*(capNum%2) : 199*(capNum%4);
    int yOffset = capNum < 4 ? 112*(capNum/2) : 224 + 112*((capNum-4)/4);
    cap->frame.copyTo(generalMonitor.rowRange(yOffset, yOffset + 112).colRange(xOffset, xOffset + cap->frame.cols));
    if(isLive){ // show the top left output
        cv::Mat tmp = frameToShow.clone();
        cv::resize(tmp, tmp, cv::Size((tmp.cols/(double)tmp.rows)*224, 224), 0.0,0.0, cv::INTER_AREA);
        tmp = tmp(cv::Range(0, tmp.rows), cv::Range(tmp.cols/(double)2 - 199, tmp.cols/(double)2 + 199));
        tmp.copyTo(generalMonitor.rowRange(0, tmp.rows).colRange(0, tmp.cols));
    } 
}

void Scene::clearGeneralMonitor(){
    generalMonitor = cv::Scalar(33,33,33);
    cv::putText(generalMonitor, "CAM     N       A       V       W       S", cv::Point(810, 30), cv::FONT_HERSHEY_PLAIN, 1.3, CV_RGB(230, 230, 230), 1, cv::LINE_AA);
    cv::putText(generalMonitor, "N = number of areas, A = area, V = avg speed", cv::Point(810, generalMonitor.rows - 24), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(230, 230, 230), 1, cv::LINE_AA);
    cv::putText(generalMonitor, "W = frame weight, S = final score", cv::Point(810, generalMonitor.rows - 10), cv::FONT_HERSHEY_PLAIN, 1, CV_RGB(230, 230, 230), 1, cv::LINE_AA);
}

void Scene::outputGeneralMonitor(cv::Mat* frame, int fps){
    if(displayAllCaptures){
        outGeneralMonitor.write(*frame);
        cv::waitKey(1);
        if(!getWindowProperty("General Monitor", cv::WND_PROP_VISIBLE)) displayAllCaptures = false;
        else{
            cv::putText(generalMonitor, "FPS: " + std::to_string(fps), cv::Point(810, generalMonitor.rows - 45), cv::FONT_HERSHEY_PLAIN, 1.2, CV_RGB(230, 230, 230), 1, cv::LINE_AA);
            cv::imshow("General Monitor", *frame);
        } 
    }
}

void Scene::outputFrame(cv::Mat* frame, int fps){
    // Resize and crop
    double ratio = frame->cols/(double)(frame->rows);
    if(ratio*outHeight >= outWidth){ // resize fixed height
        cv::resize(*frame, *frame, cv::Size((frame->cols/(double)(frame->rows))*outHeight, outHeight), 0.0, 0.0, cv::INTER_AREA);
        //Crop to out dimensions
        (*frame) = (*frame)(cv::Range(0, outHeight), cv::Range(frame->cols/2 - outWidth/2, frame->cols/2 + outWidth/2));
    } else { // resize fixed width
        cv::resize(*frame, *frame, cv::Size(outWidth, (frame->rows/(double)frame->cols)*outWidth), 0.0, 0.0, cv::INTER_AREA);
        //Crop to out dimensions
        (*frame) = (*frame)(cv::Range(frame->rows/2 - outHeight/2, frame->rows/2 + outHeight/2), cv::Range(0, outWidth));
    }
    
    cv::Mat writeFrame = frame->clone();
    // Write to the stream
    outVideo.write(writeFrame);
    if(displayOutput){
        cv::namedWindow("OUT", cv::WINDOW_AUTOSIZE);
        cv::waitKey(1);
        if(!getWindowProperty("OUT", cv::WND_PROP_VISIBLE)) displayOutput = false;
        else{
            //Resize for displaying
            cv::resize(*frame, *frame, cv::Size(1200,(frame->rows/(double)frame->cols)*1200));
            //FPS LABEL
            cv::putText(*frame, //target image
            "FPS: " + std::to_string(fps), //text
            cv::Point(10, 40), //top-left position
            cv::FONT_HERSHEY_SIMPLEX,
            1.0,
            CV_RGB(0, 255, 0), //font color
            2);
            cv::imshow("OUT", *frame);
        } 
    }
}
