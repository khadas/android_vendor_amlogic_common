/*
 * Copyright 2012, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "screencontrol-jni"
#include "com_droidlogic_app_ScreenControlManager.h"

static sp<ScreenControlClient> spScreenCtrl = NULL;
static jclass g_jclazz;
static jmethodID g_proc;
static JavaVM*g_vm = NULL;
static jobject g_obj = NULL;

class AvcRecordMsg;
class YuvRecordMsg;


static sp<AvcRecordMsg> gAvcRecordMsg;
static sp<YuvRecordMsg> gYuvRecordMsg;

JNIEnv *attach_java_thread(const char * threadName){
    static __thread JNIEnv* g_t_env = NULL;
    if (g_t_env != NULL) {
      //return g_t_env;
    }
    JavaVMAttachArgs args;
    jint result;
    JNIEnv*e = NULL;
    args.version = JNI_VERSION_1_4;
    args.name = (char*) threadName;
    args.group = NULL;
    if ((result = g_vm->AttachCurrentThread(&e, (void*) &args)) != JNI_OK) {
      ALOGE("NOTE: attach of thread '%s' failed\n", threadName);
      return NULL;
    }
    g_t_env = e;
    return e;
}

class AvcRecordMsg:public ScreenControlClient::AvcRecordCallback {
public:
    AvcRecordMsg(){
    }
    virtual ~AvcRecordMsg(){}
    void onAvcDataArouse(void* data, int32_t size, int32_t frame_type, int64_t pts){
        ALOGD("onAvcDataArouse----- data=%p,size=%d,frame_type=%d,pts=%lld",data,size,frame_type,pts);
        JNIEnv *env = attach_java_thread("screen_crontrol");
        jbyteArray arr = env->NewByteArray(size);
        env->SetByteArrayRegion(arr, 0, size, (jbyte *)data);
        env->CallStaticIntMethod(g_jclazz, g_proc, g_obj, MSG_DATA_TYPE_AVC, frame_type, pts, arr);
        env->DeleteLocalRef(arr);
    }
};

class YuvRecordMsg:public ScreenControlClient::YuvRecordCallback{
public:
    YuvRecordMsg(){
    }
    virtual ~YuvRecordMsg(){}
    void onYuvDataArouse(void *data, int32_t size) {
        ALOGD("onYuvDataArouse------------- data=%p,size=%d",data,size);
        JNIEnv *env = attach_java_thread("screen_crontrol");
        jbyteArray arr = env->NewByteArray(size);
        env->SetByteArrayRegion(arr, 0, size, (jbyte *)data);
        uint8_t frameType =0;
        int64_t pts =0;
        env->CallStaticIntMethod(g_jclazz, g_proc, g_obj, MSG_DATA_TYPE_YUV,frameType,pts,arr);
        env->DeleteLocalRef(arr);
    }
};

static sp<ScreenControlClient>& getScreenControlClient()
{
    if (spScreenCtrl == NULL)
        spScreenCtrl = new ScreenControlClient();
    return spScreenCtrl;
}

static void ConnectScreenControl(JNIEnv *env __unused, jclass clazz __unused)
{
    ALOGI("Connect Screen Control");
}



static jbyteArray ScreenControlCapScreenBuffer(JNIEnv *env, jobject, jint left,
    jint top, jint right, jint bottom, jint width, jint height, jint sourceType)
{
    sp<ScreenControlClient>& scc = getScreenControlClient();
    if (scc != NULL) {
        jbyte *buffer = NULL;
        int bufferSize = 0;
        int ret = NO_ERROR;

        ret = scc->startScreenCapBuffer(left, top, right, bottom, width,
                    height, sourceType, (void **)&buffer, &bufferSize);
        jbyteArray arr = env->NewByteArray(bufferSize);
        env->SetByteArrayRegion(arr, 0, bufferSize, (jbyte *)buffer);
        delete [] buffer;
        return arr;
    } else
        return NULL;
}

static jint ScreenControlRecordScreen(JNIEnv *env, jobject, jint left,
    jint top, jint right, jint bottom, jint width, jint height,jint frameRate, jint bitRate, jint limitTimeSec, jint sourceType, jstring jfilename)
{
    sp<ScreenControlClient>& scc = getScreenControlClient();
    if (scc != NULL) {
        const char *filename = env->GetStringUTFChars(jfilename, nullptr);
        return scc->startScreenRecord(left, top, right, bottom, width, height,
            frameRate, bitRate, limitTimeSec, sourceType, filename);
    } else
        return -1;
}

static void ScreenControlStartReceiver(JNIEnv *env , jobject, jobject wo)
{
    jclass cls;
    if ((cls = env->FindClass("com/droidlogic/app/ScreenControlManager")) == NULL) {
      ALOGE("Can't find class : com/droidlogic/app/ScreenControlManager");
      return ;
    }
    g_jclazz = (jclass) env->NewGlobalRef(cls);
    if ((g_proc = env->GetStaticMethodID(g_jclazz, "native_proc", "(Ljava/lang/Object;IIJLjava/lang/Object;)I"))
        == NULL) {
      ALOGE("no such method: native_proc");
      return ;
    }
    if ((g_obj = env->NewGlobalRef(wo)) == NULL) {
      return ;
    }
}

static jint ScreenControlStartAvcRecord(JNIEnv *, jobject, jint left,jint top, jint right, jint bottom,
                                    jint width, jint height, jint frameRate, int bitRate, jint sourceType)
{
    sp<ScreenControlClient>& scc = getScreenControlClient();
    if (scc != NULL) {
        gAvcRecordMsg= new AvcRecordMsg();
        if (gAvcRecordMsg ==NULL)
            return -1;
        scc->setAvcCallback(gAvcRecordMsg);
        return scc->startAvcScreenRecord(left, top, right, bottom, width, height,frameRate, bitRate,sourceType);
    }
    return -1;
}

static jint ScreenControlStartYuvRecord(JNIEnv *, jobject, jint left,jint top, jint right, jint bottom,
                                    jint width, jint height, jint frameRate, jint sourceType)
{
    sp<ScreenControlClient>& scc = getScreenControlClient();
    if (scc != NULL) {
        gYuvRecordMsg= new YuvRecordMsg();
        if (gYuvRecordMsg ==NULL)
            return -1;
        scc->setYuvCallback(gYuvRecordMsg);
        return scc->startYuvScreenRecord(left, top, right, bottom, width, height,frameRate,sourceType);
    }
    return -1;
}




static void ScreenControlForceStop(JNIEnv *, jobject)
{
    sp<ScreenControlClient>& scc = getScreenControlClient();
    if (scc != NULL) {
        scc->forceStop();
    }
}




static JNINativeMethod ScreenControl_Methods[] = {
    {"native_ConnectScreenControl", "()V", (void *) ConnectScreenControl },
    {"native_ScreenCapBuffer", "(IIIIIII)[B", (void *) ScreenControlCapScreenBuffer},
    {"native_ScreenRecord", "(IIIIIIIIIILjava/lang/String;)I", (void *) ScreenControlRecordScreen},
    {"native_StartReceiver", "(Ljava/lang/ref/WeakReference;)V", (void *) ScreenControlStartReceiver },
    {"native_startAvcRecord", "(IIIIIIIII)I", (void *) ScreenControlStartAvcRecord},
    {"native_startYuvRecord", "(IIIIIIII)I", (void *) ScreenControlStartYuvRecord},
    {"native_ForceStop", "()V", (void *) ScreenControlForceStop },
};

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className);

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

int register_com_droidlogic_app_ScreenControlManager(JNIEnv *env)
{
    static const char *const kClassPathName = "com/droidlogic/app/ScreenControlManager";
    jclass clazz;
    int rc;
    FIND_CLASS(clazz, kClassPathName);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", kClassPathName);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, ScreenControl_Methods, NELEM(ScreenControl_Methods)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", kClassPathName, rc);
        return -1;
    }

    return rc;
}

jint JNI_OnLoad(JavaVM *vm, void *reserved __unused)
{
    JNIEnv *env = NULL;
    jint result = -1;

    g_vm=vm;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        ALOGI("ERROR: GetEnv failed\n");
        goto bail;
    }
    assert(env != NULL);

    if (register_com_droidlogic_app_ScreenControlManager(env) < 0)
    {
        ALOGE("Can't register DtvkitGlueClient");
        goto bail;
    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

bail:
    return result;
}



