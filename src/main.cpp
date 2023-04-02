#include "scene.h"
#include <iostream>
#include <string>
#include <csignal>

// Mat cropFrame(Mat &uncuttedFrame){
//     Rect cropRect(TOP_CORNER_X, TOP_CORNER_Y, BOTTOM_CORNER_X, BOTTOM_CORNER_Y);
//     Mat croppedFrame = uncuttedFrame(cropRect);
//     return croppedFrame.clone();
// }

void signalHandler( int signum ) {
   std::cout << "Signal received by main" <<  std::endl;
   Capture::stopSignalReceived = true;
}

int main( int argc, char** argv ){
    //Scene init
    Scene scene(std::string("../scene.conf"));
    //Signals init
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGABRT, signalHandler);

    //scene.displayCaptures(TOP);

    //start the switching
    scene.cameraSwitch();
    //std::cout << scene << std::endl;
    return 0;
}