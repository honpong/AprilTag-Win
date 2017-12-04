slam/synthetic_data_generator

slam/synthetic_data_generator software renders synthetic images and writes the camera/object pose information to the file. It takes a mesh file as input, if there exists a texture, applies texture to it and animates the camera based on the given animation file. The main branch of the repo requires mesh parameter, while the usha/simulator branch could be used to test the rendering by projecting simple points and doesn't necessarily require a mesh. Spline interpolation is performed among the key frames to animate the camera. Additionally, usha/simulator supports animating another object (called as controller) in the scene. To animate the controller along with the camera, a separate animation file for the controller has to be inputted to the renderer.

Currently tested on Windows 10

Required

There are two dependencies to run the renderer,
•VTK 7.1.1
•OpenCV 3.2

Build

Build the project using CMAKE and add the dependencies.
To build for Windows using cmake, ensure to have the following environment variables set:
VTK_DIR which points to the VTKConfig.cmake and OpenCV_DIR which points to OpenCVConfig.cmake
