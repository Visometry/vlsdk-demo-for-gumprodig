cmake_minimum_required(VERSION 3.13)


project(vlsdk_demo_for_gumprodig DESCRIPTION "Demonstrates usage of vlSDK for object location in a single-frame, multi-view setting."
                                 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})


# find package OpenCV
IF((DEFINED OpenCV_DIR) AND (EXISTS "${OpenCV_DIR}"))
  message(STATUS "OpenCV_DIR is set!")
  find_package(OpenCV REQUIRED)
  IF (NOT OpenCV_FOUND)
    message(FATAL_ERROR "No OpenCV found at location ${OpenCV_DIR}. Please verify!")
    set(OpenCV_DIR "" CACHE PATH "Install directory of OpenCV" FORCE)
  ENDIF()
ELSE()
  set(OpenCV_DIR "" CACHE PATH "Install directory of OpenCV" FORCE)
  message(FATAL_ERROR "Please specify OpenCV_DIR!")
ENDIF()


# find package vlSDK
IF (NOT DEFINED vlSDK_INSTALL_PREFIX)
  message(STATUS "no 'vlSDK_INSTALL_PREFIX' variable is set. Checking default location.")
  set(vlSDK_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/vlSDK" CACHE PATH "CMake directory of vlSDK" FORCE)
  set(vlSDK_DIR ${vlSDK_INSTALL_PREFIX}/cmake)
ELSE()
  message(STATUS "vlSDK_INSTALL_PREFIX is defined as ${vlSDK_INSTALL_PREFIX}")
  set(vlSDK_DIR ${vlSDK_INSTALL_PREFIX}/cmake)
ENDIF()

IF((DEFINED vlSDK_DIR) AND (EXISTS "${vlSDK_DIR}"))
  find_package(vlSDK REQUIRED)
  IF (NOT vlSDK_FOUND)
    message(FATAL_ERROR "No vlSDK found at location ${vlSDK_DIR}. Please verify 'vlSDK_INSTALL_PREFIX'!")
    set(vlSDK_INSTALL_PREFIX "" CACHE PATH "CMake directory of vlSDK" FORCE)
  ELSE()
    message(STATUS "Found vlSDK-CMake-config at ${vlSDK_DIR}")
    if (TARGET_ARCH)
      set(vlSDK_BINARY_DIR "${vlSDK_INSTALL_PREFIX}/${TARGET_ARCH}/bin" CACHE PATH "Binary directory of vlSDK" FORCE)
    endif()
  ENDIF()
ELSE()
    message(FATAL_ERROR "Error in configuration, ${vlSDK_DIR} does not exist!")
ENDIF()


set(JSON_DIR "submodules/nlohmann")
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${JSON_DIR}/include)

set(HELPERS_DIR "Source/Helpers")
add_library(Helpers STATIC ${HELPERS_DIR}/HelperFunctions.h ${HELPERS_DIR}/HelperFunctions.cpp ${HELPERS_DIR}/PointerHandler.h ${HELPERS_DIR}/GetJsonCommandResult.h)
target_include_directories(Helpers PUBLIC ${HELPERS_DIR})
target_link_libraries(Helpers PUBLIC vlSDK::vlSDK nlohmann_json)

set(MAIN_TARGET "TrackingDemoMain")
add_executable(${MAIN_TARGET} Source/TrackingDemoMain.cpp)
target_compile_features(${MAIN_TARGET} PRIVATE cxx_std_17)
target_link_libraries(${MAIN_TARGET} ${OpenCV_LIBS} vlSDK::vlSDK nlohmann_json Helpers)

# For convenience. Adds the directories with visionLib and OpenCV DLLs to the PATH variable in Visual Studio's Debugger Environment.
set(OCV_BINARY_DIR ${OpenCV_DIR}/${OpenCV_ARCH}/${OpenCV_RUNTIME}/bin)
message("Assuming vlSDK binaries are located at: ${vlSDK_BINARY_DIR}")
message("Assuming OpenCV binaries are located at: ${OCV_BINARY_DIR}")
IF (MSVC_IDE)
  set_target_properties(${TARGETNAME} PROPERTIES VS_DEBUGGER_ENVIRONMENT "PATH=${vlSDK_BINARY_DIR};${OCV_BINARY_DIR};%PATH%")
  set_target_properties(${TARGETNAME} PROPERTIES VS_STARTUP_PROJECT ${MAIN_TARGET})
ELSE ()
  message("Generator is not Visual Studio. Add bin directories to the PATH of your system/IDE manually")
ENDIF()
