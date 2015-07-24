#include <string.h>
#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <rc_intel_interface.h>

#define TAG "RCUtility"
#define LOGV(...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, TAG, __VA_ARGS__))
#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

static JavaVM *javaVM;
static jobject trackerProxyObj;
static rc_Tracker *tracker;
static jobject dataUpdateObj;

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    LOGI("JNI_OnLoad");
    javaVM = vm;
    return JNI_VERSION_1_6;
}

#pragma mark - utility functions

static wchar_t *createWcharFromChar(const char *text)
{
    size_t size = strlen(text) + 1;
    wchar_t *wa = new wchar_t[size];
    mbstowcs(wa, text, size);
    return wa;
}

bool RunExceptionCheck(JNIEnv *env)
{
    if (env->ExceptionCheck())
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

void initJavaObject(JNIEnv *env, const char *path, jobject *objptr)
{
    jclass cls = env->FindClass(path);
    if (!cls)
    {
        LOGE("initJavaObject: failed to get %s class reference", path);
        return;
    }
    jmethodID constr = env->GetMethodID(cls, "<init>", "()V");
    if (!constr)
    {
        LOGE("initJavaObject: failed to get %s constructor", path);
        return;
    }
    jobject obj = env->NewObject(cls, constr);
    if (!obj)
    {
        LOGE("initJavaObject: failed to create a %s object", path);
        return;
    }
    (*objptr) = env->NewGlobalRef(obj);
}

#pragma mark - callbacks up into java land

static void status_callback(void *handle, rc_TrackerState state, rc_TrackerError error, rc_TrackerConfidence confidence, float progress)
{
    int status;
    JNIEnv *env;

    if (!trackerProxyObj)
    {
        LOGE("status_callback: Tracker proxy object is null.");
        return;
    }

    status = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0)
    {
        LOGE("status_callback: Failed to get JNI env.");
        return;
    }

    status = javaVM->AttachCurrentThread(&env, NULL);
    if (status < 0)
    {
        LOGE("status_callback: Failed to attach current thread.");
        return;
    }

    jclass trackerProxyClass = env->GetObjectClass(trackerProxyObj);
    jmethodID methodId = env->GetMethodID(trackerProxyClass, "onStatusUpdated", "(IIIF)V");

    env->CallVoidMethod(trackerProxyObj, methodId, (int)state, (int)error, (int)confidence, progress);
    RunExceptionCheck(env);
}

static void data_callback(void *handle, rc_Timestamp time, rc_Pose pose, rc_Feature *features, size_t feature_count)
{
    int status;
    JNIEnv *env;

    if (!trackerProxyObj)
    {
        LOGE("data_callback: Tracker proxy object is null.");
        return;
    }

    status = javaVM->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (status < 0)
    {
        LOGE("data_callback: Failed to get JNI env.");
        return;
    }

    status = javaVM->AttachCurrentThread(&env, NULL);
    if (status < 0)
    {
        LOGE("data_callback: Failed to attach current thread.");
        return;
    }

    jclass dataUpdateClass = env->GetObjectClass(dataUpdateObj);
    if (RunExceptionCheck(env)) return;

    // set properties on the SensorFusionData instance
    jmethodID methodId = env->GetMethodID(dataUpdateClass, "setTimestamp", "(J)V");
    env->CallVoidMethod(dataUpdateObj, methodId, (long) time);
    if (RunExceptionCheck(env)) return;

    methodId = env->GetMethodID(dataUpdateClass, "setPose", "(FFFFFFFFFFFF)V");
    env->CallVoidMethod(dataUpdateObj, methodId, pose[0], pose[1], pose[2], pose[3], pose[4], pose[5], pose[6], pose[7], pose[8], pose[9], pose[10], pose[11]);
    if (RunExceptionCheck(env)) return;

    methodId = env->GetMethodID(dataUpdateClass, "addFeaturePoint", "(JFFFFF)V"); // takes a long and 5 floats

    // add features to SensorFusionData instance
    for (int i = 0; i < feature_count; ++i)
    {
        rc_Feature feat = features[i];
        env->CallVoidMethod(dataUpdateObj, methodId, feat.id, feat.world.x, feat.world.y, feat.world.z, feat.image_x, feat.image_y);
        if (RunExceptionCheck(env)) return;
    }

    jclass trackerProxyClass = env->GetObjectClass(trackerProxyObj);
    methodId = env->GetMethodID(trackerProxyClass, "onDataUpdated", "(Lcom/realitycap/android/rcutility/SensorFusionData;)V");

    env->CallVoidMethod(trackerProxyObj, methodId, dataUpdateObj);
    if (RunExceptionCheck(env)) return;
}

#pragma mark - functions that get called from java land

extern "C"
{
    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_createTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("createTracker");
        tracker = rc_create();
        if (!tracker) return (JNI_FALSE);

        // save this object for the callbacks.
        trackerProxyObj = env->NewGlobalRef(thiz);

        // init a SensorFusionData instance
        initJavaObject(env, "com/realitycap/android/rcutility/SensorFusionData", &dataUpdateObj);

        rc_setStatusCallback(tracker, status_callback, NULL);
        rc_setDataCallback(tracker, data_callback, NULL);

        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_destroyTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("destroyTracker");
        if (!tracker) return (JNI_FALSE);
        rc_destroy(tracker);
        tracker = NULL;
        trackerProxyObj = NULL;
        env->DeleteGlobalRef(trackerProxyObj);
        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("startTracker");
        if (!tracker) return (JNI_FALSE);

        rc_startTracker(tracker);

        return (JNI_TRUE);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_stopTracker(JNIEnv *env, jobject thiz)
    {
        LOGD("stopTracker");
        if (!tracker) return;
        rc_stopTracker(tracker);
        rc_reset(tracker, 0, rc_POSE_IDENTITY);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startCalibration(JNIEnv *env, jobject thiz)
    {
        LOGD("startCalibration");
        if (!tracker) return (JNI_FALSE);

        rc_startCalibration(tracker);

        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_startReplay(JNIEnv *env, jobject thiz)
    {
        LOGD("startReplay");
        if (!tracker) return (JNI_FALSE);
        return (JNI_TRUE);
    }

    JNIEXPORT jstring JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_getCalibration(JNIEnv *env, jobject thiz)
    {
        LOGD("getCalibration");
        if (!tracker) return env->NewStringUTF("");

        const wchar_t *cal;
        size_t size = rc_getCalibration(tracker, &cal);

        if (!size) return env->NewStringUTF("");

        // convert wchar_t to char
        char* buffer = (char*)malloc(sizeof(char)*size);
        wcstombs(buffer, cal, size);

        jstring result = env->NewStringUTF(buffer);

        delete buffer;

        return result;
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_setCalibration(JNIEnv *env, jobject thiz, jstring calString)
    {
        LOGD("setCalibration");
        if (!tracker) return (JNI_FALSE);
        const char *cString = env->GetStringUTFChars(calString, 0);
        const wchar_t *wString = createWcharFromChar(cString);
        jboolean result = (jboolean) rc_setCalibration(tracker, wString);
        delete wString;
        env->ReleaseStringUTFChars(calString, cString);
        return (result);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_setOutputLog(JNIEnv *env, jobject thiz, jstring filename)
    {
        LOGD("setOutputLog");
        if (!tracker) return (JNI_FALSE);
        const char *cFilename = env->GetStringUTFChars(filename, 0);
        const wchar_t *wFilename = createWcharFromChar(cFilename);
        rc_setOutputLog(tracker, wFilename);
        delete wFilename;
        env->ReleaseStringUTFChars(filename, cFilename);
        return (JNI_TRUE);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_configureCamera(JNIEnv *env, jobject thiz, jint camera, jint width_px, jint height_px, jfloat center_x_px,
                                                                                                  jfloat center_y_px, jfloat focal_length_x_px, jfloat focal_length_y_px, jfloat skew, jboolean fisheye,
                                                                                                  jfloat fisheye_fov_radians)
    {
        if (!tracker) return (JNI_FALSE);
        const rc_Pose pose = {0, -1, 0, 0,
                -1, 0, 0, 0,
                0, 0, -1, 0};
        rc_configureCamera(tracker, (rc_Camera) camera, pose, width_px, height_px, center_x_px, center_y_px, focal_length_x_px, focal_length_y_px, skew, fisheye, fisheye_fov_radians);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveAccelerometer(JNIEnv *env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp)
    {
        if (!tracker) return;
        rc_Vector vec = {x, y, z};
        rc_Timestamp ts = timestamp / 1000; // convert ns to us
        rc_receiveAccelerometer(tracker, ts, vec);
    //	    LOGV("%li accel %f, %f, %f", (long)timestamp, x, y, z);
    }

    JNIEXPORT void JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveGyro(JNIEnv *env, jobject thiz, jfloat x, jfloat y, jfloat z, jlong timestamp)
    {
        if (!tracker) return;
        rc_Vector vec = {x, y, z};
        rc_Timestamp ts = timestamp / 1000; // convert ns to us
        rc_receiveGyro(tracker, ts, vec);
    //	    LOGV("%li gyro %f, %f, %f", (long)timestamp, x, y, z);
    }

    JNIEXPORT jboolean JNICALL Java_com_realitycap_android_rcutility_TrackerProxy_receiveImageWithDepth(JNIEnv *env, jobject thiz, jlong time_us, jlong shutter_time_us, jboolean force_recognition,
                                                                                                        jint width, jint height, jint stride, jobject colorData, jint depthWidth, jint depthHeight,
                                                                                                        jint depthStride, jobject depthData)
    {
        if (!tracker) return (JNI_FALSE);

        void *colorPtr = env->GetDirectBufferAddress(colorData);
        jlong colorLength = env->GetDirectBufferCapacity(colorData);
        if (RunExceptionCheck(env)) return (JNI_FALSE);

        void *depthPtr = env->GetDirectBufferAddress(depthData);
        jlong depthLength = env->GetDirectBufferCapacity(depthData);
        if (RunExceptionCheck(env)) return (JNI_FALSE);

//        LOGV(">>>>>>>>>>> Synced camera frames received <<<<<<<<<<<<<");

        rc_receiveImageWithDepth(tracker, rc_EGRAY8, time_us, shutter_time_us, NULL, false, width, height, stride, colorData, NULL, NULL, depthWidth, depthHeight, depthStride, depthData, NULL, NULL);

        return (JNI_TRUE);
    }
}