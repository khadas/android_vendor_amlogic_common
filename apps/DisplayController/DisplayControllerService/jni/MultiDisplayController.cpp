/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC droid_logic_DisplaySetting
 */

#define LOG_NDEBUG 0
#define LOG_TAG "MM"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nativehelper/JNIHelp.h>
#include <jni.h>
#include <utils/Log.h>
#include <utils/KeyedVector.h>
#include <android_runtime/AndroidRuntime.h>
#include <android_runtime/android_view_Surface.h>
#include <nativehelper/scoped_local_ref.h>
#include <nativehelper/scoped_utf_chars.h>
#include <android/native_window.h>
#include <gui/Surface.h>
#include <gui/SurfaceControl.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayId.h>
#include <gui/IGraphicBufferProducer.h>
#include <ui/GraphicBuffer.h>
#include <ui/DisplayState.h>
namespace android
{
    static bool getPhysicalDisplayIdFromPort(int port,PhysicalDisplayId& outDisplayId) {
        const std::vector<PhysicalDisplayId> ids = SurfaceComposerClient::getPhysicalDisplayIds();
        if (ids.empty()) {
         return false;
        }
        for (auto i = 0; i < ids.size(); ++i) {
        if (ids[i].getPort() == port) {
            outDisplayId = ids[i];
            return true;
        }
        }
        return false;
    }
    static jlong mirrorNative(JNIEnv *env, jobject obj, jint fromPort,
        jint toPort) {
        ALOGE("mirror native %d %d",fromPort,toPort);
        PhysicalDisplayId fromPhysicalId, toPhysicalId;
        if (!getPhysicalDisplayIdFromPort(fromPort,fromPhysicalId) || !getPhysicalDisplayIdFromPort(toPort,toPhysicalId)) {
            return false;
        }

        auto displayToken = SurfaceComposerClient::getPhysicalDisplayToken(toPhysicalId);
        if (displayToken == nullptr) {
            ALOGE("Given display id");
            return false;
        }
        auto mirrorRoot = SurfaceComposerClient::getDefault()->mirrorDisplay(fromPhysicalId);
        if (mirrorRoot == nullptr) {
            ALOGE("Failed to create a mirror for screenrecord");
            return false;
        }
        ALOGE("get the right display mirror surface");
        ui::DisplayState displayState;
        auto err = SurfaceComposerClient::getDisplayState(displayToken, &displayState);
        if (err != NO_ERROR) {
            ALOGE( "ERROR: unable to get display state\n");
            return false;
        }
        ALOGE( "displayState.layerStack %d\n",displayState.layerStack);
        SurfaceComposerClient::Transaction t;
        t.setDisplayLayerStack(displayToken, displayState.layerStack);
        t.setLayerStack(mirrorRoot, displayState.layerStack);
        t.setLayer(mirrorRoot, INT32_MAX - 1);
        t.show(mirrorRoot);
        t.apply();
        mirrorRoot->incStrong((void *)mirrorNative);
        return reinterpret_cast<jlong>(mirrorRoot.get());
    }
    static jboolean nativeSwitch(JNIEnv *env, jobject obj, jint fromPort, jint toPort) {
        ALOGE("mirror nativeSwitch %d %d",fromPort,toPort);
        PhysicalDisplayId fromPhysicalId, toPhysicalId;
        if (!getPhysicalDisplayIdFromPort(fromPort,fromPhysicalId) || !getPhysicalDisplayIdFromPort(toPort,toPhysicalId)) {
            ALOGE("cannot get port");
            return false;
        }

        auto displayFrmoToken = SurfaceComposerClient::getPhysicalDisplayToken(fromPhysicalId);
        if (displayFrmoToken == nullptr) {
            ALOGE("Given display id");
            return false;
        }
        auto displayToToken = SurfaceComposerClient::getPhysicalDisplayToken(toPhysicalId);
        if (displayToToken == nullptr) {
            ALOGE("Given display id");
            return false;
        }

        ui::DisplayState displayStateF,displayStateT;
        auto err = SurfaceComposerClient::getDisplayState(displayFrmoToken, &displayStateF);
        if (err != NO_ERROR) {
            ALOGE( "ERROR: unable to get display state\n");
            return false;
        }

        err = SurfaceComposerClient::getDisplayState(displayToToken, &displayStateT);
        if (err != NO_ERROR) {
            ALOGE( "ERROR: unable to get display state\n");
            return false;
        }
        ALOGE( "displayState.layerStack %d-->\n",displayStateF.layerStack,displayStateT.layerStack);
        SurfaceComposerClient::Transaction t;
        t.setDisplayLayerStack(displayFrmoToken, displayStateT.layerStack);
        t.setDisplayLayerStack(displayToToken, displayStateF.layerStack);
        t.apply();
        return true;
    }

    static void nativeRelease(JNIEnv *env, jclass obj, jlong nativeObject) {
        ALOGE("mirror nativeRelease");
        SurfaceControl* surfaceControl = reinterpret_cast<SurfaceControl*>(nativeObject);
        surfaceControl->decStrong((void*)mirrorNative);
        surfaceControl = nullptr;
    }
    static JNINativeMethod sMethods[] = {
        {"nativeRelease",           "(J)V",               (void*)nativeRelease},
        {"nativeMirror",           "(II)J",               (void*)mirrorNative},
        {"nativeSwitch",           "(II)Z",               (void*)nativeSwitch},
    };


    int register_android_MultiDisplayController(JNIEnv* env) {
        jclass clazz;
        const char *kClassPathName = "com/droidlogic/displaycontrollerservice/MultiDisplayController";

        clazz = env->FindClass(kClassPathName);
        if (clazz == NULL) {
            ALOGE("Native registration unable to find class '%s'",
                    kClassPathName);
            return JNI_FALSE;
        }
        int count = sizeof(sMethods) / sizeof(sMethods[0]);
        if (env->RegisterNatives(clazz, sMethods, count) < 0) {
            env->DeleteLocalRef(clazz);
            ALOGE("RegisterNatives failed for '%s'", kClassPathName);
            return JNI_FALSE;
        }

        return JNI_TRUE;
    }
} // end namespace android

using namespace android;

extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        ALOGE("GetEnv failed!");
        return result;
    }
    ALOG_ASSERT(env, "Could not retrieve the env!");

    register_android_MultiDisplayController(env);

    return JNI_VERSION_1_4;
}

