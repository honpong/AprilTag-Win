#include <string.h>
#include <jni.h>
#include <android/log.h>

#define TAG "RCUtility"
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

extern "C"
{
    void SendFakeDataUpdate(JNIEnv* env, jobject thiz)
    {
        // init a SensorFusionData instance
        jclass dataClass = env->FindClass("com/realitycap/android/rcutility/SensorFusionData");
        jmethodID initId = env->GetMethodID(dataClass, "<init>", "()V");
        jobject dataObj = env->NewObject(dataClass, initId);
        
        // set the timestamp
        jmethodID setTimestampId = env->GetMethodID(dataClass, "setTimestamp", "(J)V"); // J means argument is a long. V means return type is void.
        jvalue longVal;
        longVal.j = 12345678L;
        env->CallVoidMethod(dataObj, setTimestampId, longVal);
        
        // pass object to the callback
        jclass sensorFusionClass = env->GetObjectClass(thiz);
        jmethodID methodId = env->GetMethodID(sensorFusionClass, "onSensorFusionDataUpdate", "(Lcom/realitycap/android/rcutility/SensorFusionData;)V");
        env->CallVoidMethod(thiz, methodId, dataObj);
    }
    
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
	    
	    // check if array size >0
	    if(size<=0) return(JNI_FALSE);
	    if(env->ExceptionCheck())
        {
#ifndef NDEBUG
	      env->ExceptionDescribe();
#endif
	      env->ExceptionClear();
	      return(JNI_FALSE);
	    }
	    
	    // allocate buffer for array and get data from Java array
	    buffer  = new jbyte[size];
	    env->GetByteArrayRegion(data, 0, size, buffer);
	    // exception check
	    if(env->ExceptionCheck())
        {
#ifndef NDEBUG
	      env->ExceptionDescribe();
#endif
	      env->ExceptionClear();
	      delete[] buffer;
	      return(JNI_FALSE);
	    }
        
        LOGV(">>>>>>>>>>> Camera frame received <<<<<<<<<<<<<");
	    
	    // set result to Java array and delete buffer
	    env->SetByteArrayRegion(data, 0, size, buffer);
	    delete[] buffer;
	    if(env->ExceptionCheck())
        {
#ifndef NDEBUG
	      env->ExceptionDescribe();
#endif
	      env->ExceptionClear();
	      return(JNI_FALSE);
	    }
        
        SendFakeDataUpdate(env, thiz);
        
	    return(JNI_TRUE);
	}
}