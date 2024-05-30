/*
 * Copyright (C) 2016 The Android Open Source Project
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

#define LOG_TAG "ScreenControlUnitTest"
#include <dlfcn.h>
#include <gtest/gtest.h>
#include <log/log.h>
#include <string>
#include <thread>
#include <time.h>
#include "ScreenControlClient.h"
using namespace android;
#define RUN_COUNT 50000

#define DIFF_TIMES_US 43200000000
#define NO_DATA_TIMEOUT_US 5000000

inline int64_t getNowTimesUs() {
    struct timespec now;
    clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t now_time = (int64_t)now.tv_sec * 1000 * 1000 + (int64_t)now.tv_nsec / 1000;
    return now_time;
}

class AvcRecTester : public ScreenControlClient::AvcRecordCallback {
public:
    AvcRecTester() {
        ALOGD("AvcRecTester this =%p", this);
        mFirstPts = 0;
        mLastPts = 0;
        client = ScreenControlClient::getInstance();
    }

    ~AvcRecTester() {
        ALOGD("~AvcRecTester this =%p", this);
        client = nullptr;
    }
    bool start(int left, int top, int right, int bottom, int width, int height, int source_type, int32_t frame_rate,
               int32_t bit_rate) {
        int ret = client->startAvcScreenRecord(width, height, frame_rate, bit_rate, source_type);
        client->setAvcCallback(this);
        return ret == 0 ? true : false;
    }
    void stop() {
        ALOGD("AvcRecTester stop in this =%p", this);
        client->forceStop();
        ALOGD("AvcRecTester stop out this =%p", this);
    }
    void onAvcDataArouse(void* data, int32_t size, int32_t frameType, int64_t pts) {
        printf("onAvcDataArouse frameType=%d,mFirstPts = %lld,pts =%lld,diff =%lld,pid=%d\n", frameType, mFirstPts, pts,
               (pts - mFirstPts), getpid());
        if (mFirstPts == 0)
            mFirstPts = pts;
        mLastPts = pts;
    }
    int64_t getLastPts() { return mLastPts; }
    int64_t getFirstPts() { return mFirstPts; }
    int64_t getDiffPts() { return (mFirstPts == 0 || mLastPts == 0) ? 0 : (mLastPts - mFirstPts); }

private:
    int64_t mFirstPts;
    int64_t mLastPts;
    ScreenControlClient* client;
};

class YuvRecTester : public ScreenControlClient::YuvRecordCallback {
public:
    YuvRecTester() {
        mFirstPts = 0;
        mLastPts = 0;
        client = ScreenControlClient::getInstance();
    }
    ~YuvRecTester() {}
    bool start(int left, int top, int right, int bottom, int width, int height, int source_type, int32_t frame_rate) {
        int ret = client->startYuvScreenRecord(width, height, frame_rate, source_type);
        client->setYuvCallback(this);
        return ret == 0 ? true : false;
    }
    void stop() { client->forceStop(); }
    void onYuvDataArouse(void* data, int32_t size) {
        int64_t pts = getNowTimesUs();
        printf("onYuvDataArouse mFirstPts = %lld,pts =%lld,diff =%lld,pid=%d\n", mFirstPts, pts, (pts - mFirstPts),
               getpid());
        if (mFirstPts == 0)
            mFirstPts = pts;
        mLastPts = pts;
    }
    int64_t getLastPts() { return mLastPts; }
    int64_t getFirstPts() { return mFirstPts; }
    int64_t getDiffPts() { return (mFirstPts == 0 || mLastPts == 0) ? 0 : (mLastPts - mFirstPts); }

private:
    int64_t mFirstPts;
    int64_t mLastPts;
    ScreenControlClient* client;
};

class ScreenControlUnitTest : public ::testing::Test {
public:
    ~ScreenControlUnitTest() {}

    virtual void SetUp() override {
        ::testing::Test::SetUp();
        client = ScreenControlClient::getInstance();
        ASSERT_NE(client, nullptr);
    }

    virtual void TearDown() override {}

    ScreenControlClient* client = nullptr;
};

TEST_F(ScreenControlUnitTest, testLoopScreenCap) {
    ALOGD("testLoopScreenCap begin");
    printf("testLoopScreenCap begin\n");
    for (int i = 0; i < RUN_COUNT; i++) {
        char* buffer = nullptr;
        int bufferSize = 0;
        int ret = client->startScreenCapBuffer(0, 0, 1920, 1080, 1920, 1080, 1, (void**)&buffer, &bufferSize);
        ASSERT_EQ(ret, 0);
        ASSERT_NE(buffer, nullptr);
        ASSERT_GT(bufferSize, 0);
        printf("finish screen cap i = %d\n", i);
        delete[] buffer;
    }
    ALOGD("testLoopScreenCap success");
    printf("testLoopScreenCap success\n");
}

TEST_F(ScreenControlUnitTest, testLoopTsRecord) {
    ALOGD("testLoopTsRecord begin");
    printf("testLoopTsRecord begin\n");
    for (int i = 0; i < RUN_COUNT; i++) {
        int ret = client->startScreenRecord(0, 0, 1920, 1080, 1920, 1080, 30, 4000000, 1, 1, "testLoopTsRecord");
        ASSERT_EQ(ret, 0);
        printf("finish ts screen record i = %d\n", i);
    }
    ALOGD("testLoopTsRecord success");
    printf("testLoopTsRecord success\n");
}

TEST_F(ScreenControlUnitTest, testLoopAvcRecord) {
    ALOGD("testLoopAvcRecord begin");
    printf("testLoopAvcRecord begin\n");
    int64_t diff = 1000000;
    for (int i = 0; i < RUN_COUNT; i++) {
        sp<AvcRecTester> recorder = new AvcRecTester();
        bool ret = recorder->start(0, 0, 1920, 1080, 1920, 1080, 1, 30, 4000000);
        ASSERT_TRUE(ret);
        int64_t firstTimeUs = getNowTimesUs();
        while (1) {
            int64_t diffpts = recorder->getDiffPts();
            ;
            int64_t firstPts = recorder->getFirstPts();
            int64_t lastPts = recorder->getLastPts();
            if (firstPts == 0) {
                int64_t nowTimeUs = getNowTimesUs();
                ASSERT_LE((nowTimeUs - firstTimeUs), NO_DATA_TIMEOUT_US);
            }
            if (diffpts >= diff) {
                printf("testLoopAvcRecord firstPts =%lld,lastPts=%lld,diffpts=%lld\n", firstPts, lastPts, diffpts);
                break;
            }
            usleep(5 * 1000); // 5ms
        }
        recorder->stop();
        printf("finish avc screen record i =%d\n", i);
    }
    ALOGD("testLoopAvcRecord success");
    printf("testLoopAvcRecord success\n");
}

TEST_F(ScreenControlUnitTest, testLoopYuvRecord) {
    ALOGD("testLoopYuvRecord begin");
    printf("testLoopYuvRecord begin\n");
    for (int i = 0; i < RUN_COUNT; i++) {
        sp<YuvRecTester> recorder = new YuvRecTester();
        bool ret = recorder->start(0, 0, 1920, 1080, 1920, 1080, 1, 30);
        ASSERT_TRUE(ret);
        int64_t diff = 1000000;
        int64_t firstTimeUs = getNowTimesUs();
        while (1) {
            int64_t diffpts = recorder->getDiffPts();
            int64_t firstPts = recorder->getFirstPts();
            int64_t lastPts = recorder->getLastPts();
            if (firstPts == 0) {
                int64_t nowTimeUs = getNowTimesUs();
                ASSERT_LE((nowTimeUs - firstTimeUs), NO_DATA_TIMEOUT_US);
            }
            if (diffpts >= diff) {
                printf("testLoopYuvRecord firstPts =%lld,lastPts=%lld,diffpts=%lld\n", firstPts, lastPts, diffpts);
                break;
            }
            usleep(5 * 1000); // 5ms
        }
        recorder->stop();
        printf("finish avc screen record i =%d\n", i);
    }
    ALOGD("testLoopYuvRecord success");
    printf("testLoopYuvRecord success\n");
}

TEST_F(ScreenControlUnitTest, testTsRecordScreenCapture) {
    ALOGD("testTsRecordScreenCapture begin");
    printf("testTsRecordScreenCapture begin\n");
    int64_t first = getNowTimesUs();
    auto TsRecordFunc = [](int64_t first) -> void {
        int count = 0;
        while (1) {
            ScreenControlClient* client = ScreenControlClient::getInstance();
            int ret = client->startScreenRecord(0, 0, 1920, 1080, 1920, 1080, 30, 4000000, 1, 1, "testLoopTsRecord");
            ASSERT_EQ(ret, 0);
            printf("testTsRecordScreenCapture finish ts screen record count = %d\n", count);
            count++;
            int64_t nowTimeUs = getNowTimesUs();
            if ((nowTimeUs - first) > DIFF_TIMES_US) {
                printf("testTsRecordScreenCapture nowTimeUs = %lld,first =%lld,diff =%lld\n", nowTimeUs, first,
                       (nowTimeUs - first));
                break;
            }
        }
    };
    auto ScreenCaptureFunc = [](int64_t first) -> void {
        int count = 0;
        while (1) {
            char* buffer = nullptr;
            int bufferSize = 0;
            ScreenControlClient* client = ScreenControlClient::getInstance();
            int ret = client->startScreenCapBuffer(0, 0, 1920, 1080, 1920, 1080, 1, (void**)&buffer, &bufferSize);
            ASSERT_EQ(ret, 0);
            ASSERT_NE(buffer, nullptr);
            ASSERT_GT(bufferSize, 0);
            printf("testTsRecordScreenCapture  screen cap count = %d\n", count);
            delete[] buffer;
            count++;
            int64_t nowTimeUs = getNowTimesUs();
            if ((nowTimeUs - first) > DIFF_TIMES_US) {
                printf("testTsRecordScreenCapture nowTimeUs = %lld,first =%lld,diff =%lld\n", nowTimeUs, first,
                       (nowTimeUs - first));
                break;
            }
        }
    };
    std::thread tsRecordThread(TsRecordFunc, first);
    std::thread ScreenCaptureThread(ScreenCaptureFunc, first);
    tsRecordThread.join();
    ScreenCaptureThread.join();
    ALOGD("testTsRecordScreenCapture success");
    printf("testTsRecordScreenCapture success\n");
}

TEST_F(ScreenControlUnitTest, testAvcRcordScreenCapture) {
    ALOGD("testAvcRcordScreenCapture begin");
    printf("testAvcRcordScreenCapture begin\n");
    int64_t first = getNowTimesUs();
    auto AvcRecordFunc = [](int64_t first) -> void {
        int count = 0;
        while (1) {
            sp<AvcRecTester> recorder = new AvcRecTester();
            bool ret = recorder->start(0, 0, 1920, 1080, 1920, 1080, 1, 30, 4000000);
            ASSERT_TRUE(ret);
            int64_t diff = 1000000;
            int64_t firstTimeUs = getNowTimesUs();
            while (1) {
                int64_t diffpts = recorder->getDiffPts();
                int64_t firstPts = recorder->getFirstPts();
                int64_t lastPts = recorder->getLastPts();
                if (firstPts == 0) {
                    int64_t nowTimeUs = getNowTimesUs();
                    ASSERT_LE((nowTimeUs - firstTimeUs), NO_DATA_TIMEOUT_US);
                }
                if ((diffpts >= diff) || ((lastPts - first) > DIFF_TIMES_US)) {
                    printf("testAvcRcordScreenCapture firstPts =%lld,lastPts=%lld,diffpts=%lld\n", firstPts, lastPts,
                           diffpts);
                    break;
                }
                usleep(5 * 1000); // 5ms
            }
            recorder->stop();
            printf("testAvcRcordScreenCapture finish avc screen record count =%d\n", count);
            count++;
            int64_t nowTimeUs = getNowTimesUs();
            if ((nowTimeUs - first) > DIFF_TIMES_US) {
                printf("testAvcRcordScreenCapture nowTimeUs = %lld,first =%lld,diff =%lld\n", nowTimeUs, first,
                       (nowTimeUs - first));
                break;
            }
        }
    };
    auto ScreenCaptureFunc = [](int64_t first) -> void {
        int count = 0;
        ScreenControlClient* client = ScreenControlClient::getInstance();
        while (1) {
            char* buffer = nullptr;
            int bufferSize = 0;
            int ret = client->startScreenCapBuffer(0, 0, 1920, 1080, 1920, 1080, 1, (void**)&buffer, &bufferSize);
            ASSERT_EQ(ret, 0);
            ASSERT_NE(buffer, nullptr);
            ASSERT_GT(bufferSize, 0);
            printf("testAvcRcordScreenCapture  screen cap count = %d\n", count);
            delete[] buffer;
            count++;
            int64_t nowTimeUs = getNowTimesUs();
            if ((nowTimeUs - first) > DIFF_TIMES_US) {
                printf("testAvcRcordScreenCapture nowTimeUs = %lld,first =%lld,diff =%lld\n", nowTimeUs, first,
                       (nowTimeUs - first));
                break;
            }
        }
    };
    std::thread avcRecordThread(AvcRecordFunc, first);
    std::thread ScreenCaptureThread(ScreenCaptureFunc, first);
    avcRecordThread.join();
    ScreenCaptureThread.join();
    ALOGD("testAvcRcordScreenCapture success");
    printf("testAvcRcordScreenCapture success\n");
}

TEST_F(ScreenControlUnitTest, testYuvRcordScreenCapture) {
    ALOGD("testYuvRcordScreenCapture begin");
    printf("testYuvRcordScreenCapture begin\n");
    int64_t first = getNowTimesUs();
    auto YuvRecordFunc = [](int64_t first) -> void {
        int count = 0;
        while (1) {
            sp<YuvRecTester> recorder = new YuvRecTester();
            bool ret = recorder->start(0, 0, 1920, 1080, 1920, 1080, 1, 30);
            ASSERT_TRUE(ret);
            int64_t diff = 1000000;
            int64_t firstTimeUs = getNowTimesUs();
            while (1) {
                int64_t diffpts = recorder->getDiffPts();
                int64_t firstPts = recorder->getFirstPts();
                int64_t lastPts = recorder->getLastPts();
                if (firstPts == 0) {
                    int64_t nowTimeUs = getNowTimesUs();
                    ASSERT_LE((nowTimeUs - firstTimeUs), NO_DATA_TIMEOUT_US);
                }
                if ((diffpts >= diff) || ((lastPts - first) > DIFF_TIMES_US)) {
                    printf("testYuvRcordScreenCapture firstPts =%lld,lastPts=%lld,diffpts=%lld\n", firstPts, lastPts,
                           diffpts);
                    break;
                }
                usleep(5 * 1000); // 5ms
            }
            recorder->stop();
            printf("testYuvRcordScreenCapture finish avc screen record count =%d\n", count);
            count++;
            int64_t nowTimeUs = getNowTimesUs();
            if ((nowTimeUs - first) > DIFF_TIMES_US) {
                printf("testYuvRcordScreenCapture nowTimeUs = %lld,first =%lld,diff =%lld\n", nowTimeUs, first,
                       (nowTimeUs - first));
                break;
            }
        }
    };
    auto ScreenCaptureFunc = [](int64_t first) -> void {
        int count = 0;
        ScreenControlClient* client = ScreenControlClient::getInstance();
        while (1) {
            char* buffer = nullptr;
            int bufferSize = 0;
            int ret = client->startScreenCapBuffer(0, 0, 1920, 1080, 1920, 1080, 1, (void**)&buffer, &bufferSize);
            ASSERT_EQ(ret, 0);
            ASSERT_NE(buffer, nullptr);
            ASSERT_GT(bufferSize, 0);
            printf("testYuvRcordScreenCapture  screen cap count = %d\n", count);
            delete[] buffer;
            count++;
            int64_t nowTimeUs = getNowTimesUs();
            if ((nowTimeUs - first) > DIFF_TIMES_US) {
                printf("testYuvRcordScreenCapture nowTimeUs = %lld,first =%lld,diff =%lld\n", nowTimeUs, first,
                       (nowTimeUs - first));
                break;
            }
        }
    };
    std::thread yuvRecordThread(YuvRecordFunc, first);
    std::thread ScreenCaptureThread(ScreenCaptureFunc, first);
    yuvRecordThread.join();
    ScreenCaptureThread.join();
    ALOGD("testYuvRcordScreenCapture success");
    printf("testYuvRcordScreenCapture success\n");
}

TEST_F(ScreenControlUnitTest, testLongTimeTsRecord) {
    ALOGD("testLongTimeTsRecord begin");
    printf("testLongTimeTsRecord begin\n");
    int ret = client->startScreenRecord(0, 0, 1920, 1080, 1920, 1080, 30, 4000000, 43200, 1, "testLoopTsRecord");
    ASSERT_EQ(ret, 0);
    ALOGD("testLongTimeTsRecord success");
    printf("testLongTimeTsRecord success\n");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int status = RUN_ALL_TESTS();
    ALOGE("Test status = %d", status);
    return status;
}
