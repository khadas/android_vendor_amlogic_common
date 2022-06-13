#pragma once
#include <thread>
#include <mutex>

#include "AmlNativeSubRender.h"
#include <SubtitleNativeAPI.h>

struct MyRect {
    int x;
    int y;
    int w;
    int h;
};
class NativeRender {
public:
    NativeRender() = delete;
    NativeRender(SubNativeRenderHnd win);
    ~NativeRender();

    bool render(const char *data,
                int size, AmlSubDataType type,
                int x, int y, int width, int height,
                int videoWidth, int videoHeight);

    bool clear();
    bool clearDirty();

private:
    SubNativeRenderHnd mWin;
};
