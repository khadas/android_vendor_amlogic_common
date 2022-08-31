/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC JHdmiCecExtend
 */

#define LOG_TAG "HdmiCecExtend"

#include <jni.h>
//#include <JNIHelp.h>
#include <sys/param.h>

#include <HdmiCecBase.h>
//#include <HdmiCecClient.h>
#include <HdmiCecHidlClient.h>
#include <android/log.h>

#include "hdmi_scoped_array.h"

namespace android {

#define FIND_CLASS(var, className) \
        var = env->FindClass(className); \
        LOG_FATAL_IF(! var, "Unable to find class " className)

#define GET_METHOD_ID(var, clazz, methodName, methodDescriptor) \
        var = env->GetMethodID(clazz, methodName, methodDescriptor); \
        LOG_FATAL_IF(! var, "Unable to find method " methodName);

static struct {
    jmethodID handleIncomingMessage;
    jmethodID onAction;
} gHdmiCecExtendClassInfo;

class JHdmiCecExtend : public HdmiCecBase, public HdmiCecEventListener, public HdmiCecActionCallback {
public:
    JHdmiCecExtend(JNIEnv *env, jobject callbacksObj);
    ~JHdmiCecExtend();

    void init();

    virtual int getVersion(int* version);
    virtual int getVendorId(uint32_t* vendorId);
    virtual int getPhysicalAddress(uint16_t* addr);
    virtual int sendMessage(const cec_message_t* message, bool isExtend);

    virtual int openCecDevice() {return -1;}
    virtual int closeCecDevice() {return -1;}
    virtual void getPortInfos(hdmi_port_info_t* list[] __unused, int* total __unused) {}
    virtual int addLogicalAddress(cec_logical_address_t address __unused) {return -1;}
    virtual void clearLogicaladdress() {}
    virtual void setOption(int flag __unused, int value __unused) {}
    virtual void setAudioReturnChannel(int port __unused, bool flag __unused) {}
    virtual bool isConnected(int port __unused) {return false;}

    virtual void onEventUpdate(const hdmi_cec_event_t* event);
    virtual bool onAction(hdmi_cec_action_type action, const hdmi_cec_event_t* event);

private:
    HdmiCecHidlClient *mHdmiCecHidlClient;
    JNIEnv *mEnv;
    jobject mCallbacksObj;
};

JHdmiCecExtend::JHdmiCecExtend(JNIEnv *env, jobject callbacksObj) :
        mEnv(env), mCallbacksObj(callbacksObj) {
    //mHdmiCecClient = HdmiCecClient::connect();
    //mHdmiCecClient->setEventObserver(this);
    mHdmiCecHidlClient = HdmiCecHidlClient::connect(CONNECT_TYPE_EXTEND);
    mHdmiCecHidlClient->setEventObserver(this);

}

JHdmiCecExtend::~JHdmiCecExtend() {
    //mHdmiCecClient.clear();

    delete mHdmiCecHidlClient;
}

void JHdmiCecExtend::init() {
}

int JHdmiCecExtend::getPhysicalAddress(uint16_t* addr) {
    //if (mHdmiCecClient != NULL)
    //    return mHdmiCecClient->getPhysicalAddress(addr);
    //return 0;

    return mHdmiCecHidlClient->getPhysicalAddress(addr);
}


int JHdmiCecExtend::getVendorId(uint32_t* vendorId) {
    //if (mHdmiCecClient != NULL)
    //    return mHdmiCecClient->getVendorId(vendorId);
    //return 0;

    return mHdmiCecHidlClient->getVendorId(vendorId);
}

int JHdmiCecExtend::getVersion(int* version) {
    //if (mHdmiCecClient != NULL)
    //    return mHdmiCecClient->getVersion(version);
    //return 0;

    return mHdmiCecHidlClient->getVersion(version);
}

int JHdmiCecExtend::sendMessage(const cec_message_t* message, bool isExtend) {
    //if (mHdmiCecClient != NULL)
    //    return mHdmiCecClient->sendMessage(message, isExtend);
    //return 0;

    return mHdmiCecHidlClient->sendMessage(message, isExtend);
}

void JHdmiCecExtend::onEventUpdate(const hdmi_cec_event_t* event)
{
    if ((event->eventType & HDMI_EVENT_RECEIVE_MESSAGE) != 0) {
        char msg_buf[CEC_MESSAGE_BODY_MAX_LENGTH];
        memset(msg_buf, 0, sizeof(msg_buf));
        memcpy(msg_buf + 1, event->cec.body, event->cec.length);
        msg_buf[0] = ((event->cec.initiator << 4) & 0x0f) | (event->cec.destination & 0x0f);

        jbyteArray array = mEnv->NewByteArray(event->cec.length + 1);
        const jbyte* bodyPtr = reinterpret_cast<const jbyte *>(msg_buf);
        mEnv->SetByteArrayRegion(array, 0, event->cec.length + 1, bodyPtr);
        mEnv->CallVoidMethod(mCallbacksObj, gHdmiCecExtendClassInfo.handleIncomingMessage, array);
        mEnv->DeleteLocalRef(array);
    }
}

bool JHdmiCecExtend::onAction(hdmi_cec_action_type action, const hdmi_cec_event_t* event)
{
    ALOGD("onAction action %d", (int)action);
    jbyteArray array = NULL;
    if (event != NULL) {
        char msg_buf[CEC_MESSAGE_BODY_MAX_LENGTH];
        memset(msg_buf, 0, sizeof(msg_buf));
        memcpy(msg_buf + 1, event->cec.body, event->cec.length);
        msg_buf[0] = ((event->cec.initiator << 4) & 0x0f) | (event->cec.destination & 0x0f);

        array = mEnv->NewByteArray(event->cec.length + 1);
        const jbyte* bodyPtr = reinterpret_cast<const jbyte *>(msg_buf);
        mEnv->SetByteArrayRegion(array, 0, event->cec.length + 1, bodyPtr);
    }

    jboolean result = mEnv->CallBooleanMethod(mCallbacksObj, gHdmiCecExtendClassInfo.onAction, (int)action, array);
    mEnv->DeleteLocalRef(array);
    return result;
}

//----------------------------------------------------------------------------------------

static jint nativeSendMessage(JNIEnv* env, jclass clazz __unused, jlong extendPtr, jint dest, jbyteArray body)
{
    JHdmiCecExtend* extend = reinterpret_cast<JHdmiCecExtend*>(extendPtr);
    cec_message_t message;

    message.destination = static_cast<cec_logical_address_t>(dest);

    jsize len = env->GetArrayLength(body);
    message.length = MIN(len, CEC_MESSAGE_BODY_MAX_LENGTH);

    ScopedByteArrayRO bodyPtr(env, body);
    std::memcpy(message.body, bodyPtr.get(), message.length);

    return extend->sendMessage(&message, true);
}

static jlong nativeInit(JNIEnv *env, jobject thiz __unused, jobject obj)
{
    JHdmiCecExtend *extend = new JHdmiCecExtend(env, env->NewGlobalRef(obj));
    extend->init();
    return reinterpret_cast<jlong>(extend);
}

static jint nativeGetPhysicalAddress(JNIEnv* env __unused, jclass clazz __unused, jlong extendPtr)
{
    JHdmiCecExtend* extend = reinterpret_cast<JHdmiCecExtend*>(extendPtr);
    unsigned short addr = -1;
    extend->getPhysicalAddress(&addr);
    return addr;
}

static jint nativeGetVendorId(JNIEnv* env __unused, jclass clazz __unused, jlong extendPtr)
{
    JHdmiCecExtend* extend = reinterpret_cast<JHdmiCecExtend*>(extendPtr);
    unsigned int id = 0;
    extend->getVendorId(&id);
    return id;
}

static jint nativeGetVersion(JNIEnv* env __unused, jclass clazz __unused, jlong extendPtr)
{
    JHdmiCecExtend* extend = reinterpret_cast<JHdmiCecExtend*>(extendPtr);
    int version = 0;
    extend->getVersion(&version);
    return version;
}

static JNINativeMethod hdmiExtend_method[] = {
    {"nativeSendCecMessage", "(JI[B)I", (void *)nativeSendMessage},
    {"nativeInit", "(Lcom/droidlogic/HdmiCecExtend;)J", (void *)nativeInit},
    {"nativeGetPhysicalAddr", "(J)I", (void *)nativeGetPhysicalAddress},
    {"nativeGetVendorId", "(J)I", (void *)nativeGetVendorId},
    {"nativeGetCecVersion", "(J)I", (void *)nativeGetVersion},
};

#define CLASS_PATH "com/droidlogic/HdmiCecExtend"

int register_droidlogic_HdmiCecExtend(JNIEnv* env) {
    int rc;
    jclass clazz;
    FIND_CLASS(clazz, CLASS_PATH);

    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'\n", CLASS_PATH);
        return -1;
    }

    rc = (env->RegisterNatives(clazz, hdmiExtend_method, NELEM(hdmiExtend_method)));
    if (rc < 0) {
        env->DeleteLocalRef(clazz);
        ALOGE("RegisterNatives failed for '%s' %d\n", CLASS_PATH, rc);
        return -1;
    }

    GET_METHOD_ID(gHdmiCecExtendClassInfo.handleIncomingMessage, clazz, "handleIncomingMessage", "([B)V");
    GET_METHOD_ID(gHdmiCecExtendClassInfo.onAction, clazz, "onAction", "(I[B)Z");

    return rc;
}

}//end namespace android

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved __unused)
{
    JNIEnv* env = NULL;
    jint result = -1;

    ALOGD("load hdmi cec extend jni...\n");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGD("GetEnv failed!\n");
        return result;
    }

    register_droidlogic_HdmiCecExtend(env);

    return JNI_VERSION_1_4;
}

