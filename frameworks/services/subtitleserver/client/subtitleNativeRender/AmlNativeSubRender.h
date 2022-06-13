#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Actually this is the same as ANativeWindow_Buffer, we redefine this
 * to make treble compile/link happy
 */
typedef struct SubNativeRenderBuffer {
    /// The number of pixels that are shown horizontally.
    int32_t width;

    /// The number of pixels that are shown vertically.
    int32_t height;

    /// The number of *pixels* that a line in the buffer takes in
    /// memory. This may be >= width.
    int32_t stride;

    /// The format of the buffer. One of AHARDWAREBUFFER_FORMAT_*
    int32_t format;

    /// The actual bits.
    void* bits;

    /// Do not touch.
    uint32_t reserved[6];
} SubNativeRenderBuffer;

typedef void *SubNativeRenderHnd;

// QTone Data callback
// after calling this cb, the 'buf' will released, so must copy out!
typedef void (*QToneDataCallback)(int session, const char*buf, int size);

typedef struct SubNativeRenderCallback {
    int32_t (*lock)(SubNativeRenderHnd hnd, SubNativeRenderBuffer *buf);
    int32_t (*unlockAndPost)(SubNativeRenderHnd hnd);
    int32_t (*setBuffersGeometry)(SubNativeRenderHnd hnd, int w, int h, int format);
    int64_t (*getPts)(int sessionId);
} SubNativeRenderCallback;


bool aml_RegisterNativeWindowCallback(SubNativeRenderCallback cb);
bool aml_AttachSurfaceWindow(int sessionId, SubNativeRenderHnd win);
bool aml_SetSubtitleSessionEnabled(int sessionId, bool enabled);
bool aml_GetSubtitleSessionEnabled(int sessionId);
bool aml_SetSubtitleURI(int sessionId, const char*url);
bool aml_RegisterQtoneDataCb(int sessionId, QToneDataCallback cb);

#ifdef __cplusplus
}
#endif

