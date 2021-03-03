
cmake_minimum_required(VERSION 3.4.1)

add_library(
        native-lib
        SHARED
        EnjoyPlayer.cpp
        JavaCallHelper.cpp
        native-lib.cpp
        AudioChannel.cpp
        VideoChannel.cpp)

set(libs ${CMAKE_SOURCE_DIR}/${ANDROID_ABI})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${libs}")


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
