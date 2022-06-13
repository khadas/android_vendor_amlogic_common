#include <jni.h>
#include <utils/RefBase.h>
#include <utils/Mutex.h>

#include <utils/Atomic.h>
#include <utils/Log.h>
//#include <utils/RefBase.h>
#include <utils/String8.h>
//#include <utils/String16.h>
#include <unistd.h>
#include <cutils/properties.h>
#include <dlfcn.h>


#include <android/native_window.h>
//#include <android/native_window_jni.h>

using namespace android;

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

void startNativeRender(JNIEnv* env, jobject object, jobject surface) {
    //ANativeWindow *nwin = ANativeWindow_fromSurface(env, surface);

    sp<ANativeWindow> win = android_view_Surface_getNativeWindow(env, surface);
    if (win != NULL) {
    win->incStrong((void*)ANativeWindow_fromSurface);
    }
    return win.get();

}

static JNINativeMethod SubtitlRender_Methods[] = {
    {"nStartNativeRender", "(Landroid/view/Surface;)V", (void *)startNativeRender},

};

int register_com_droidlogic_app_SubtitleViewAdaptor(JNIEnv *env) {
    static const char *const kClassPathName = "com/droidlogic/app/SubtitleViewAdaptor";
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

    return rc;
}



jint JNI_OnLoad(JavaVM *vm, void *reserved __unused) {
    JNIEnv *env = NULL;
    jint result = -1;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    gJniContext = new JniContext();
    getJniContext()->mJavaVM = vm;

    if (register_com_droidlogic_app_SubtitleManager(env) < 0) {
        ALOGE("Can't register SubtitleManager JNI");
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

