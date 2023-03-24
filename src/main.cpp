#include "scene.h"
#include <iostream>
#include <string>

// Mat cropFrame(Mat &uncuttedFrame){
//     Rect cropRect(TOP_CORNER_X, TOP_CORNER_Y, BOTTOM_CORNER_X, BOTTOM_CORNER_Y);
//     Mat croppedFrame = uncuttedFrame(cropRect);
//     return croppedFrame.clone();
// }

int main( int argc, char** argv ){
    Scene scene(std::string("../scene.conf"));
    //scene.displayCaptures(TOP);
    scene.cameraSwitch();
    return 0;
}