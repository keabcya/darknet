# Yolo-v3 for Windows

More details: http://pjreddie.com/darknet/yolo/

[![License: Unlicense](https://img.shields.io/badge/license-Unlicense-blue.svg)](https://github.com/AlexeyAB/darknet/blob/master/LICENSE)  


* [Requirements (and how to install dependecies)](#requirements)

1. [How to use Yolo as DLL and SO libraries](#how-to-use-yolo-as-dll-and-so-libraries)
2. [How to use Camera for detection](#how-to-use-camera-for-detection)

## Requirements

* Windows
* **CUDA 10.0**: https://developer.nvidia.com/cuda-toolkit-archive (on Linux do [Post-installation Actions](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html#post-installation-actions))
* **OpenCV >= 2.4**: use your preferred package manager (brew, apt), build from source using [vcpkg](https://github.com/Microsoft/vcpkg) or download from [OpenCV official site](https://opencv.org/releases.html) (on Windows set system variable `OpenCV_DIR` = `C:\opencv\build` - where are the `include` and `x64` folders [image](https://user-images.githubusercontent.com/4096485/53249516-5130f480-36c9-11e9-8238-a6e82e48c6f2.png))
* **cuDNN >= 7.0 for CUDA 10.0** https://developer.nvidia.com/rdp/cudnn-archive 
  (on **Windows** copy `cudnn.h`,`cudnn64_7.dll`, `cudnn64_7.lib` as desribed here https://docs.nvidia.com/deeplearning/sdk/cudnn-install/index.html#installwindows )
* **GPU with CC >= 3.0**: https://en.wikipedia.org/wiki/CUDA#GPUs_supported

## How to use Yolo as DLL and SO libraries

* on Windows
    * build `darknet` using `cmake` or
    * compile `build\darknet\yolo_cpp_dll.sln` solution or `build\darknet\yolo_cpp_dll_no_gpu.sln` solution

There are 2 APIs:
* C API: https://github.com/AlexeyAB/darknet/blob/master/include/darknet.h
    * Python examples using the C API::     
         * https://github.com/AlexeyAB/darknet/blob/master/darknet.py	
         * https://github.com/AlexeyAB/darknet/blob/master/darknet_video.py
    
* C++ API: https://github.com/AlexeyAB/darknet/blob/master/include/yolo_v2_class.hpp
    * C++ example that uses C++ API: https://github.com/AlexeyAB/darknet/blob/master/src/yolo_console_dll.cpp
    
----

1. To compile Yolo as C++ DLL-file `yolo_cpp_dll.dll` - open the solution `build\darknet\yolo_cpp_dll.sln`, set **x64** and **Release**, and do the: Build -> Build yolo_cpp_dll
    * You should have installed **CUDA 10.0**
    * To use cuDNN do: (right click on project) -> properties -> C/C++ -> Preprocessor -> Preprocessor Definitions, and add at the beginning of line: `CUDNN;`

`yolo_cpp_dll.dll`-API: [link](https://github.com/AlexeyAB/darknet/blob/master/src/yolo_v2_class.hpp#L42)
```
struct bbox_t {
    unsigned int x, y, w, h;    // (x,y) - top-left corner, (w, h) - width & height of bounded box
    float prob;                    // confidence - probability that the object was found correctly
    unsigned int obj_id;        // class of object - from range [0, classes-1]
    unsigned int track_id;        // tracking id for video (0 - untracked, 1 - inf - tracked object)
    unsigned int frames_counter;// counter of frames on which the object was detected
};

class Detector {
public:
        Detector(std::string cfg_filename, std::string weight_filename, int gpu_id = 0);
        ~Detector();

        std::vector<bbox_t> detect(std::string image_filename, float thresh = 0.2, bool use_mean = false);
        std::vector<bbox_t> detect(image_t img, float thresh = 0.2, bool use_mean = false);
        static image_t load_image(std::string image_filename);
        static void free_image(image_t m);

#ifdef OPENCV
        std::vector<bbox_t> detect(cv::Mat mat, float thresh = 0.2, bool use_mean = false);
	std::shared_ptr<image_t> mat_to_image_resize(cv::Mat mat) const;
#endif
};
```

## How to use Camera for detection
1. You can run your console application from Windows Explorer `build\darknet\x64\yolo_console_dll.exe`
    
    ##### use this command:
    * source from video
        * `yolo_console_dll.exe data/coco.names yolov3.cfg yolov3.weights test.mp4`
    * Source from webcam
        * `yolo_console_dll.exe web_camera`
    * Source from Basler camera
        * `yolo_console_dll.exe basler_camera`
    
    * after launching your console application and entering the image file name - you will see info for each object: 
    `<obj_id> <left_x> <top_y> <width> <height> <probability>`
    * to use simple OpenCV-GUI you should uncomment line `//#define OPENCV` in `yolo_console_dll.cpp`-file: [link](https://github.com/AlexeyAB/darknet/blob/a6cbaeecde40f91ddc3ea09aa26a03ab5bbf8ba8/src/yolo_console_dll.cpp#L5)
    * you can see source code of simple example for detection on the video file: [link](https://github.com/AlexeyAB/darknet/blob/ab1c5f9e57b4175f29a6ef39e7e68987d3e98704/src/yolo_console_dll.cpp#L75)
  