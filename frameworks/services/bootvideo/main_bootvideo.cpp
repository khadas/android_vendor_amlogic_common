/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "bootvideo"

#include <stdint.h>
#include <inttypes.h>
#include <getopt.h>
#include <binder/IServiceManager.h>
#include <cutils/properties.h>
#include <sys/resource.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include "bootvideo.h"

using namespace android;
using namespace std;

bool bootAnimationDisabled() {
    int disable = property_get_int32("debug.sf.nobootanimation", 0);
    int type = property_get_int32("persist.vendor.media.bootvideo.tsplayer", 0);
    if (type == 0 || disable > 0) {
        ALOGD("bootAnimationDisabled %d, type=%d", disable, type);
        return true;
    }

    disable = property_get_int32("ro.boot.quiescent", 0);
    if (disable > 0) {
        // Only show the bootanimation for quiescent boots if this system property is set to enabled
        if (!property_get_bool("ro.bootanim.quiescent.enabled", false)) {
            return true;
        }
    }
    return false;
}

void waitForSurfaceFlinger() {
    // TODO: replace this with better waiting logic in future, b/35253872
    int64_t waitStartTime = elapsedRealtime();
    sp<IServiceManager> sm = defaultServiceManager();
    const String16 name("SurfaceFlinger");
    const int SERVICE_WAIT_SLEEP_MS = 100;
    const int LOG_PER_RETRIES = 10;
    int retry = 0;
    while (sm->checkService(name) == nullptr) {
        retry++;
        if ((retry % LOG_PER_RETRIES) == 0) {
            ALOGW("Waiting for SurfaceFlinger, waited for %" PRId64 " ms",
                  elapsedRealtime() - waitStartTime);
        }
        usleep(SERVICE_WAIT_SLEEP_MS * 1000);
    };
    int64_t totalWaited = elapsedRealtime() - waitStartTime;
    if (totalWaited > SERVICE_WAIT_SLEEP_MS) {
        ALOGI("Waiting for SurfaceFlinger took %" PRId64 " ms", totalWaited);
    }
}

int GetPid(char* pid) {
    if (*pid == '0' && (*(pid + 1) == 'x' || *(pid + 1) == 'X')) {
        return strtol(pid,NULL,16);
    }
    return strtol(pid,NULL,10);
}

static void usage(char **argv) {
    printf("Usage: %s\n", argv[0]);
    printf("Version 0.1\n");
    printf("[options]:\n");
    printf("-i | --in           Ts file path\n");
    printf("-t | --tstype       demod:0, memory:1[default]\n");
    printf("-y | --avsync       amaster:0[default], vmaster:1, pcrmaster:2, nosync:3\n");
    printf("-c | --vtrick       none:0[default], pause:1, pause next:2, Ionly:3\n");
    printf("-v | --vcodec       unknown:0, mpeg1:1, mpeg2:2, h264:3[default], h265:4, vp9:5 avs:6 mpeg4:7\n");
    printf("-a | --acodec       unknown:0, mp2:1, mp3:2, ac3:3, eac3:4, dts:5, aac:6[default], latm:7, pcm:8\n");
    printf("-V | --vpid         video pid,default:0x100\n");
    printf("-A | --apid         audio pid,default:0x101\n");
    printf("-p | --playback     disable:0[default], enable:1\n");
    printf("-h | --help         print this usage\n");
}

void parseArgs(BootVideo * bv, int argc, char** argv) {
    int arg;
    int option_index;
    int optionChar = 0;
    int optionIndex = 0;
    const char *shortOptions = "i:t:b:y:c:v:a:V:A:p:h";
    struct option longOptions[] = {
        { "in",             required_argument,  NULL, 'i' },
        { "tstype",         required_argument,  NULL, 't' },
        { "buftype",        required_argument,  NULL, 'b' },
        { "avsync",         required_argument,  NULL, 'y' },
        { "videotrick",     required_argument,  NULL, 'c' },
        { "vcodec",         required_argument,  NULL, 'v' },
        { "acodec",         required_argument,  NULL, 'a' },
        { "vpid",           required_argument,  NULL, 'V' },
        { "apid",           required_argument,  NULL, 'A' },
        { "playback",       required_argument,  NULL, 'p' },
        { "help",           no_argument,        NULL, 'h' },
        { NULL,             0,                  NULL,  0  },
    };

    while ((arg = getopt_long(argc, argv, shortOptions, longOptions, &option_index)) != -1) {
        switch (arg) {
            case 'i':
                snprintf(bv->mTsplayParam.filePath, 128, "%s", (const char*)optarg);
                break;
            case 't':
                bv->mTsplayParam.tsType = static_cast<am_tsplayer_input_source_type>(atoi(optarg));
                break;
            case 'y':
                bv->mTsplayParam.avsyncMode = static_cast<am_tsplayer_avsync_mode>(atoi(optarg));
                break;
            case 'c':
                bv->mTsplayParam.vTrickMode = static_cast<am_tsplayer_video_trick_mode>(atoi(optarg));
                break;
            case 'v':
                bv->mTsplayParam.vCodec = static_cast<am_tsplayer_video_codec>(atoi(optarg));
                break;
            case 'a':
                bv->mTsplayParam.aCodec = static_cast<am_tsplayer_audio_codec>(atoi(optarg));
                break;
            case 'V':
                bv->mTsplayParam.vPid = GetPid(optarg); //char* --> int
                break;
            case 'A':
                bv->mTsplayParam.aPid = GetPid(optarg); //char* --> int
                break;
            case 'p':
                bv->mTsplayParam.emPlaybackType = static_cast<am_tsplayer_playback_type>(atoi(optarg));
                break;
            case 'h':
                usage(argv);
                exit(-1);

            default:
                break;
        }
    }
}

int main(int argc, char **argv)
{
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);

    bool noBootAnimation = bootAnimationDisabled();
    ALOGI_IF(noBootAnimation,  "boot video disabled");
    if (!noBootAnimation) {
        BootVideo * boot = new BootVideo();
        parseArgs(boot, argc, argv);
        waitForSurfaceFlinger();
        boot->play();
        ALOGD("Boot video finish.");
    }
    return 0;
}
