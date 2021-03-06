cmake_minimum_required(VERSION 3.13)

project(
  vlsdk_demo_for_gumprodig
  DESCRIPTION
    "Demonstrates usage of vlSDK for object location in a single-frame, multi-view setting."
  LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc)
find_package(vlSDK REQUIRED)

set(JSON_DIR "submodules/nlohmann")
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${JSON_DIR}/include)

set(MAIN_TARGET "TrackingDemoMain")
add_executable(
  ${MAIN_TARGET}
  Source/TrackingDemoMain.cpp Source/MultiViewDetector.cpp Source/Extrinsic.cpp
  Source/Helpers/ImageHelpers.cpp Source/Visualization/ResultVisualization.cpp)
target_include_directories(${MAIN_TARGET} PRIVATE Source)
target_link_libraries(${MAIN_TARGET} ${OpenCV_LIBS} vlSDK::vlSDK nlohmann_json)
target_compile_features(${MAIN_TARGET} PRIVATE cxx_std_17)

# For convenience. Adds the directories with visionLib and OpenCV DLLs to the
# PATH variable in Visual Studio's Debugger Environment.
if(MSVC_IDE)
  get_target_property(vlSDK_DLL vlSDK::vlSDK IMPORTED_LOCATION_RELEASE)
  get_filename_component(VL_BINARY_DIR ${vlSDK_DLL} DIRECTORY)
  message("Assuming vlSDK binaries are located at: ${VL_BINARY_DIR}")

  get_target_property(OpenCV_DLL opencv_world IMPORTED_LOCATION_RELEASE)
  get_filename_component(OpenCV_BINARY_DIR ${OpenCV_DLL} DIRECTORY)
  message("Assuming OpenCV binaries are located at: ${OCV_BINARY_DIR}")

  # Allows relative path's to work in Visual Studio
  set_target_properties(${MAIN_TARGET} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY
                                                  ${PROJECT_SOURCE_DIR})
  set_target_properties(
    ${MAIN_TARGET}
    PROPERTIES VS_DEBUGGER_ENVIRONMENT
               "PATH=${VL_BINARY_DIR};${OpenCV_BINARY_DIR};%PATH%")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT
                                                              ${MAIN_TARGET})
else()
  message(
    "Generator is not Visual Studio. Add bin directories to the PATH of your system or IDE manually"
  )
endif()
