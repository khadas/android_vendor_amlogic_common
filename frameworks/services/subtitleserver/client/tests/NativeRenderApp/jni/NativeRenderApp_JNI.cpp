#define LOG_TAG "SubtitleControl-jni"
#include <jni.h>

#include <unistd.h>
#include <android/log.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "BasePlayerNativeInterface_v2.h"


#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))


//using namespace android;

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
       if(! var) ALOGE("Unable to find class %s", className);

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

void startNativeRender(JNIEnv* env, jobject object, jobject surface) {
    ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);

    ALOGD("start Native Render now! calling");
    bpni_setSubtitleSurfaceNativeWindow(BPNI_PLAYER_HANDLE_ID_MAIN, nwin);
    bpni_setSubtitleEnabled(BPNI_PLAYER_HANDLE_ID_MAIN, true);
}

void stopNativeRender(JNIEnv* env, jobject object) {
    ALOGD("stop Native Render now! calling");
    bpni_setSubtitleEnabled(BPNI_PLAYER_HANDLE_ID_MAIN, false);
}

static JNINativeMethod SubtitlRender_Methods[] = {
    {"nStartNativeRender", "(Landroid/view/Surface;)V", (void *)startNativeRender},
    {"nStopNativeRender", "()V", (void *)stopNativeRender},
};


extern "C" void bpni_initialze();

int register_com_droidlogic_app_SubtitleViewAdaptor(JNIEnv *env) {
    static const char *const kClassPathName = "com/droidlogic/NativeRenderSubTest/PlayerActivity";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, SubtitlRender_Methods, NELEM(SubtitlRender_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    bpni_initialze();

    return rc;
}


jint JNI_OnLoad(JavaVM *vm, void *reserved __unused) {
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }


    if (register_com_droidlogic_app_SubtitleViewAdaptor(env) < 0) {
        ALOGE("Can't register SubtitleViewAdaptor JNI");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}

