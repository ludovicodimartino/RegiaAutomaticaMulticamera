#include "capture.h"
#include <chrono>
#include <thread>
#include <csignal>

Capture::Capture(std::string _capName, std::string _source, bool _analysis) : VideoCapture(_source){
    if(!isOpened()){
        std::cerr << "[CAPTURE " << _capName << "]: Video stream opening error!" << std::endl;
        exit(1);
    }
    capName = _capName;
    source = _source;
    analysis = _analysis;
    processedFrameNum = -1; // frame number that is being processed
    readyToRetrieve = false;
    isdisplayAnalysis = false;
    score = 0;
    active = true;
    weight = 1;
    area = 0;
    area_n = 0;
    vel = 0;
    paramToDisplay = {{"FINAL_SCORE", "0"}, {"AREAS_NUM", "0"}, {"WEIGHT", std::to_string(weight)},
                      {"AVG_SPEED", "0"}, {"AREA", "0"}};
    cropCoords[0] = 0;
    cropCoords[1] = get(cv::CAP_PROP_FRAME_HEIGHT);
    cropCoords[2] = 0;
    cropCoords[3] = get(cv::CAP_PROP_FRAME_WIDTH);
    ratio = get(cv::CAP_PROP_FRAME_WIDTH)/get(cv::CAP_PROP_FRAME_HEIGHT);

}

bool Capture::stopSignalReceived = false;
double Capture::alpha = 0;

std::ostream& operator <<(std::ostream& os, const Capture& cap){
    os << "CAPTURE NAME: " << cap.capName << " CAPTURE PATH: " << cap.source << " RATIO: " << cap.ratio << " ANALYSIS: " << cap.analysis;
    return os;
}

bool Capture::operator==(const Capture& cap)const{
    return capName == cap.capName;
}

void Capture::setCrop(const int cropArray[]){
    for(int i = 0; i < 4; i++){
        cropCoords[i] = cropArray[i];
    }
}

void Capture::setWeight(const int w){
    weight = w;
}

void Capture::setDisplayAnalysis(const bool da){
    isdisplayAnalysis = da;
}

void Capture::display(){
    unsigned int frameNum = 0;
    cv::Mat currentFrame, resized;
    while(1){
        if(!read(currentFrame)) break;
        if(frameNum > 2 && !getWindowProperty(capName, cv::WND_PROP_VISIBLE)) break; // close the window
        if(stopSignalReceived) break;
        cv::resize(currentFrame, resized, cv::Size((int)(ratio*400), 400));
        cv::namedWindow(capName, cv::WINDOW_NORMAL);
        imshow(capName, resized);
        cv::waitKey(1);
        frameNum++;
    }
}

void Capture::FrameDiffAreaOnly(){
    printf("FrameDiffAreaOnly thread ID: %d Name: %s\n",std::this_thread::get_id(), capName.c_str());
    cv::Mat croppedFrame, previousFrame, originalFrame, currDiffFrame;
    while(isOpened()){
        active = true;
        if(!read(originalFrame))break;
        
        // Copy the original frame
        croppedFrame = originalFrame.clone();
        preProcessing(&croppedFrame);

        // Check if a stop signal has arrived
        if(stopSignalReceived){
            readyToRetrieve = true;
            break;
        }
        
        if(processedFrameNum + 1) {
            frameDifferencing(&currDiffFrame, &previousFrame, &croppedFrame);
            std::vector<std::vector<cv::Point>> contours;
            findContours(currDiffFrame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            double tmpArea = 0, s = 0;
            int n = contours.size();
            //Check whether contours.size is greater than 0 before performing the calculation
            if(n > 0){
                tmpArea = getArea(contours);
                s = tmpArea*weight; // calculate the weighted score
            }
            //acquire lock
            std::unique_lock lk(mx);
            condVar.wait(lk, [this] {return !readyToRetrieve;});
            
            score = s; // update the score
            area = tmpArea; // update the area

            frame.release();
            frame = originalFrame.clone(); // update the frame
            readyToRetrieve = true;
            // Unlock and notify
            lk.unlock();
            condVar.notify_one();
        }
        previousFrame.release();
        previousFrame = croppedFrame.clone(); // Save the previous frame
        croppedFrame.release();
        currDiffFrame.release();
        originalFrame.release();
        ++processedFrameNum;
    }
    active = false;
    condVar.notify_one();
}

void Capture::FrameDiffAreaAndVel(){
    printf("FrameDiffAreaAndVel thread ID: %d Name: %s\n",std::this_thread::get_id(), capName.c_str());
    cv::Mat croppedFrame, previousFrame, originalFrame, currDiffFrame;
    while(isOpened()){
        active = true;
        if(!read(originalFrame))break;
        
        // Copy the original frame
        croppedFrame = originalFrame.clone();
        preProcessing(&croppedFrame);
        
        // Check if a stop signal has arrived
        if(stopSignalReceived){
            readyToRetrieve = true;
            break;
        }

        if(processedFrameNum + 1) {
            frameDifferencing(&currDiffFrame, &previousFrame, &croppedFrame);
            //Calculate the score of the frame
            std::vector<std::vector<cv::Point>> contours;
            findContours(currDiffFrame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            double tmpArea = 0, avgVel = 0, s = 0;
            int n = contours.size();
            //Check whether contours.size is greater than 0 before performing the calculation
            if(n > 0){
                tmpArea = getArea(contours);
                avgVel = getAvgSpeed(croppedFrame, previousFrame, contours);
                double nalpha = std::pow(n, alpha); // n to the power of alpha
                s = tmpArea*avgVel*weight*nalpha; // calculate the weighted score
            }

            //acquire lock
            std::unique_lock lk(mx);
            condVar.wait(lk, [this] {return !readyToRetrieve;});
            
            score = s; // update the score
            area = tmpArea; // update the area
            vel = avgVel; // update the speed
            area_n = n; // update areas number
            if(isdisplayAnalysis) displayAnalysis(currDiffFrame, croppedFrame, contours, tmpArea, avgVel); 
            frame.release();
            frame = originalFrame.clone(); // update the frame
            readyToRetrieve = true;
            // Unlock and notify
            lk.unlock();
            condVar.notify_one();
        }

        previousFrame.release();
        previousFrame = croppedFrame.clone(); // Save the previous frame
        croppedFrame.release();
        currDiffFrame.release();
        originalFrame.release();
        ++processedFrameNum;
    }
    active = false;
    condVar.notify_one(); // To unlock the scene while loop
}

void Capture::grabFrame(){
    printf("Grab thread ID: %d Name: %s\n",std::this_thread::get_id(), capName.c_str());
    cv::Mat grabbedFrame;
    while(isOpened()){
        active = true;
        if(!read(grabbedFrame))break;
        
        // Check if a stop signal has arrived
        if(stopSignalReceived){
            readyToRetrieve = true;
            break;
        }

        //acquire lock
        std::unique_lock lk(mx);
        condVar.wait(lk, [this] {return !readyToRetrieve;});
        
        frame.release();
        frame = grabbedFrame.clone(); // update the frame
        readyToRetrieve = true;
        // Unlock and notify
        lk.unlock();
        condVar.notify_one();
        grabbedFrame.release();
        ++processedFrameNum;
    }
    active = false;
    condVar.notify_one(); // To unlock the scene while loop
}

void Capture::preProcessing(cv::Mat* f)const{
     // Crop the frame in order to consider just the playground
    (*f) = (*f)(cv::Range(cropCoords[0], cropCoords[1]), cv::Range(cropCoords[2], cropCoords[3]));

    //Resize the frame for faster analysis
    cv::resize(*f, *f, cv::Size(150, (f->rows/(double)f->cols)*150), cv::INTER_AREA);
    
    // Pre-processing
    cvtColor(*f, *f, cv::COLOR_BGR2GRAY); //gray scale
    GaussianBlur(*f, *f, cv::Size(5,5), 0.3); //gussian blur
}

void Capture::frameDifferencing(cv::Mat* dst, cv::Mat* f1, cv::Mat* f2)const{
    cv::absdiff(*f1, *f2, *dst);
    // make the areas bigger
    dilate(*dst, *dst, cv::getStructuringElement( cv::MORPH_ELLIPSE,
                cv::Size( 2*DILATE_SIZE + 1, 2*DILATE_SIZE+1 ),
                cv::Point(DILATE_SIZE, DILATE_SIZE))); // DILATE_SIZE depends on the resolution of the frame
    //Threshold (black and white)
    threshold(*dst, *dst, 20, 255, cv::THRESH_BINARY);
}

double Capture::getArea(const std::vector<std::vector<cv::Point>>& contours){
    // Calculate the area
    double totalArea = 0;
    for(int i = 0; i < contours.size(); i++){
        double a = contourArea(contours[i]);
        if (a < 10){ // Do not include obj with a small area
            continue;
        }
        totalArea += a;
    }
    return totalArea;
}

double Capture::getAvgSpeed(const cv::Mat& currFrameGray, const cv::Mat& prevFrameGray, const std::vector<std::vector<cv::Point>>& contours){

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
    double speedSum = 0;
    for(int i = 0; i < mc.size(); i++){
        // Select good points
        if(status[i] == 1) {
            good++;
            cv::Point2f diff = mc[i] - calcPoints[i];    
            speedSum += cv::sqrt((diff.x*diff.x) + (diff.y*diff.y));
            good_new.push_back(calcPoints[i]);
        }
    }
    return (speedSum/good)*100; // *100 to avoid sub 1 values
}

void Capture::displayAnalysis(const cv::Mat& diffFrame, const cv::Mat& croppedFrame, const std::vector<std::vector<cv::Point>>& contours, const double area, const double avgVel){
    std::string winName = capName + " ANALYSIS - for DEBUGGING purposes ONLY";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);
    cv::waitKey(1);
    if(!getWindowProperty(winName, cv::WND_PROP_VISIBLE)) isdisplayAnalysis = false;
    else{
        
        // Concatenate the two frames 
        cv::Mat out;
        cv::hconcat(croppedFrame, diffFrame, out);
        cv::cvtColor(out, out, cv::COLOR_GRAY2BGR);
        drawContours(out, contours, -1, cv::Scalar(0, 255, 0), 1);
        cv::resize(out, out, cv::Size(1200, (out.rows/(double)out.cols)*1200));

        //Update values to display every 15 frames -> so you can read
        if(!(processedFrameNum%15)){
            paramToDisplay["FINAL_SCORE"] = std::to_string((int)std::floor(score));
            paramToDisplay["AREAS_NUM"] = std::to_string((int)contours.size());
            paramToDisplay["AVG_SPEED"] = std::to_string((int)avgVel);
            paramToDisplay["AREA"] = std::to_string((int)area);
            paramToDisplay["WEIGHT"] = std::to_string(weight);
        }
        // Insert some labels
        int i = 0;
        for(const auto& [key, val] : paramToDisplay){
            cv::putText(out, //target image
                key + ": " + val, //text
                cv::Point(5, 30*(1 + i++)),
                cv::FONT_HERSHEY_PLAIN,
                1.5,
                CV_RGB(0, 0, 255), //font color
                1);
        }
        if(processedFrameNum == 0)analysisOut = cv::VideoWriter("../out/" + capName + "_Analysis.mp4", cv::VideoWriter::fourcc('m','p','4','v'),25, cv::Size(out.cols, out.rows));
        analysisOut.write(out);
        cv::imshow(winName, out);
    }
}