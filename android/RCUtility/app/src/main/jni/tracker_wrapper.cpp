#include <string.h>
#include <jni.h>
#include <android/log.h>
#include "rc_intel_interface.h"

#define TAG "RCUtility"
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

bool RunExceptionCheck(JNIEnv* env)
{
    if(env->ExceptionCheck())
    {
#ifndef NDEBUG
        env->ExceptionDescribe();
        LOGE("JNI ERROR");
#endif
        env->ExceptionClear();
        return true;
    }
    else
    {
        return false;
    }
}

void CallMethod(JNIEnv* env, jclass theClass, jobject obj, const char* methodName, int argument)
{
    jmethodID methodId = env->GetMethodID(theClass, methodName, "(I)V");
    env->CallVoidMethod(obj, methodId, argument);
    if(RunExceptionCheck(env)) return;
}

void CallMethod(JNIEnv* env, jclass theClass, jobject obj, const char* methodName, float argument)
{
    jmethodID methodId = env->GetMethodID(theClass, methodName, "(F)V");
    env->CallVoidMethod(obj, methodId, argument);
    if(RunExceptionCheck(env)) return;
}

void CallMethod(JNIEnv* env, jclass theClass, jobject obj, const char* methodName, long argument)
{
    // we have to use jvalue here for some reason
    jmethodID methodId = env->GetMethodID(theClass, methodName, "(J)V");
    jvalue argVal;
    argVal.j = argument;
    env->CallVoidMethod(obj, methodId, argVal);
    if(RunExceptionCheck(env)) return;
}

void SendStatusUpdate(JNIEnv* env, jobject thiz, int runState, float progress, int confidence, int errorCode)
{
    // init a SensorFusionStatus instance
    jclass statusClass = env->FindClass("com/realitycap/android/rcutility/SensorFusionStatus");
    if(RunExceptionCheck(env)) return;
    
    jmethodID initId = env->GetMethodID(statusClass, "<init>", "()V");
    if(RunExceptionCheck(env)) return;
    
    jobject statusObj = env->NewObject(statusClass, initId);
    if(RunExceptionCheck(env)) return;
    
    CallMethod(env, statusClass, statusObj, "setRunState", runState);
    CallMethod(env, statusClass, statusObj, "setProgress", progress);
    CallMethod(env, statusClass, statusObj, "setConfidence", confidence);
    CallMethod(env, statusClass, statusObj, "setErrorCode", errorCode);
    
    // pass status object to the callback
    jclass sensorFusionClass = env->GetObjectClass(thiz);
    if(RunExceptionCheck(env)) return;
    
    jmethodID methodId = env->GetMethodID(sensorFusionClass, "onStatusUpdated", "(Lcom/realitycap/android/rcutility/SensorFusionStatus;)V");
    if(RunExceptionCheck(env)) return;
    
    env->CallVoidMethod(thiz, methodId, statusObj);
    if(RunExceptionCheck(env)) return;
}

void SendProgressUpdate(JNIEnv *env, jobject thiz, long timestamp)
{
    // init a SensorFusionData instance
    jclass dataClass = env->FindClass("com/realitycap/android/rcutility/SensorFusionData");
    if(RunExceptionCheck(env)) return;
    
    jmethodID initId = env->GetMethodID(dataClass, "<init>", "()V");
    if(RunExceptionCheck(env)) return;
    
    jobject dataObj = env->NewObject(dataClass, initId);
    if(RunExceptionCheck(env)) return;
    
    // TODO add other properties
    CallMethod(env, dataClass, dataObj, "setTimestamp", timestamp);
    
    // pass data object to the callback
    jclass sensorFusionClass = env->GetObjectClass(thiz);
    if(RunExceptionCheck(env)) return;
    
    jmethodID methodId = env->GetMethodID(sensorFusionClass, "onProgressUpdated", "(Lcom/realitycap/android/rcutility/SensorFusionData;)V");
    if(RunExceptionCheck(env)) return;
    
    env->CallVoidMethod(thiz, methodId, dataObj);
    if(RunExceptionCheck(env)) return;
}

rc_Tracker* tracker;

extern "C"
{
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_create( JNIEnv* env, jobject thiz )
    {
        LOGD("rc_create");
        tracker = rc_create();
        if (!tracker) return (JNI_FALSE);
        else return(JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_destroy( JNIEnv* env, jobject thiz )
    {
        LOGD("rc_destroy");
        if (!tracker) return (JNI_FALSE);
        rc_destroy(tracker);
        return(JNI_TRUE);
    }

	JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_start( JNIEnv* env, jobject thiz )
	{
        LOGD("start");
        if (!tracker) return (JNI_FALSE);
        rc_startTracker(tracker);
	    return(JNI_TRUE);
    }
    
    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_stop( JNIEnv* env, jobject thiz )
    {
        LOGD("stop");
        if (!tracker) return;
        rc_stopTracker(tracker);
//        rc_reset(tracker, 0, rc_POSE_IDENTITY);
    }
    
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startCalibration( JNIEnv* env, jobject thiz )
    {
        LOGD("startCalibration");
        if (!tracker) return (JNI_FALSE);
        return(JNI_TRUE);
    }
    
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startReplay( JNIEnv* env, jobject thiz )
    {
        LOGD("startReplay");
        if (!tracker) return (JNI_FALSE);
        return(JNI_TRUE);
    }
    
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_setOutputLog( JNIEnv* env, jobject thiz )
    {
        LOGD("setOutputLog");
        if (!tracker) return (JNI_FALSE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_configureCamera(JNIEnv* env, jobject thiz, jint camera, jint width_px, jint height_px, jfloat center_x_px, jfloat center_y_px, jfloat focal_length_x_px, jfloat focal_length_y_px, jfloat skew, jboolean fisheye, jfloat fisheye_fov_radians)
    {
        if (!tracker) return (JNI_FALSE);
        const rc_Pose pose = { 0, -1, 0, 0,
                             -1, 0, 0, 0,
                              0, 0, -1, 0 };
        rc_Camera cam = (rc_Camera)camera;
        rc_configureCamera(tracker, cam, pose, width_px, height_px, center_x_px, center_y_px, focal_length_x_px, focal_length_y_px, skew, fisheye, fisheye_fov_radians);
    }

	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveAccelerometer( JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp )
	{
        if (!tracker) return;
        rc_Vector vec = { x, y, z };
        rc_Timestamp ts = timestamp / 1000; // convert ns to us
        rc_receiveAccelerometer(tracker, ts, vec);
//	    LOGV("%li accel %f, %f, %f", (long)timestamp, x, y, z);
	}
	
	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveGyro( JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp )
	{
        if (!tracker) return;
        rc_Vector vec = { x, y, z };
        rc_Timestamp ts = timestamp / 1000; // convert ns to us
        rc_receiveGyro(tracker, ts, vec);
//	    LOGV("%li gyro %f, %f, %f", (long)timestamp, x, y, z);
	}
	
	JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveImageWithDepth( JNIEnv* env, jobject thiz, jlong time_us, jlong shutter_time_us, jboolean force_recognition, jint width, jint height, jint stride, jobject colorData, jint depthWidth, jint depthHeight, jint depthStride, jobject depthData)
    {
        if (!tracker) return (JNI_FALSE);

        void* colorPtr = env->GetDirectBufferAddress(colorData);
        jlong colorLength = env->GetDirectBufferCapacity(colorData);
        if(RunExceptionCheck(env)) return(JNI_FALSE);

        void* depthPtr = env->GetDirectBufferAddress(depthData);
        jlong depthLength = env->GetDirectBufferCapacity(depthData);
        if(RunExceptionCheck(env)) return(JNI_FALSE);

        LOGV(">>>>>>>>>>> Synced camera frames received <<<<<<<<<<<<<");

        rc_receiveImageWithDepth(tracker, rc_EGRAY8, time_us, shutter_time_us, NULL, false, width, height, stride, colorData, NULL, NULL, depthWidth, depthHeight, depthStride, depthData, NULL, NULL);

        return(JNI_TRUE);
    }
}