cmake_minimum_required(VERSION 2.6)

include(CheckIncludeFile)
CHECK_INCLUDE_FILE(/opt/vc/include/bcm_host.h ISRASPBERRYPIE)

#if(ISRASPBERRYPIE)
OPTION(RASPBERRYPI_GLES11 "Compile for Raspberry PI GLES" ON)
#endif(ISRASPBERRYPIE)

#OPTION(RASPBERRYPI_OPENGL "Compile for Rasperry PI OpenGL" ON)


set(CMAKE_BUILD_TYPE "Release") #use Release or Debug

#for raspberry directory
#link_directories("/usr/lib/arm-linux-gnueabihf")

project(RTPaperCart)
include(../../shared/linux/Proton.cmake)
#include(../source)

proton_include_components(FocusInputComponent)
proton_include_components(CustomInputComponent ArcadeInputComponent)

set(APP "../source")
proton_set_sources("${APP}/App.cpp" "${APP}/RomUtils.cpp" "${PROTON_MANAGER}/OpenCVManager.cpp" "${PROTON_NETWORK}/NetUtils.cpp")

target_sources(RTPaperCart PUBLIC ${APP}/ConsoleManager.cpp)


#the QR stuff
target_sources(RTPaperCart PUBLIC ${PROTON_MANAGER}/QRGenerateManager.cpp ${PROTON_UTIL}/QR-Code-generator/cpp/BitBuffer.cpp ${PROTON_UTIL}/QR-Code-generator/cpp/QrCode.cpp ${PROTON_UTIL}/QR-Code-generator/cpp/QrSegment.cpp)


#if OpenCV is required...
#target_link_libraries(RTPaperCart opencv_core opencv_imgproc opencv_video opencv_videoio opencv_highgui opencv_imgcodecs)

target_link_libraries(RTPaperCart zbar raspicam mmal mmal_core mmal_util)
target_link_libraries(RTPaperCart)
