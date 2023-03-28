# OpenCV configurations for C++ on VS Code (Windows)

- Download VS Code from this [link](https://code.visualstudio.com/).
- Inside VS Code install the C/C++ extension from Microsoft.
- Download and install in the C: folder [MinGW](https://www.msys2.org/).
- Run the following command in the MSYS2 terminal to install the Mingw-w64 toolchain (accept the default options).
    
        pacman -S --needed base-devel mingw-w64-x86_64-toolchain

- Download [OpenCV](https://github.com/huihut/OpenCV-MinGW-Build) (zip) and exctract it in the C: folder.
- Rename the extracted folder as *OpenCV-MinGW-Build*.
- Add the MinGW bin folder to the Path evironment variable (*e.g. C:\msys64\mingw64\bin*)
- Add the OpenCV bin folder to the Path environment variable (*e.g. C:\OpenCV-MinGW-Build\x64\mingw\bin*)
- Restart the computer.
- In VS code open a folder (workspace) and create a *.cpp* file.
- At the top right of the screen click on the gear to open the settings, a *launch.json* file will be opened. 
- Delete the files in the *.vscode* folder and replace them with the followings:

<details><summary>launch.json</summary>

    {
        "configurations": [
            {
                "name": "C/C++: gcc.exe compilare ed eseguire il debug del file attivo",
                "type": "cppdbg",
                "request": "launch",
                "program": "${fileDirname}\\out\\out.exe",
                "args": [],
                "stopAtEntry": false,
                "cwd": "${fileDirname}",
                "environment": [],
                "externalConsole": false,
                "MIMode": "gdb",
                "miDebuggerPath": "C:\\msys64\\mingw64\\bin\\gdb.exe",
                "setupCommands": [
                    {
                        "description": "Abilita la riformattazione per gdb",
                        "text": "-enable-pretty-printing",
                        "ignoreFailures": true
                    },
                    {
                        "description": "Imposta Versione Disassembly su Intel",
                        "text": "-gdb-set disassembly-flavor intel",
                        "ignoreFailures": true
                    }
                ],
                "preLaunchTask": "C/C++: gcc.exe compila il file attivo"
            },
            {
                "name": "C/C++: g++.exe compilare ed eseguire il debug del file attivo",
                "type": "cppdbg",
                "request": "launch",
                "program": "${workspaceFolder}\\out\\out.exe",
                "args": [],
                "stopAtEntry": false,
                "cwd": "${fileDirname}",
                "environment": [],
                "externalConsole": false,
                "MIMode": "gdb",
                "miDebuggerPath": "C:\\msys64\\mingw64\\bin\\gdb.exe",
                "setupCommands": [
                    {
                        "description": "Abilita la riformattazione per gdb",
                        "text": "-enable-pretty-printing",
                        "ignoreFailures": true
                    },
                    {
                        "description": "Imposta Versione Disassembly su Intel",
                        "text": "-gdb-set disassembly-flavor intel",
                        "ignoreFailures": true
                    }
                ],
                "preLaunchTask": "C/C++: g++.exe compila il file attivo"
            }
        ],
        "version": "2.0.0"
    }

</details>

<details><summary>tasks.json</summary>

    {
        "tasks": [
            {
                "type": "cppbuild",
                "label": "C/C++: gcc.exe compila il file attivo",
                "command": "C:\\msys64\\mingw64\\bin\\gcc.exe",
                "args": [
                    "-fdiagnostics-color=always",
                    "-g",
                    "${file}",
                    "-o",
                    "${fileDirname}\\${fileBasenameNoExtension}.exe"
                ],
                "options": {
                    "cwd": "${fileDirname}"
                },
                "problemMatcher": [
                    "$gcc"
                ],
                "group": "build",
                "detail": "Attività generata dal debugger."
            },
            {
                "type": "cppbuild",
                "label": "C/C++: g++.exe compila il file attivo",
                "command": "C:\\msys64\\mingw64\\bin\\g++.exe",
                "args": [
                    "-std=c++17",
                    "-fdiagnostics-color=always",
                    "-g",
                    "${workspaceFolder}\\src\\*.cpp",
                    "-o",
                    "${workspaceFolder}\\out\\out.exe",
                    "-IC:\\OpenCV-MinGW-Build\\include",
                    "-LC:\\OpenCV-MinGW-Build\\x64\\mingw\\bin",
                    "-llibopencv_calib3d455",
                    "-llibopencv_core455",
                    "-llibopencv_dnn455",
                    "-llibopencv_features2d455",
                    "-llibopencv_flann455",
                    "-llibopencv_highgui455",
                    "-llibopencv_imgcodecs455",
                    "-llibopencv_imgproc455",
                    "-llibopencv_ml455",
                    "-llibopencv_objdetect455",
                    "-llibopencv_photo455",
                    "-llibopencv_stitching455",
                    "-llibopencv_video455",
                    "-llibopencv_videoio455"
                ],
                "options": {
                    "cwd": "${fileDirname}"
                },
                "problemMatcher": [
                    "$gcc"
                ],
                "group": {
                    "kind": "build",
                    "isDefault": true
                },
                "detail": "Attività generata dal debugger."
            }
        ],
        "version": "2.0.0"
    }

</details>

<details><summary>c_cpp_properties.json</summary>

    {
        "configurations": [
            {
                "name": "Win32",
                "includePath": [
                    "${workspaceFolder}/**",
                    "C:\\OpenCV-MinGW-Build\\include"
                ],
                "defines": [
                    "_DEBUG",
                    "UNICODE",
                    "_UNICODE"
                ],
                "compilerPath": "C:\\msys64\\mingw64\\bin\\gcc.exe",
                "cStandard": "c11",
                "cppStandard": "c++17",
                "intelliSenseMode": "clang-x64"
            }
        ],
        "version": 4
    }

</details>
<br />

## Test configurations

### Ming-w64

To check that your Mingw-w64 tools are correctly installed and available, open a new Command Prompt and type:

    gcc --version
    g++ --version
    gdb --version

<br />

### Hello world project

- From a Windows command prompt, create an empty folder called projects where you can place all your VS Code projects. Then create a sub-folder called helloworld, navigate into it, and open VS Code in that folder by entering the following commands:

    mkdir projects
    cd projects
    mkdir helloworld
    cd helloworld
    code .

- Remember to replace the files in the .vscode folder with the ones provided above.

- In the File Explorer title bar, select the New File button and name the file helloworld.cpp.

- Now paste in this source code:

```
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main()
{
    vector<string> msg {"Hello", "C++", "World", "from", "VS Code", "and the C++ extension!"};

    for (const string& word : msg)
    {
        cout << word << " ";
    }
    cout << endl;
}
```

- Press the play button in the top right corner of the editor.

<br />

### OpenCV first project

- Replace the code in the hello world program with this source code:

```
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

int main(int argc, char** argv) {
    
    //create a gui window:
    namedWindow("Output",1);
    
    //initialize a 120X350 matrix of black pixels:
    Mat output = Mat::zeros( 120, 350, CV_8UC3 );
    
    //write text on the matrix:
    putText(output,
            "Hello World :)",
            cvPoint(15,70),
            FONT_HERSHEY_PLAIN,
            3,
            cvScalar(0,255,0),
            4);
    
    //display the image:
    imshow("Output", output);
    
    //wait for the user to press any key:
    waitKey(0);
    
    return 0;

}
```

- Press the play button in the top right corner of the editor.
