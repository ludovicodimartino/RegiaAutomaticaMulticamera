#include "scene.h"
#include <iostream>
#include <string>
#include <csignal>

void signalHandler(int signum);
void printHelpAndExit();
void printErrorMessage(std::string_view errMsg);

int main(int argc, char** argv){
    std::string config_path = "../scene.conf"; //default config path
    bool displayMode = false; // In display mode the program shows the input camera streams
    
    // Arguments parsing
    std::vector<std::string> args(argv, argv+argc);
    for(int i = 0; i < args.size(); i++){
        if(args[i] == "-h" || args[i] == "--help" || args[i] == "-H") printHelpAndExit();
        if(args[i] == "-c" || args[i] == "--config"){
            if(i + 1 == args.size()){
                printErrorMessage("No config file specified");
                exit(0);
            }
            config_path = args[i +1];
        }
        if(args[i] == "-d" || args[i] == "--display") displayMode = true;
    }

    //Scene init
    Scene scene(config_path);

    //Signals init
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGABRT, signalHandler);

    //start the camera switching or the camera display 
    if(displayMode) scene.displayCaptures(TOP);
    else scene.cameraSwitch();

    //std::cout << scene << std::endl;
    return 0;
}

void signalHandler( int signum ) {
   std::cout << "Signal received" <<  std::endl;
   Capture::stopSignalReceived = true;
}

void printHelpAndExit(){
    std::cout << "Usage\n\n";
    std::cout << "  MultiCamSwitch [options]\n\n";
    std::cout << "Options\n";
    std::cout << "  -c,--config <path-to-config-file>   = Explicitly specify the path to the configuration file." << std::endl;
    std::cout << "  -d,--display                        = display input strams. No camera switching." << std::endl;
    std::cout << "  -h,-H,--help                        = print usage information and exit." << std::endl;
    exit(0);
}

void printErrorMessage(std::string_view errMsg){
    std::cout << "ERROR: " << errMsg << std::endl;
    std::cout << "Run MultiCamSwitch --help for help" << std::endl;
}