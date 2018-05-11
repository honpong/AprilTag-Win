/********************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.

*********************************************************************************/

#ifndef SP_CORE_H
#define SP_CORE_H

#include "SP_ReconstructionTypes.h"
#include <stdint.h>

#if defined(_MSC_VER) && (defined(WIN32) || defined(WIN64))
# define DllExport   __declspec( dllexport )
#else
# define DllExport
#endif //#if defined(_MSC_VER) && (defined(WIN32) || defined(WIN64))

#ifdef __cplusplus
extern "C" {
#endif //_cplusplus

struct SP_Calibration;

/// <summary>
/// SP_setConfiguration sets configuration parameters for the scene perception
/// internal state. This includes camera information in addition to tracking
/// volume resolution.
/// Users MUST check return value for error before calling other functions.
/// </summary>
///
/// <paramref name="pCamParamRGB"> : Color camera intrinsics and image dimensions.
/// </paramref>
///
/// <paramref name="pCamParamDepth"> : Depth camera intrinsics and image dimensions.
/// </paramref>
///
/// <paramref name="extrinsics"> : Camera translation extrinsics.
/// </paramref>
///
/// <paramref name="resolution"> : resolution of the three dimensional reconstruction.
/// Possible values are:
/// SP_LOW_RESOLUTION use this resolution in a room-sized scenario (4/256m)
/// SP_MED_RESOLUTION use this resolution in a table-top-sized scenario (2/256m)
/// SP_HIGH_RESOLUTION use this resolution in a object-sized scenario (1/256m).
/// Choosing SP_HIGH_RESOLUTION in a room-size environment may degrade the tracking
/// robustness and quality. Choosing SP_LOW_RESOLUTION in an object-sized scenario
/// may result in a reconstructed model missing the fine details.
/// </paramref>
///
/// <param name="pRCconfiguration"> : configuration parameters that include calibration 
/// data for IMU sensors. </param>
/// <returns>Returns
/// SP_STATUS_ERROR: if failed to create a new internal state or the requested
/// configuration was not a valid one,
/// SP_STATUS_INVALIDARG: if arguments are not valid (e.g. not supported camera image
/// SP_STATUS_PLATFORM_NOT_SUPPORTED: if primary display driver doesn't support
/// OpenCL 1.2, and SP_STATUS_SUCCESS on success.
/// </returns>
DllExport SP_STATUS SP_setConfiguration
(
	SP_CameraIntrinsics* pCamParamDepth,
	SP_CameraIntrinsics* pCamParamRGB,
	SP_Resolution resolution
);

/// <summary>
/// SP_getCameraPose provides access to the camera's latest pose.
/// Latest pose is determined as corresponding to the last depth image data
/// that was successfully tracked.
/// </summary>
///
/// <param name="pPose"> : pointer to a pre-allocated array of 12 floats to
/// store camera pose. Camera pose is specified in a 3 by 4 matrix [R | T]
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is the translation vector.
/// Camera pose is stored the a 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz] </param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : when pPose is updated with the most current pose.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.
/// </returns>
DllExport SP_STATUS SP_getCameraPose
(
    float(*pPose)[12]
);

/// <summary>
/// SP_setCameraPose sets the system to a camera pose.
/// </summary>
///
/// <param name="pPose"> : pointer to a pre-allocated array of 12 floats
/// that stores a camera pose. Camera pose is specified in a 3 by 4 matrix
/// [R | T], where
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is the translation vector.
/// Camera pose is stored in the 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz] </param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : camera pose is successfully updated to the given
/// pose.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.
/// </returns>
DllExport SP_STATUS SP_setCameraPose
(
    const float(*pPose)[12]
);

/// <summary>
/// SP_doTracking performs tracking of camera position based on latest
/// depth data input, rgb data input (if available) and reconstructs a 3D
/// volumetric representation based on depth inputs. If tracking is failed
/// on current depth data, volume data and camera pose are not updated.
/// </summary>
///
/// <param name="pTrackingStatus "> : Used to return the tracking status. Can take
/// one of the following values:
/// HIGH: when tracking is successful and of high accuracy. Also, if
/// doSceneReconstruction is specified, the current depth input
/// might get integrated into the current reconstruction volume.
/// MED: when tracking is successful and of med accuracy.
/// LOW : when tracking is successful and of low accuracy.
/// FAILED: If tracking fails.</param>
///
/// <param name="pInputStream"> : structure containing pointers refering to
/// the current input depth and rgb images, and the IMU input.
/// </param>
///
/// <param name="pSegmentationParameters"> : optional structure containing parameters
/// for segmentation of the depth into foreground and background.
/// pSegmentationParameters.minForegroundDepthDifference must be greater than 0 and
/// is the distance in front of the reconstructed surface where depth pixels start to
/// be classified as foreground. Set pSegmentationParameters.isPercentDepthDifference
/// false to have an absolute threshold distance. Note that an absolute threshold may
/// cause more far-away pixels to be classified as foreground due to camera accuracy
/// and noise. Consider setting true when working with surfaces at a distance,
/// so that the threshold is a percentage of the depth at that reconstructed pixel,
/// hence the actual threshold distance will increase with increasing depth from the
/// camera. Note that some objects close to background surfaces may be classified
/// </param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : camera pose is successfully updated to the given
/// pose.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.
/// </returns>
///
/// \see SP_setConfiguration \see SP_init
DllExport SP_STATUS SP_doTracking (
    SP_TRACKING_ACCURACY * pTrackingStatus,
    const SP_InputStream * pInputStream);

/// <summary>
/// SP_doReconstruction sets the current pose and invokes internal
/// reconstruction using the input pose, depth and color image provided.
/// Note: the function should be used without tracking (SP_doTracking).
/// Calling this function while tracking is running will have severe impact on
/// the performance of tracking.
/// </summary>
///
/// <param name="pPose"> : pointer to a pre-allocated array of 12 floats
/// that stores a camera pose. Camera pose is specified in a 3 by 4 matrix
/// [R | T], where
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is the translation vector.
/// Camera pose is stored in the 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz] </param>
///
/// <param name="pInputStream"> : structure containing pointers refering to
/// the current input depth and rgb images, and the IMU input.
/// </param>
///
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : when reconstruction is successful on the current inputs.
/// SP_STATUS_ERROR : when there is an internal state error.
/// SP_STATUS_INVALIDARG : when there is an invalid argument.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.</returns>
///
/// \see SP_setConfiguration \see SP_init
DllExport SP_STATUS SP_doReconstruction
(
    const float(*pPose)[12],
    const SP_InputStream * pInputStream
);

/// <summary>
/// SP_setDoSceneReconstruction sets the internal state of whether to integrate
/// depth data into the 3D volume representation or use existing volumetric data
/// for tracking leaving it unaffected by new depth.
/// </summary>
///
/// <param name="doSceneReconstruction"> : When non-zero, depth is integrated
/// otherwise it is not.</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : when tracking is successful on the current inputs.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.</returns>
///
/// \see SP_setConfiguration \see SP_init
DllExport SP_STATUS SP_setDoSceneReconstruction
(
    int doSceneReconstruction
);

DllExport SP_STATUS SP_GetDepthTrackerErr(float* pfErr);

DllExport SP_STATUS SP_GetDepthTrackerInlierRatio(float* pfInlierRatio);

DllExport SP_STATUS SP_GetDepthTrackerFillRate(float* pfRatio);


/// <summary>
/// SP_getInternalCameraIntrinsics gets the camera intrinsics used internally and
/// corresponding to returned output images from API calls such as:
/// SP_doRenderVolume, SP_getVerticesImage, and SP_getNormalsImage.
/// </summary>
///
/// <param name="pCamParam"> : Output parameter for camera intrinsics .</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : when values are found and returned correctly
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_INVALIDARG : if pCamParam is null.</returns>
///
/// \see SP_setConfiguration \see SP_init
DllExport SP_STATUS SP_getInternalCameraIntrinsics
(
    SP_CameraIntrinsics* pCamParam
);


/// <summary>
/// Render the reconstructed volume from a given camera pose by ray-casting.
/// The result is an RGBA image in addition to the vertices and normals images 
/// used to produce it. Currently, the resulting image has a fixed
/// size of 320 x 240.
/// </summary>
///
/// <param name="pPose"> : pointer to a pre-allocated array of 12 floats
/// that stores a camera pose. Camera pose is specified in a 3 by 4 matrix
/// [R | T], where
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is the translation vector.
/// Camera pose is stored in the 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz] </param>
///
/// <param name="pImageRenderedVolume"> pointer to a pre-allocated array
/// whose content will get updated by the result of volume rendering. The
/// size of array in bytes is 4 * imageWidth * imageHeight, where
/// imageWidth and imageHeight are specified via SP_CameraIntrinsics
/// parameter passed into SP_setConfiguration.</param>
///
/// <param name="pVerticesImage"> pointer to a pre-allocated array of
/// floats to store vertices. Each vertex is a vector of x, y and z
/// components.
/// The image size in pixels must be equal to that returned by SP_getInternalCameraIntrinsics and hence the array
/// size in bytes is calculated as:
/// size = 3 x (float's byte size) x (pCamParam.imageWidth x pCamParam.imageHeight)</param>
///
/// <param name="pNormalsImage"> : pointer to a pre-allocated array of
/// floats to store normal vectors. Each normal is a vector of x, y and z
/// components.
/// The image size in pixels must be equal to that returned by SP_getInternalCameraIntrinsics and hence the array
/// size in bytes is calculated as:
/// size = 3 x (float's byte size) x (pCamParam.imageWidth x pCamParam.imageHeight)</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : when the rendering is successful and result is
/// written to the argument pImageRenderedVolume.
/// SP_STATUS_ERROR: when the rendering has failed and result is not
/// obtained.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.
/// SP_STATUS_INVALIDARG: when there is an invalid argument, e.g. incorrect
/// pose argument or invalid pointer for pImageRenderedVolume.
/// </returns>
/// \see SP_setConfiguration
DllExport SP_STATUS SP_doRenderVolume
(
    const float *pPose,
    unsigned char *pImageRenderedVolume,
	float *pVerticesImage,
	float *pNormalsImage,
	SP_CameraIntrinsics* intrinsics
);

/// <summary>
/// SP_init initializes the reconstructed model with the provided input image.
/// It also sets the camera pose to the pose provided or a default initial pose.
/// </summary>
///
/// <param name="pInitPose"> : pointer to a pre-allocated array of 12 floats
/// that stores a camera pose. Camera pose is specified in a 3 by 4 matrix
/// [R | T], where
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is the translation vector.
/// Camera pose is stored in the 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz] </param>
///
/// <param name="pDepthSrc"> : pointer to a depth source data.</param>
///
/// <param name="pRGBSrc"> : pointer to a RGB source data.</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: when the initialization succeeds.
/// SP_STATUS_ERROR: when there is an internal state error (failed initialization).
/// SP_STATUS_INVALIDARG: when there is an invalid argument.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration. </returns>
///
/// \see SP_setConfiguration
DllExport SP_STATUS SP_init
(
    const float(*pInitPose)[12],
    const SP_InputStream* pInputStream
);

/// <summary>
/// SP_reset removes all reconstructed model (volume) information and
/// reinitializes the model with the provided depth and RGB images or the
/// last input depth and RGB images.
/// It also resets the camera pose to the one provided or a default initial
/// pose.</summary>
///
/// <param name="pResetPose"> : pointer to a pre-allocated array of 12 floats
/// that stores a camera pose. Camera pose is specified in a 3 by 4 matrix
/// [R | T], where
/// R = [ r11 r12 r13 ]
///     [ r21 r22 r23 ]
///     [ r31 r32 r33 ]
/// is a rotation matrix and
/// T = [tx ty tz], is the translation vector.
/// Camera pose is stored in the 12-element array in row-major order:
/// [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz] </param>
///
/// <param name="pDepthSrc"> : pointer to a depth source data.</param>
///
/// <param name="pRGBSrc"> : pointer to an RGB source data.</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: when the reset succeeds.
/// SP_STATUS_ERROR: when there is an internal state error.
/// SP_STATUS_INVALIDARG: when there is an invalid argument.
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration. </returns>
///
/// \see SP_setConfiguration
DllExport SP_STATUS SP_reset
(
    const float(*pResetPose)[12] = NULL,
    const SP_InputStream* pInputStream = NULL
);

/// <summary>
/// SP_setMeshingUpdateThresholds sets the thresholds indicating the magnitude
/// of changes occurring in any block that would be considered significant for
/// re-meshing.
/// </summary>
///
/// <param name="maxDiffThreshold"> : If the maximum change in a block exceeds
/// this value, then the block will be re-meshed. Setting the value to zero will
/// retrieve all blocks.</paramref>
///
/// <param name="aveDiffThreshold"> : If the average change in a block exceeds
/// this value, then the block will be re-meshed. Setting the value to zero will
/// retrieve all blocks.</paramref>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS : on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration. </returns>
DllExport SP_STATUS SP_setMeshingUpdateThresholds
(
    float maxDiffThreshold,
    float aveDiffThreshold
);

/// <summary>
/// SP_isReconstructionUpdated indicates if the reconstruction of the scene has been
/// modified since the last call to SP_doMeshingUpdate.
/// </summary>
///
/// <returns>
/// Returns 1 if reconstruction was modified and 0 otherwise.
/// </returns>
DllExport int SP_isReconstructionUpdated();

/// <summary>
/// SP_doMeshingUpdate gets mesh information, performs meshing, and hole
/// filling if requested.
/// </summary>
///
/// <param name="bmui"> On call to this function, the members m_iNumBlockMeshes,
/// m_iNumVertices, and m_iNumFaces need to be set to the maximum number of
/// block meshes, vertices, and faces to be retrieved respectively. The array members
/// m_blockMeshes, m_fVertices, m_uColors, and m_iFaces must be pre-allocated
/// according to the corresponding maximum sizes. However, m_blockMeshes and m_uColors
/// can be set to nullptr if the corresponding information is not needed. The boolean
/// member m_bFillHoles indicates if the user needs hole filling applied for
/// the current meshing call. Hole filling will take several seconds to
/// complete. It is also limited to the region of space currently affected by
/// reconstruction. If meshing thresholds are non-zero hole filling can cause
/// discrepancies with data previously retrienved by the user.
///
/// On return the members m_iNumBlockMeshes, m_iNumVertices, and m_iNumFaces
/// will be set to the number of BlobkMeshes, vertices, and faces found. The
/// lists pointed to by m_blockMeshes, m_fVertices, m_uColors, and m_iFaces
/// will be filled with the needed data respectively. Each vertex in
/// m_fVertices is a 4 tuple: 3 floats for the vertex coordinates and the
/// fourth for a confidence metric in the range [0.0, 1.0]. The list m_iFaces
/// contains faces each represented by the 3 integer indices of the face forming
/// vertices.
///
/// This call is designed to be thread safe if called in parallel with
/// SP_doTracking. It does not require a separate thread if the user does not
/// wish to but we thought to allow it for better tracking performance while
/// meshing.
///
/// Note: The output of this function depends on values set by
/// SP_setMeshingUpdateThresholds.
/// </param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success
/// SP_STATUS_INVALIDARG: when the vertices or faces arrays are not allocated
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration.
/// SP_STATUS_WARNING: if meshing was terminated due to vertices or faces
/// exceeding the limit sizes.
/// SP_STATUS_ERROR: on internal errors.
/// </returns>
DllExport SP_STATUS SP_doMeshingUpdate
(
    SP_BlockMeshingUpdateInfo* bmui
);

/// Release the scene perception state and resources
DllExport void SP_release();

/// <summary>
/// SP_saveMesh saves a mesh from the volume in an ASCII obj file.
/// This call is designed to be thread safe if called in parallel with SP_doTracking or
/// SP_doMeshingUpdate. The resolution of the mesh saved is SP_HIGH_MESHRESOLUTION.
/// </summary>
///
/// <paramref name="pFileName"> the path of the file to use for saving the
/// mesh</paramref>
/// <paramref name="pMeshinfo"> Mesh saving info structure to indicate the resolution required (pMeshinfo->m_res),
/// hole filling option (pMeshinfo->m_bFillHole, 0 -> wo hole filling, and 1 -> w hole filling), and to mesh with
/// color information or not (pMeshinfo->m_bMeshWithColor, 0 -> wo color, 1 -> w color). Can not be null.
/// </paramref>
/// <paramref name="bNotOpenCVCoords"> Flag to indicate if OpenCV coordinates
/// not used. </paramref>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_ERROR: on internal errors (e.g. failed memory allocation).
/// </returns>
DllExport SP_STATUS SP_saveMesh
(
    const char *pFileName,
    SP_MeshingSavingInfo* pMeshinfo,
    int bNotOpenCVCoords
);

/// <summary>
/// SP_exportSurfaceVoxels exports the centers of the voxels intersected
/// by the surface scanned. The voxel size is set based on the resolution passed
/// to the SP_setConfiguration call.
///
/// </summary>
///
/// <paramref name="fVertices"> Pointer to float 3 tuples of voxels centers
/// coordinates. The user needs to allocate the array prior to calling this
/// functions. If fVertices is null only the number of voxels is returned.</paramref>
///
/// <paramref name="uColors">  Pointer to list of RGB colors corresponding to the
/// vertices (3 uchar per vertex). Set to null if colors not needed.</paramref>
///
/// <paramref name="numVoxels"> On input this should be set to maximum number of
/// float 3 tuples to be retrieved. On exit it will contain the actual
/// number of voxels found. If the output value is larger than the input value
/// a warning is returned indicating not all voxels were exported to the list.
/// </paramref>
///
/// <paramref name="boundToBox"> Bool flag indicating if voxels found to be
/// restricted to a bounding box. </paramref>
///
/// <paramref name="boundingBox"> Bounding box min and max coordinates
/// respectively.</paramref>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_WARNING: more voxels exist exceeding limit set.
/// SP_STATUS_ERROR: on internal errors.</returns>
DllExport SP_STATUS SP_exportSurfaceVoxels
(
    float* fVertices,
    unsigned char* uColors,
    int* numVoxels,
    int boundToBox,
    float boundingBox[6]
);

/// <summary>
/// SP_saveCurrentState saves the current scene perception's state to a
/// file. A later call to SP_loadState will restore Scene Perception to
/// the saved state.
/// </summary>
///
/// <paramref name="pFileName"> the path of the file to use for saving the
/// scene perception state</paramref>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_ERROR: on internal errors (e.g. file write permission).</returns>
DllExport SP_STATUS SP_saveCurrentState
(
    const char* pFileName
);

/// <summary>
/// SP_loadState loads a previous state from a file, configures and
/// initializes Scene Perception accordingly. The file should be produced
/// from a prior call to SP_saveCurrentState. This call need not be preceded
/// by a call to SP_setConfiguration since it loads the configuration from
/// from the file. Hence, input stream data sent to SP_doTracking must have
/// properties similar to those saved in the file. Also, SP_reset need not
/// be called since it is internally invoked based on the loaded state which
/// also includes the pose.
/// If any of SP_setConfiguration SP_init or SP_reset is called immediately
/// after SP_loadState, the Scene Perception's internal state will no
/// longer correspond to the state captured previously in the file.
///
/// Note: The camera settings used when saving the state file should be
/// identical to the camera settings used when loading the state from the
/// file. Otherwise, an error is returned.
/// </summary>
///
/// <paramref name="pFileName"> the path of the file to load from the scene
/// perception state</paramref>
/// <paramref name="pCamParam">   </paramref>

/// <paramref name="pCamParamRGB"> : Color camera intrinsics and image dimensions.
/// This parameter is tested against the camera intrinsics loaded from the file.
/// If they don't match a warning is returned. On exit this parameter value is set
/// to the camera intrinsics in the file.
/// </paramref>
///
/// <paramref name="pCamParamDepth"> : Depth camera intrinsics and image dimensions.
/// This parameter is tested against the camera intrinsics loaded from the file.
/// If they don't match a warning is returned. On exit this parameter value is set
/// to the camera intrinsics in the file.
/// </paramref>
///
/// <paramref name="extrinsics"> : Camera translation extrinsics.
/// </paramref>
///
/// <paramref name="resolution"> this parameter is used to return the
/// resolution found in the loaded file.</paramref>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_WARNING: on unmatched camera settings
/// SP_STATUS_ERROR: on internal errors.</returns>
DllExport SP_STATUS SP_loadState
(
    const char* pFileName,
    SP_CameraIntrinsics* pCamParamRGB,
    SP_CameraIntrinsics* pCamParamDepth,
    float* extrinsics,
    SP_Resolution* pResolution,
	const int colorFPS = 30
);

/// <summary>
/// SP_getSceneQuality determines whether the input image is suitable for tracking
/// before tracking is started.
/// </summary>
///
/// <param name="pDepthSrc"> pointer to a depth source data (required).
/// </param>
///
/// <param name="pRGBSrc"> pointer to a RGB source data (optional NULL).
/// </param>
///
/// <param name="pGravity"> a float array that contains x, y, z component
/// values of the most recent NORMALIZED gravity vector (optional NULL).
/// </param>
///
/// <param name="bUsePlaneAssessment"> an int value if set true, the function will
/// also assess planes in the scene to determine scene quality or not otherwise.
/// By default, the value is true.
/// </param>
///
/// <returns>Returns quality value in the range [0.0, 1.0] where:
/// 1.0  -> represents ideal images for track.
/// 0.0  -> represents image that are unsuitable to track.
/// -1.0 -> represents a scene without enough structure
/// -2.0 -> represents a scene without enough depth pixels
/// Also, value 0.0 is returned on missing internal configuration.
/// </returns>
DllExport float SP_getSceneQuality
(
    const unsigned short *pDepthSrc,
    const unsigned char *pRGBSrc = NULL,
    const float(*pGravity)[3] = NULL,
    int bUsePlaneAssessment = 1
);

/// <summary>
/// SP_fillDepthImage fills a depth image.
/// </summary>
///
/// <paramref name="depthImage"> the depth image to be filled. The dimensions of
/// the image should be identical to the depth image dimensions passed to
/// SP_setConfiguration. The zero depth values will be filled. </paramref>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_INVALIDARG: if pointer was not properly allocated.
/// </returns>
DllExport SP_STATUS SP_fillDepthImage
(
    unsigned short *pDepthImage
);

/// <summary>
/// SP_getNormalsImage provides access to the normals of surface points
/// that are within view from the camera's current pose.
/// </summary>
///
/// <param name="pNormalsImage"> : pointer to a pre-allocated array of
/// floats to store normal vectors. Each normal is a vector of x, y and z
/// components.
/// The image size in pixels must be equal to that returned by SP_getInternalCameraIntrinsics and hence the array
/// size in bytes is calculated as:
/// size = 3 x (float's byte size) x (pCamParam.imageWidth x pCamParam.imageHeight)</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_ERROR: on internal errors (e.g. pNormalsImage not properly
/// allocated)
/// SP_STATUS_INVALIDARG: if pNormalsImage is not allocated. </returns>
DllExport SP_STATUS SP_getNormalsImage
(
    float *pNormalsImage
);

/// <summary>
/// SP_getVerticesImage provides access to the surface's vertices
/// that are within view from camera's current pose.
/// </summary>
///
/// <param name="pVerticesImage"> : pointer to a pre-allocated array of
/// floats to store vertices. Each vertex is a vector of x, y and z
/// components.
/// The image size in pixels must be equal to that returned by SP_getInternalCameraIntrinsics and hence the array
/// size in bytes is calculated as:
/// size = 3 x (float's byte size) x (pCamParam.imageWidth x pCamParam.imageHeight)</param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_NOT_CONFIGURED: on missing internal configuration
/// SP_STATUS_ERROR: on internal errors (e.g. pVerticesImage not properly
/// allocated)
/// SP_STATUS_INVALIDARG : if pVerticesImage is not allocated. </returns>
DllExport SP_STATUS SP_getVerticesImage
(
    float *pVerticesImage
);

/// <summary>
/// SP_depthGen fills a depth image by stereo matching a pair of IR images. Only supports DS4 camera.
/// </summary>
///
/// <param name="pucLIR"> : pointer to the left IR image.
/// </param>
/// <param name="pucRIR"> : pointer to the right IR image.
/// </param>
/// <param name="pusSrcDepth">: pointer to the source depth image.
/// </param>
/// <param name="pusDestDepth">: pointer to the destination depth image.
/// </param>
/// <param name="fBaseLine">: Base line between two IR cameras.
/// </param>
/// <param name="iW, iH">: width and height of the images, defaulted to 320 by 240.
/// </param>
/// <param name="pucConf">: confidence map indicating which pixel is from DS4(255) or software(0)
/// </param>
///
/// <returns>Returns
/// SP_STATUS_SUCCESS: on success,
/// SP_STATUS_ERROR: on internal errors (e.g. non-supported dimensions).
/// </returns>
DllExport SP_STATUS SP_depthGen
(
    unsigned char* pucLIR,
    unsigned char* pucRIR,
    const unsigned short* const pusSrcDepth,
    unsigned short*             pusDestDepth,
    float                       fBaseLine,
    int                         iW = 320,
    int                         iH = 240,
    unsigned char* pucConf = nullptr
);

/// <summary>
/// SP_validatePose allows user to check whether supplied pose array
/// forms a valid pose
/// </summary>
///
/// <param name="pose" type="input">: Array of 12 float that stores camera pose
/// in row-major order. Camera pose is specified in a
/// 3 by 4 matrix [R | T] = [Rotation Matrix | Translation Vector]
/// where R = [ r11 r12 r13 ]
/// [ r21 r22 r23 ]
/// [ r31 r32 r33 ]
/// T = [ tx  ty  tz  ]
/// Pose Array Layout = [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz]
/// Translation vector is in meters.</param>
///
/// <returns>
/// On Success, returns flag indicating whether the supplied array
/// forms valid pose.</returns>

DllExport int SP_validatePose(const float pose[12]);

/// <summary>
/// SP_createPoseFromRow Allows user to Create Pose from supplied gravity
/// </summary>
///
/// <param name="pose" type="output">: Array of 12 float where user wishes
/// to stores the ouput camera pose in row-major order.
/// Camera pose is represneted by a
/// 3 by 4 matrix [R | T] = [Rotation Matrix | Translation Vector]
/// where R = [ r11 r12 r13 ]
/// [ r21 r22 r23 ]
/// [ r31 r32 r33 ]
/// T = [ tx  ty  tz  ]
/// Pose Array Layout = [r11 r12 r13 tx r21 r22 r23 ty r31 r32 r33 tz]
/// Translation vector is in meters.</param>
///
/// <param name="rowValues" type="input"> : Pre-allocated float array of 3 elements
/// containing normalized gravity vector.</param>
///
/// <param name="translation" type="input"> : Optional Pre-allocated float array of 3 elements
/// containing translation vector. If NULL is specified then translation will be set to the
/// origin (0, 0, 0).</param>
///
/// <param name="rowPosition" type="input"> : Row number (index from 1) of the output pose matrix
/// where user wishes to apply gravity.</param>
///
/// <returns>
/// Returns flag indicating whether pose was successfully created.</returns>
///
DllExport int SP_createPoseFromRow(float pose[12], const float rowValues[3],
                                   const float translation[3],
                                   const int rowPosition);
/// <summary>
/// SP_create Allows user to create Scene perception instance.
/// User must call this function prior to calling any API functions.
/// </summary>
///
/// <returns>
/// Returns flag indicating whether pose was successfully created. </returns>
///
DllExport SP_STATUS SP_create();

DllExport SP_STATUS SP_getMeshingUpdateThresholds(float* maxDiffThreshold, float* aveDiffThreshold);

DllExport int SP_isSceneReconstructionEnabled();

DllExport void SP_getOpenCVtoRSSDKtransform(float pose[12]);

DllExport void SP_getRSSDKtoOpenCVTransform(float pose[12]);

DllExport int SP_multiplyPoseAXB(const float poseA[12], const float poseB[12], float poseC[12]);

DllExport int SP_TransformCameraPose(float pose[12], const float initialPose[12], const float initialUserPoseOpenCV[12], const float transform[12]);

#ifdef __cplusplus
}
#endif //_cplusplus

inline SP_STATUS SP_setConfiguration(
    SP_CameraIntrinsics* pCamParam,
    SP_Resolution resolution
) {
    return SP_setConfiguration(pCamParam, pCamParam, resolution);
}

#endif //SP_CORE_H