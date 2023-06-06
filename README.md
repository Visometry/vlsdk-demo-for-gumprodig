# vlSDK demo for GumProDig

Demostrates how to use VisionLib for object detection in a multi-view setting, where each frame shows the object in an entirely random pose.

## Prerequisites

You will need the following software installed on your PC:
- [Git](https://git-scm.com/download/win)
- [CMake](https://cmake.org/download/)
- [Visual Studio](https://visualstudio.microsoft.com/de/downloads/) (Community Edition should be sufficient)

## Setup

### Git

Clone the repository from GitHub with the following command:

`git clone --recurse-submodules <address of this GitHub repo>`

### VisionLib

Download the current distribution of the [VisionLib Native SDK](https://visionlib.com/develop/downloads/).
The minimum required version is 2.3.
Extract the zipped content into the root folder of this repository. You should have two subfolders: vlSDK and LicenseTool.

If you do not run the demo in Visual Studio's debugger, add vlSDK's `bin` directory to you system's `PATH` manually.

<details><Summary> <i>Note for VisionLib Developers</i></Summary>

You can also use your own locally-built binaries of the VisionLib Native SDK for this demo. To this end:
- build the target `INSTALL` of the vlSDK project
- in CMAKE (of this demo), set the variable `vlSDK_DIR` to the `cmake` subdirectory of your self-built and installed vlSDK

In Visual Studio the Debugging-Environment will then automatically contain a path variable that points to the directory with the binaries of your `INSTALL`-ed vlSDK. This works for `Release`, `RelWithDebInfo`, and `Debug`configurations.

</details>


### OpenCV

Download and install [OpenCV](https://opencv.org/releases/) 

If you do not run the demo in Visual Studio's debugger, add OpenCV's `bin` directory to you system's `PATH` manually.

### License

To build and run the demo, a valid [license](https://docs.visionlib.com/v2.2.0/licensing.html) is required.

The default location is `Resources/license.xml`.

### CMake

Build this project with [CMake](https://cmake.org/download/).

We recommend using CMake GUI. 
If you want to use CMake from the command line, please consult the [CMake documentation](https://cmake.org/cmake/help/v3.2/manual/cmake.1.html).

In CMake GUI set "Where is the source code:" to the directory to which you cloned this repository.
Then set "Where to build the binaries:" to a folder of your choice. 

Add an entry named `vlSDK_DIR`, set it to the filepath to the `cmake` subdirectory in your vlSDK directory.
Alternatively add the entry `CMAKE_PREFIX_PATH` and set it to the filepath of your vlSDK directory.

Add an entry named `OpenCV_DIR`, set it to the filepath to OpenCV's `build` directory, where its cmake configuration file (`OpenCVConfig.cmake`) is located.

If you use Visual Studio as Generator (IDE), you can set or modify the default paths in cmake for the license, vl-file, and the test images directory (`VSRUNTIME_VLLICENSEPATH`, `DEFINED VSRUNTIME_VLFILE`,  `VSRUNTIME_IMAGEDIR`). This will add them as command argument is Visual Studio's Debugging Environment for convenience.

Click "Generate"

<details><Summary> <i>Note for VisionLib Developers</i></Summary>

If you have built the vlSDK from the repository, make sure that the `vlSDK_DIR` variable points to the `cmake`-subdirectory of your `INSTALL`-ed vlSDK. (see above)

You can also use the `**TESTING**`-alias for `VSRUNTIME_VLLICENSEPATH` instead of a file path as long as you are in Visometry's VPN. 

</details>

### Build and Run

Build and Run `TrackingDemoMain`. Running requires three command arguments: vl-file path, image sequence directory, and licence file path.

When running in VS-IDE those command arguments are set via cmake. Modify them either in cmake or in the project-settings of `TrackingDemoMain`.

## Tracking configuration

This demo assumes, that we have multiple cameras that all look at the same object from different angles. 
They simultaneously take a single image of the object, which can be in an entirely random pose at this point in time. 
Such a set of images - one by each camera - is called a frame.

Given a frame, our goal is to find the relative position between the object and our cameras.
We already know the camera intrisics and the relative positions between the cameras.

In this demo we create a tracker suited for this task from the tracking configuration file `Resources/bosch_injected.vl`.

<details><Summary> Here we explain the relevant components of the configuration in more detail. </Summary>
<br></br>

### Object model

A CAD model of the object. For details see https://docs.VisionLib.com/v2.2.0/vl_unity_s_d_k__preparing_models.html

_In our example_: `Resources/Rexroth-3_842_523_558_m.obj`

### Tracker

Specifies the type and the parameters of the tracking algorithm, the object model, the cameras to be used and the workspace.

_In our example_: `"tracker"->"parameters"` in `Resources/bosch_injected.vl`

Note that AutoInit is enabled, this tells the tracker to try and detect the object according to the workspace definition, if it has no prior information on the object's position.

### Workspace definition

Tells the tracking algorithm from which angles the camera's could potentially see the object. 

_In our example_: `"tracker"->"parameters"->"workspaceDefinition"` in `Resources/bosch_injected.vl`

The object orientation is entirely random so we have to take into account all angles (`"sphereThetaLength"=180, "spherePhiLength"=360`)

### Image input
Describes the source of the images in which we want to track the object, i.e. type of source and number of cameras. 
It also contains the camera calibration data to be used.

_In our example_: `"input"->"imageSources"` in `Resources/bosch_injected.vl`

The input device we use is called `"device0"` and is of type `"injectMultiView"`.

It contains one image for each camera selected in the tracker. The image corresponding to the first camera selected in the tracker has key `"injectImage_0"`, the image for the second camera `"injectImage_1"`...  

We can set these images directly from any image we have loaded in main memory, even after the tracker has been started.

### Camera calibration data
Intrinsic camera parameters but also the relative positions of the cameras to each other.

_In our example_: `"input"->"imageSources"->"data"->"cameras"` in `Resources/bosch_injected.vl`

For all cameras `r` and `t` describe the rotation (as quaternion) and translation relative to the (multi-view) camera coordinate system. 

In our example this coordinate system was selected, so that the first camera is at its origin (`r=[0, 0, 0, 1]   t=[0, 0, 0]`), but this is not required in general.

**Note:** Which cameras actually participate in the tracking is specified in `"tracker"->"parameters"->"trackingCameras"`. 
For example if `trackingCameras` were set to `[6, 2]` the tracker expects there will be two images in the input device: `"injectImage_0"` (captured by the 7th camera) and `"injectImage_1"` (captured by the 3rd camera).
</details>

## Detection output

The output we get from the tracker is a camera extrinsic. 
It is the euclidean transformation to get from the model coordinate system to the camera coordinate system.

Combining this transformation with the extrinsics from the camera calibration gives the position of the object relative to each camera.
Using the camera intrinsics we can then calculate the position of the object in each image.

## Implementation with vlSDK

The interactions with the VisionLib in order to run the detection are encapsulated in the class `MultiViewDetector`.

### MultiViewDetector()

The detector is created once and then used for all frames.
It's most important member is the `Worker` which contains all the nodes (tracker and input) and processes all the commands necessary for the tracking.

**Note:** We use a synchronous worker instance (created via `vlNew_SyncWorker`) for this demo.
Commands executed by such a worker block the calling thread until completion. VisionLib also offers asynchronous workers (created via `vlNew_Worker`), which work with a different set of API calls.

### runDetection()

This consisits of three steps:
1. **Reset the tracker** -
Normally the tracker assumes that consecutive frames belong to one image sequence (i.e. a video).
Therefore it limits it's search to poses that are similar to the pose in the last frame.
In this demo consecutive frames are completely independent from each other.
By resetting we tell the tracker to forget all information from previous frames and look for the object in all possible poses. 
2. **Inject the frame** - 
As described above, the `injectMultiView` device holds only one frame at a time.
In this step we set it to the new frame, thus overriding the previous one.
3. **Run tracking** - Detect the object in the frame that we injected in step 2.
4. **Return extrinsic** - The class `Extrinsic` handles the API calls to retrieve `t`, `r` and `valid` and memory deallocation.

## Visualization

This demo contains the option to visualize and inspect the detection output by drawing the detected model edges (returned by `getLineModelImages()`) over the actual image.
Set the flag `visualizeResults` in `TrackingDemoMain.cpp` to turn it on/off. 
