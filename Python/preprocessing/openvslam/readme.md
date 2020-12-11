
# Introduction

This code is tested on the code of the master branch of OpenVSLAM. 
The commit ID is 5919b408aff485b89a85710b9e2be29ccb7803ee.

For more information, please see the [OpenVSLAM official website](https://github.com/xdspacelab/openvslam).

## File list

```
.
├── image_util.cc
├── image_util.h
├── readme.md
├── run_camera_pose_reconstruction.cc
├── run_camera_pose_reconstruction_video.cc
├── run_images.sh
└── run_video.sh
```

- `*.h` and `*.cc` are patches of OpenVSLAM code;
- `run_image.sh` is used to reconstruct camera pose from an image sequence;
- `run_video.sh` is used to reconstruct camera pose from a video.

## Patch

The following patches are applied to the official [OpenVSLAM](https://github.com/xdspacelab/openvslam) source code:

1. Copy the files `image_util.cc` and `image_util.h` to the `example/util/` folder of the official code and replace the original files;

2. Copy the files `run_camera_pose_reconstruction.cc` and `run_camera_pose_reconstruction_video.cc` to the `example/` folder of the official code;

3. Patch `example/CMakeList.txt`, adding the following code below line 64: 

```
add_executable(run_camera_pose_reconstruction run_camera_pose_reconstruction.cc util/image_util.cc)
list(APPEND EXECUTABLE_TARGETS run_camera_pose_reconstruction)

add_executable(run_camera_pose_reconstruction_video run_camera_pose_reconstruction_video.cc util/image_util.cc)
list(APPEND EXECUTABLE_TARGETS run_camera_pose_reconstruction_video)
```

4. In `example/CMakeList.txt` (same file), add the additional link dependent lib `stdc++fs` at line 100:

```
    target_link_libraries(${EXECUTABLE_TARGET}
                          PRIVATE
                          ${PROJECT_NAME}
                          opencv_imgcodecs
                          opencv_videoio
                          stdc++fs)
```

## How to use

After applying the patches, build OpenVSLAM and change the variables in the `*.sh` script to the correct parameters.
