cmake_minimum_required(VERSION 3.13)

function(chooseAvailableImportedLocation targetName binaryFileLocation_Out)
  set(configs RELEASE RELWITHDEBINFO DEBUG)
  foreach (config ${configs})
    get_target_property(binaryFileLocation ${targetName} IMPORTED_LOCATION_${config})
    if(binaryFileLocation)
      message(STATUS "Found binary for ${targetName} in ${config} configuration")
      SET(${binaryFileLocation_Out} ${binaryFileLocation} PARENT_SCOPE)
      return()
    endif()
  endforeach()
endfunction()

function(getImportedLocationDir targetName binaryDirectory_Out)
  chooseAvailableImportedLocation(${targetName} binaryFileLocation)
  if(binaryFileLocation)
    get_filename_component(binaryDirectory ${binaryFileLocation} DIRECTORY)
  else()
    message(WARNING "No binaries found for ${targetName}. Set path manually!")
  endif()
  SET(${binaryDirectory_Out} ${binaryDirectory} PARENT_SCOPE)
endfunction()


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
  getImportedLocationDir(vlSDK::vlSDK VL_BINARY_DIR)
  message(STATUS "Adding vlSDK binary directory ${VL_BINARY_DIR} to path variable in Debugger environment")

  getImportedLocationDir(opencv_core OpenCV_BINARY_DIR)
  message(STATUS "Adding OpenCV binary directory ${OpenCV_BINARY_DIR} to path variable in Debugger environment")

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
