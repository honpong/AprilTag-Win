#include <string.h>
#include <jni.h>
#include <android/log.h>

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
    
    jmethodID methodId = env->GetMethodID(sensorFusionClass, "onSensorFusionStatusUpdate", "(Lcom/realitycap/android/rcutility/SensorFusionStatus;)V");
    if(RunExceptionCheck(env)) return;
    
    env->CallVoidMethod(thiz, methodId, statusObj);
    if(RunExceptionCheck(env)) return;
}

void SendDataUpdate(JNIEnv* env, jobject thiz, long timestamp)
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
    
    jmethodID methodId = env->GetMethodID(sensorFusionClass, "onSensorFusionDataUpdate", "(Lcom/realitycap/android/rcutility/SensorFusionData;)V");
    if(RunExceptionCheck(env)) return;
    
    env->CallVoidMethod(thiz, methodId, dataObj);
    if(RunExceptionCheck(env)) return;
}


extern "C"
{
	JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_SensorFusion_startSensorFusion( JNIEnv* env, jobject thiz )
	{
        LOGD("startSensorFusion");
	    return(JNI_TRUE);
    }
    
    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_stopSensorFusion( JNIEnv* env, jobject thiz )
    {
        LOGD("stopSensorFusion");
    }
    
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_SensorFusion_startStaticCalibration( JNIEnv* env, jobject thiz )
    {
        LOGD("startStaticCalibration");
        return(JNI_TRUE);
    }
    
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_SensorFusion_startCapture( JNIEnv* env, jobject thiz )
    {
        LOGD("startCapture");
        return(JNI_TRUE);
    }
    
    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_stopCapture( JNIEnv* env, jobject thiz )
    {
        LOGD("stopCapture");
    }
	
	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_receiveAccelerometer( JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp )
	{
//	    LOGV("%li accel %f, %f, %f", (long)timestamp, x, y, z);
	}
	
	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_receiveGyro( JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp )
	{
//	    LOGV("%li gyro %f, %f, %f", (long)timestamp, x, y, z);
	}
	
	JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_SensorFusion_receiveVideoFrame( JNIEnv* env, jobject thiz, jbyteArray data )
	{
	    jbyte* buffer = NULL;
        jsize size = env->GetArrayLength(data);
        if(RunExceptionCheck(env)) return(JNI_FALSE);
	    
	    // check if array size >0
	    if(size<=0) return(JNI_FALSE);
	    
	    // allocate buffer for array and get data from Java array
	    buffer  = new jbyte[size];
	    env->GetByteArrayRegion(data, 0, size, buffer);
	    if(RunExceptionCheck(env)) return(JNI_FALSE);
        
        LOGV(">>>>>>>>>>> Camera frame received <<<<<<<<<<<<<");
	    
	    // set result to Java array and delete buffer
	    env->SetByteArrayRegion(data, 0, size, buffer);
	    delete[] buffer;
	    if(RunExceptionCheck(env)) return(JNI_FALSE);
        
        SendStatusUpdate(env, thiz, 1, 0, 3, 0);
        SendDataUpdate(env, thiz, 12345678L);
        
	    return(JNI_TRUE);
	}
}