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

JNIEnv *jniEnv;
jobject callingObj;

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

void SendStatusUpdate(int runState, float progress, int confidence, int errorCode)
{
    if (!jniEnv || !callingObj) return;

    // init a SensorFusionStatus instance
    jclass statusClass = jniEnv->FindClass("com/realitycap/android/rcutility/SensorFusionStatus");
    if(RunExceptionCheck(jniEnv)) return;
    
    jmethodID initId = jniEnv->GetMethodID(statusClass, "<init>", "()V");
    if(RunExceptionCheck(jniEnv)) return;
    
    jobject statusObj = jniEnv->NewObject(statusClass, initId);
    if(RunExceptionCheck(jniEnv)) return;
    
    CallMethod(jniEnv, statusClass, statusObj, "setRunState", runState);
    CallMethod(jniEnv, statusClass, statusObj, "setProgress", progress);
    CallMethod(jniEnv, statusClass, statusObj, "setConfidence", confidence);
    CallMethod(jniEnv, statusClass, statusObj, "setErrorCode", errorCode);
    
    // pass status object to the callback
    jclass sensorFusionClass = jniEnv->GetObjectClass(callingObj);
    if(RunExceptionCheck(jniEnv)) return;
    
    jmethodID methodId = jniEnv->GetMethodID(sensorFusionClass, "onStatusUpdated", "(Lcom/realitycap/android/rcutility/SensorFusionStatus;)V");
    if(RunExceptionCheck(jniEnv)) return;

    jniEnv->CallVoidMethod(callingObj, methodId, statusObj);
    if(RunExceptionCheck(jniEnv)) return;
}

static void status_callback(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress)
{
    SendStatusUpdate(state, progress, confidence, error);
}

rc_Tracker* tracker;

extern "C"
{
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_createTracker( JNIEnv* env, jobject thiz )
    {
        LOGD("createTracker");
        tracker = rc_create();
        if (!tracker) return (JNI_FALSE);
        else return(JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_destroyTracker( JNIEnv* env, jobject thiz )
    {
        LOGD("destroyTracker");
        if (!tracker) return (JNI_FALSE);
        rc_destroy(tracker);
        return(JNI_TRUE);
    }

	JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startTracker( JNIEnv* env, jobject thiz )
	{
        LOGD("startTracker");
        if (!tracker) return (JNI_FALSE);

        // save these for the status callback. not sure if this will work.
        jniEnv = env;
        callingObj = thiz;

        rc_setStatusCallback(tracker, status_callback, NULL);
        rc_startTracker(tracker);

	    return(JNI_TRUE);
    }
    
    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_stopTracker( JNIEnv* env, jobject thiz )
    {
        LOGD("stopTracker");
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