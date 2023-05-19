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

    //Init the zero mat with the right dimensions
    if(displayAllCaptures){
        int n = topCaps.size() + lateralCaps.size();
        int height = 224 + 112*((n-1)/4);
        generalMonitor = cv::Mat::zeros(cv::Size(1270, height),CV_8UC3);
    }

    // outVideo init
    outVideo = cv::VideoWriter(outPath, cv::VideoWriter::fourcc('m','p','4','v'),25, cv::Size(outWidth,outHeight));
}

Scene::~Scene(){
    if(fpsToFile) fpsStream.close();
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
        throw std::invalid_argument("Not a valid argument");
    }
    std::vector<std::thread> threads; // Each cap is shown in a new thread
    for(auto& cap : capsToDisplay){
        threads.push_back(std::thread(&Capture::display, std::ref(*cap)));
    }
    for(auto& th : threads){ // Wait for each thread
        th.join();
    }
}

void Scene::readConfigFile(const std::string& configFilePath){
    std::string_view currentParsing; // What the program is parsing
    const std::vector<std::string_view> configFileLabels = {"[TOP_CAMERAS]", "[LATERAL_CAMERAS]", "[OUT]", 
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
            if(currentParsing == "[TOP_CAMERAS]" || currentParsing == "[LATERAL_CAMERAS]"){
                std::ifstream file(value); 
                if(!file.good()) throw std::invalid_argument("Error opening video stream " + value);
            }

            // Create the caps
            if(currentParsing == "[TOP_CAMERAS]") topCaps.push_back(std::make_shared<Capture>(key, value));
            if(currentParsing == "[LATERAL_CAMERAS]") lateralCaps.push_back(std::make_shared<Capture>(key, value));
        
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
                for(const auto& cap : topCaps){
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
                for(const auto& cap : topCaps) if(cap->capName == key) cap->setWeight(w);
                continue;
            }

            if(currentParsing == "[DISPLAY_ANALYSIS]"){
                for(const auto& cap : topCaps) if(cap->capName == key && value == "true") cap->setDisplayAnalysis(true);
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
        std::cout << "Configuration read!" << std::endl;
    } else throw std::invalid_argument("Error while opening the config file. Check the config file name and path.\n--help for help.");
}

void Scene::cameraSwitch(){
    // Start threads
    for(auto& cap : topCaps){
         threads.push_back(std::thread(&Capture::motionDetection, std::ref(*cap)));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Threads started\nPress Ctrl+C to stop" << std::endl;
    
    uint frameNum = 0;
    //int64 last_frame = 0;
    std::chrono::time_point<std::chrono::system_clock> last_frame = std::chrono::system_clock::now();
    int selectedFrames[topCaps.size()] = { 0 }; // Save the selected frame index as if the swithing were happening every frame
    int shownCaptureIndex = 0;
    int fpsToDisplay = 0, fps = 0;
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


        // Select the frame to show based on the frame that has the max score.
        double maxMomentum = 0;
        int slectedCapture = -1;
        cv::Mat frameToshow;
        
        // clean the generalMonitor Mat
        if(displayAllCaptures && !(frameNum%15)){
            generalMonitor = cv::Scalar(33,33,33);
            cv::putText(generalMonitor, "CAM     A       V       W       S", cv::Point(810, 30), cv::FONT_HERSHEY_PLAIN, 1.3, CV_RGB(230, 230, 230), 1);
            cv::putText(generalMonitor, "A = area, V = avg velocity, W = frame weight, S = final score", cv::Point(810, generalMonitor.rows - 10), cv::FONT_HERSHEY_PLAIN, 0.8, CV_RGB(230, 230, 230), 1);
        } 
        for(int i = 0; i < topCaps.size(); i++){
            if(topCaps[i]->active){
                // Wait for the processed frame to be ready for this thread
                std::unique_lock lk(topCaps[i]->mx);
                topCaps[i]->condVar.wait(lk, [&] {return (topCaps[i]->readyToRetrieve || !topCaps[i]->active);});
                // Stop signal received
                if(Capture::stopSignalReceived){
                    topCaps[i]->readyToRetrieve = false;
                    lk.unlock();
                    topCaps[i]->condVar.notify_one();
                    break;
                } 


                if(topCaps[i]->active){
                    if(topCaps[i]->score > maxMomentum){
                        maxMomentum = topCaps[i]->score;
                        slectedCapture = i;
                    }
                    if(slectedCapture == -1 && i == topCaps.size()-1){ // Last reached without a max score: force a frame
                        slectedCapture = i;
                        //std::cout << "Forced camera" << std::endl;
                    } 
                    
                } 

                // Copy the frame to show
                if(i == shownCaptureIndex) frameToshow = topCaps[i]->frame.clone();

                if(displayAllCaptures){
                    topCaps[i]->frame = topCaps[i]->frame(cv::Range(0, topCaps[i]->frame.rows), cv::Range(topCaps[i]->frame.cols/2 - 199, topCaps[i]->frame.cols/2 + 199));
                    cv::resize(topCaps[i]->frame, topCaps[i]->frame, cv::Size(0.0,0.0), 0.5,0.5);
                    if(i == shownCaptureIndex) cv::rectangle(topCaps[i]->frame, cv::Rect(0,0, topCaps[i]->frame.cols, topCaps[i]->frame.rows), cv::Scalar(0,255,0), 4,8);
                    if(!(frameNum%15)){
                        std::vector<std::string> stats = {std::to_string(i+1), 
                                                        std::to_string((int)topCaps[i]->area), 
                                                        std::to_string((int)topCaps[i]->vel),
                                                        std::to_string(topCaps[i]->weight),
                                                        std::to_string((int)topCaps[i]->score)};
                        for(int j = 0; j < stats.size(); j++){
                            cv::putText(generalMonitor, stats[j], cv::Point(810 + 90*j, 60 + i*30), cv::FONT_HERSHEY_PLAIN, 1.3, CV_RGB(230, 230, 230), 1);
                        }
                    }
                    cv::putText(topCaps[i]->frame, std::to_string(i+1), cv::Point(6,20), cv::FONT_HERSHEY_PLAIN, 1.3, CV_RGB(230, 230, 230), 1);
                    int xOffset = i < 4 ? 398 + 199*(i%2) : 199*(i%4);
                    int yOffset = i < 4 ? 112*(i/2) : 224 + 112*((i-4)/4);
                    topCaps[i]->frame.copyTo(generalMonitor.rowRange(yOffset, yOffset + 112).colRange(xOffset, xOffset + topCaps[i]->frame.cols));
                    if(!frameToshow.empty()){
                        frameToshow = frameToshow(cv::Range(0, frameToshow.rows), cv::Range(frameToshow.cols/2 - 199, frameToshow.cols/2 + 199));
                        frameToshow.copyTo(generalMonitor.rowRange(0, frameToshow.rows).colRange(0, frameToshow.cols));
                    } 
                }
                topCaps[i]->readyToRetrieve = false;
                lk.unlock();
                topCaps[i]->condVar.notify_one();
            }
            
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
                cap->readyToRetrieve = false;
                cap->condVar.notify_one();
            }
            break;
        }

        //Calculate the fps
        // fps = (int)std::floor(cv::getTickFrequency()/(cv::getTickCount() - last_frame));
        // last_frame = cv::getTickCount();
        std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now()-last_frame;
        fps = (int)(1/(elapsed_seconds.count()));
        last_frame = std::chrono::system_clock::now();

        //Update fpsToDisplay every 15 frames
        if(!(frameNum % 15)) fpsToDisplay = fps;
        if(fps > 0 && fps < 3000) fpsStream << "\n" + std::to_string(fps);

        //Output frame 
        try{
            //if(!frameToshow.empty())outputFrame(&frameToshow, fpsToDisplay);
            outMultiCamMonitor(&generalMonitor);
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

void Scene::outputFrame(cv::Mat* frame, int fps){
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

void Scene::outMultiCamMonitor(cv::Mat* frame){
    if(displayAllCaptures){
        cv::namedWindow("Monitor", cv::WND_PROP_OPENGL);
        cv::setWindowProperty("Monitor", cv::WND_PROP_OPENGL, cv::WINDOW_OPENGL);
        cv::waitKey(1);
        if(!getWindowProperty("Monitor", cv::WND_PROP_VISIBLE)) displayAllCaptures = false;
        else cv::imshow("Monitor", *frame);
    }
}