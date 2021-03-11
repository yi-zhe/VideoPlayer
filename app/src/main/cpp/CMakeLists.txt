
cmake_minimum_required(VERSION 3.4.1)

add_library(
        native-lib
        SHARED
        EnjoyPlayer.cpp
        JavaCallHelper.cpp
        native-lib.cpp
        AudioChannel.cpp
        VideoChannel.cpp
        VideoChannelWithX264.cpp
        AudioChannelWithFaac.cpp
        FaceTrack.cpp)

set(libs ${CMAKE_SOURCE_DIR}/${ANDROID_ABI})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${libs} -L${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI}")


include_directories(include)

target_link_libraries(
        native-lib
        avfilter
        avformat
        avcodec
        avutil
        swresample
        swscale
        rtmp
        OpenSLES
        android
        z
        x264
        faac
        opencv_java4
        SeetaFaceLandmarker
        log)
# 图像处理动态库
add_library(
        ImageUtils
        SHARED
        ImageUtils.cpp)

target_link_libraries(
        ImageUtils
        yuv
)
