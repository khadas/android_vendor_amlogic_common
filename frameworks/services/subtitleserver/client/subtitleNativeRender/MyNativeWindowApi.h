#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "AmlNativeSubRender.h"

int32_t NativeWindowLock(SubNativeRenderHnd hnd, SubNativeRenderBuffer *buf);
int32_t NativeWindowUnlockAndPost(SubNativeRenderHnd hnd);
int32_t NativeWindowSetBuffersGeometry(SubNativeRenderHnd hnd, int w, int h, int format);

