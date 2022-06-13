#pragma once

#include <android/log.h>

#ifndef LOG_TAG
#define LOG_TAG "LOG_TAG_not_defined"
#endif

#define ALOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define ALOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#define ALOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))

#ifndef ALOGD_IF
#define ALOGD_IF(cond, ...) \
     (((cond) == true) \
      ? ((void)ALOGD(__VA_ARGS__)) \
      : (void)0 )
#endif

#ifndef ALOGE_IF
#define ALOGE_IF(cond, ...) \
     (((cond) == true) \
      ? ((void)ALOGE(__VA_ARGS__)) \
      : (void)0 )
#endif


