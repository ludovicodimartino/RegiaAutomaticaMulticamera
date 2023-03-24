# OpenCV configurations for C++ on VS Code (Windows)

- Download VS Code from this [link](https://code.visualstudio.com/).
- Inside VS Code install the C/C++ extension from Microsoft.
- Download and install in the C: folder [MinGW](https://www.msys2.org/).
- Download [OpenCV](https://github.com/huihut/OpenCV-MinGW-Build) and install it in the C: folder.
- Add the MinGW bin folder to the Path evironment variable (*e.g. C:\msys64\mingw64\bin*)
- Add the OpenCV bin folder to the Path environment variable (*e.g. C:\OpenCV-MinGW-Build\x64\mingw\bin*)
- Restart the computer.
- In VS code open a folder and create a *.cpp* file.
- At the top right of the screen click on the gear to open the settings, a *launch.json* file will be opened. 
- Delete the files in the *.vscode* folder and replace them with the followings:

## *launch.json*

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

## *tasks.json*

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

## *c_cpp_properties.json*

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