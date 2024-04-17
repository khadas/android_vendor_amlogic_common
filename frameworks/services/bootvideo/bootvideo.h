
#ifndef ANDROID_BOOTANIMATION_H
#define ANDROID_BOOTANIMATION_H

#include <gui/IProducerListener.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/ISurfaceComposer.h>

#include <AmTsPlayer.h>


using namespace android;
using namespace std;

/*TS Playback Switch*/
typedef enum {
    TS_PLAYBACK_DISABLE = 0,    // Not playback when file eof
    TS_PLAYBACK_ENABLE = 1,     // Playback when file eof
} am_tsplayer_playback_type;

struct tsplay_param {
    char filePath[128];
    am_tsplayer_video_codec vCodec;
    am_tsplayer_audio_codec aCodec;
    int32_t vPid;
    int32_t aPid;
    am_tsplayer_playback_type emPlaybackType;
    am_tsplayer_input_source_type tsType;
    am_tsplayer_avsync_mode avsyncMode;
    am_tsplayer_video_trick_mode vTrickMode;
};

class BootVideo {
public:
    BootVideo();
    int play();

    struct tsplay_param mTsplayParam;

private:
    bool CreateSurface();
    bool CreateVideoTunnelId(int* id);
    bool checkExit();
    static void video_callback(void *user_data, am_tsplayer_event *event);

    //int mPlayStatus = -1;
    am_tsplayer_handle mSession;
    sp<SurfaceComposerClient> mComposerClient = NULL;
    sp<SurfaceControl> mControl = NULL;
    sp<Surface> mSurface = NULL;
    bool mWaitPlayFinish;
    uint64_t mLastPlayTs;
    uint64_t mLastGetTs;
};
#endif //ANDROID_BOOTANIMATION_H

