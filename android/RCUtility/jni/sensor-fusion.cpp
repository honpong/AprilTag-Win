#include <string.h>
#include <jni.h>
#include <android/log.h>

#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, "RCUtility", __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "RCUtility", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "RCUtility", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "RCUtility", __VA_ARGS__))

extern "C"
{
	JNIEXPORT jstring JNICALL Java_com_realitycap_android_rcutility_SensorFusion_stringFromJNI( JNIEnv* env, jobject thiz )
	{
	    return (*env)->NewStringUTF(env, "Ready");
	}
	
	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_receiveAccelerometer( JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp )
	{
	    LOGV("%li accel %f, %f, %f", (long)timestamp, x, y, z);
	}
	
	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_receiveGyro( JNIEnv* env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp )
	{
	    LOGV("%li gyro %f, %f, %f", (long)timestamp, x, y, z);
	}
	
	JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_SensorFusion_receiveVideoFrame( JNIEnv* env, jobject thiz, jbyteArray data )
	{
	    LOGV(">>>>>>>>>>> Camera frame received <<<<<<<<<<<<<");
	}
}