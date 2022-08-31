#define LOG_TAG "Presentation"

#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <utils/Looper.h>

#include <utils/Timers.h>
#include <time.h>

#include "ParserFactory.h"
#include "AndroidHidlRemoteRender.h"
#include "Presentation.h"

#define MAX_ALLOWED_EXT_SPU_NUM 1000
#define MAX_ALLOWED_STREAM_SPU_NUM 10

#define TSYNC_32_BIT_PTS 0xFFFFFFFF
#define HIGH_32_BIT_PTS 0xFFFFFFFF


static const int DVB_TIME_MULTI = 90;

static const int64_t FIVE_SECONDS_NS = 5*1000*1000*1000LL;
static const int64_t ONE_SECONDS_NS = 1*1000*1000*1000LL;

static const int64_t ADDJUST_VERY_SMALL_PTS_MS = 5*1000LL;
static const int64_t ADDJUST_NO_PTS_MS = 4*1000LL;
static bool mSubtitlePts32Bit = false;
static inline int64_t convertDvbTime2Ns(int64_t dvbMillis) {
    return ms2ns(dvbMillis)/DVB_TIME_MULTI; // dvbTime is multi 90.
}

static inline int64_t convertNs2DvbTime(int64_t ns) {
    return ns2ms(ns)*DVB_TIME_MULTI;
}

static bool cmpSpu(std::shared_ptr<AML_SPUVAR> a, std::shared_ptr<AML_SPUVAR> b) {
    return a->pts < b->pts;
}

static inline int64_t fixSpuDelay(int64_t pts, int64_t delay) {
    if (delay <= 0 || delay <= pts+500*DVB_TIME_MULTI) {
        return pts + (ADDJUST_NO_PTS_MS * DVB_TIME_MULTI);
    }

    return delay;
}

// Typically, only one spu, but some case maybe 2 or 3
static std::shared_ptr<AML_SPUVAR> getShowingSpuFromList(
        std::list<std::shared_ptr<AML_SPUVAR>> &list) {
    if (list.size() == 0) return nullptr;

    std::shared_ptr<AML_SPUVAR> spu = list.front();

    // only show the last picture, not conbine!
    if (!spu->isSimpleText) {
        return list.back();
    }

    // multiline, need combine all the subtitles! do combine here!
    if (list.size() > 1) {
        int totalSize = 0;
        std::for_each(list.begin(), list.end(), [&](std::shared_ptr<AML_SPUVAR> &s) {
            //w.dump(prefix);
            totalSize += spu->buffer_size +1; // addinital newline.
        });

        std::shared_ptr<AML_SPUVAR> newCue =  std::shared_ptr<AML_SPUVAR>(new AML_SPUVAR());
        // shandow copy.
        memcpy(newCue.get(), list.front().get(), sizeof(AML_SPUVAR));

        // combine lines
        newCue->spu_data = new uint8_t[totalSize+1](); //additional '\0'
        std::for_each(list.begin(), list.end(), [&](std::shared_ptr<AML_SPUVAR> &s) {
            strcat((char *)newCue->spu_data, (const char *)s->spu_data);
            strcat((char *)newCue->spu_data, "\n");
        });
        newCue->spu_data[totalSize] = 0;
        spu = newCue;
    }

    return spu;
}

/* Some cue may showing at current pts. but cue start pts may not the same
 *  we filter this, when the cue is too old, but not run out of it's life time.
 */
void reAssembleSpuList(std::list<std::shared_ptr<AML_SPUVAR>> &list) {
    if (list.size() <= 1) return;

    std::shared_ptr<AML_SPUVAR> spu = list.back();

    for (auto it = list.begin(); it != list.end();) {
       uint64_t ptsDiff = spu->pts - (*it)->pts;

        if (ptsDiff > convertNs2DvbTime(ms2ns(200))) {
            it = list.erase(it);
        } else {
            it++;
        }
    }
}



Presentation::Presentation(std::shared_ptr<Display> disp) :
    mCurrentPresentRelativeTime(0),
    mStartPresentMonoTimeNs(0),
    mStartTimeModifier(0)
{
    std::unique_lock<std::mutex> autolock(mMutex);
    mDisplay = disp;
    mRender = std::shared_ptr<Render>(new AndroidHidlRemoteRender/*SkiaRenderI*/(disp));
    mMsgProcess = nullptr; // only access in threadloop.
}

Presentation::~Presentation() {
    //TODO: do we need poke thread exit immediately? by post a message?
    //      then need add a lock, for protect access mLooper in multi-thread
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        //delete mMsgProcess;
        // We do not delete here, let sp pointer do this!
        mMsgProcess->join();
        mMsgProcess = nullptr; // late sp delete it.
    }
}

bool Presentation::notifyStartTimeStamp(int64_t startTime) {
    mStartTimeModifier = convertDvbTime2Ns(startTime);

    ALOGD("notifyStartTimeStamp: %lld", startTime);
    return true;
}


bool Presentation::syncCurrentPresentTime(int64_t pts) {
    if (mSubtitlePts32Bit) {
        pts &= TSYNC_32_BIT_PTS;
    }

    mCurrentPresentRelativeTime = convertDvbTime2Ns(pts);

    // the time
    mStartPresentMonoTimeNs = systemTime(SYSTEM_TIME_MONOTONIC) - convertDvbTime2Ns(pts);

    // Log information, do not rush out too much, throttle to 1/300.
    static int i = 0;
    if (i++%300 == 0) {
        ALOGD("pts:%lld mCurrentPresentRelativeTime:%lld  current:%lld",
            pts, ns2ms(mCurrentPresentRelativeTime), ns2ms(systemTime(SYSTEM_TIME_MONOTONIC)));
    }

    // external subtitle, just polling and showing the subtitle.
    if (mParser != nullptr && mParser->isExternalSub()) {
        std::unique_lock<std::mutex> autolock(mMutex);
        if (mMsgProcess != nullptr) {
            mMsgProcess->notifyMessage(MessageProcess::MSG_PTS_TIME_CHECK_SPU);
        }
    }

    return true;
}


bool Presentation::startPresent(std::shared_ptr<Parser> parser) {
    std::unique_lock<std::mutex> autolock(mMutex);
    mParser = parser;
    mMsgProcess = new MessageProcess(this, parser->isExternalSub());
    return true;
}
bool Presentation::stopPresent() {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        //delete mMsgProcess;
        // We do not delete here, let sp pointer do this!
        mMsgProcess->join();
        mMsgProcess->decStrong(nullptr);
        mMsgProcess = nullptr; // late sp delete it.
    }
    return true;
}


//ssa text subtitle may have two continuous packet while have same pts
bool Presentation::combinSamePtsSubtitle(std::shared_ptr<AML_SPUVAR> spu1, std::shared_ptr<AML_SPUVAR> spu2) {
    if (spu1 != nullptr && spu2 != nullptr) {
        if (spu1->isExtSub || spu1->isImmediatePresent ) {
            return false;
        }

        if (spu1->pts == spu2->pts) {
            int dataLen1 = spu1->buffer_size;
            int dataLen2 = spu2->buffer_size;
            char* data = (char*) malloc(dataLen1);
            memset(data, 0, dataLen1);
            strcpy(data, (char *)spu1->spu_data);

            //resize new buffer size
            int size = dataLen1 + 1 + dataLen2 + 1;
            ALOGD("combine pts:%lld,spu1 size:%d, spu2 size:%d, new buffer size:%d,", spu1->pts, dataLen1, dataLen2, size);
            spu1->spu_data = new uint8_t[size]();
            strcat((char *)spu1->spu_data, data);
            strcat((char *)spu1->spu_data, "\n");
            strcat((char *)spu1->spu_data, (char *)spu2->spu_data);
            strcat((char *)spu1->spu_data, "\0");
            spu1->buffer_size = size;
            free(data);
            return true;
        }
    }
    return false;
}

bool static inline isMore32Bit(int64_t pts) {
    if (((pts >> 32) & HIGH_32_BIT_PTS) > 0) {
        return true;
    }

    return false;
}

//tsync only support 32 bit pts, so if video pts from tsync
//is more than 32 bits, subtitle pts will change to 32 bit pts.
//mediasync support 64 bit pts which don't need change.
bool Presentation::compareBitAndSyncPts(std::shared_ptr<AML_SPUVAR> spu, int64_t vPts) {
    if (spu->pts <= 0 || vPts <= 0) {
        return false;
    }

    if (isMore32Bit(spu->pts) && !isMore32Bit(vPts)) {
        ALOGD("SUB PTS and video pts bits diff, before subpts: %llu, vpts:%llu", spu->pts, vPts);
        spu->pts &= TSYNC_32_BIT_PTS;
        spu->m_delay &= TSYNC_32_BIT_PTS;
        return true;
    }

    return false;
}


bool Presentation::resetForSeek() {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        mMsgProcess->notifyMessage(MessageProcess::MSG_RESET_MESSAGE_QUEUE);
    }
    return true;
}

void Presentation::notifySubdataAdded() {
    std::unique_lock<std::mutex> autolock(mMutex);
    if (mMsgProcess != nullptr) {
        mMsgProcess->notifyMessage(MessageProcess::MSG_PTS_TIME_CHECK_SPU);
    }
}

void Presentation::dump(int fd, const char *prefix) {
    dprintf(fd, "%s Presentation:\n", prefix);
    dprintf(fd, "%s   CurrentPresentRelativeTime[dvb time]: %d\n",
            prefix, convertNs2DvbTime(mCurrentPresentRelativeTime));
    dprintf(fd, "%s   StartPresentMonoTime[dvb time]: %d\n",
            prefix, convertNs2DvbTime(mStartPresentMonoTimeNs));
    dprintf(fd, "%s   StartTimeModifier[dvb time]: %d\n",
            prefix, convertNs2DvbTime(mStartTimeModifier));
    dprintf(fd, "\n");
    if (mParser != nullptr) {
        dprintf(fd, "%s  Parser : %p\n", prefix, mParser.get());
    }
    if (mDisplay != nullptr) {
        dprintf(fd, "%s  Display: %p\n", prefix, mDisplay.get());
    }
    if (mRender != nullptr) {
        dprintf(fd, "%s  Render : %p\n", prefix, mRender.get());
    }
    dprintf(fd, "\n");

    {
        dprintf(fd, "%s   Ready for showing SPUs:\n", prefix);
        auto it = mEmittedShowingSpu.begin();
        for (; it != mEmittedShowingSpu.end();  ++it) {
            if ((*it) != nullptr) {
                    (*it)->dump(fd, "      ");
            }
        }
    }
    dprintf(fd, "\n");

    {
        dprintf(fd, "%s   Showed but not deleted SPUs:\n", prefix);
        auto it = mEmittedFaddingSpu.begin();
        for (; it != mEmittedFaddingSpu.end();  ++it) {
            if ((*it) != nullptr) {
                    (*it)->dump(fd, "      ");
            }
        }
    }
}

Presentation::MessageProcess::MessageProcess(Presentation *present, bool isExtSub) {
    mRequestThreadExit = false;
    mPresent = present;
    mIsExtSub = isExtSub;

    // hold a reference for RefBase object
    // we move the inc here, before the thread started, to avoid multi-thread problem
    incStrong(nullptr);
    mLooperThread = std::shared_ptr<std::thread>(new std::thread(&MessageProcess::looperLoop, this));
}

Presentation::MessageProcess::~MessageProcess() {
    mLastShowingSpu = nullptr;
    mPresent = nullptr;
}

void Presentation::MessageProcess::join() {
    mRequestThreadExit = true;
    if (mLooper) {
        mLooper->removeMessages(this, MSG_PTS_TIME_CHECK_SPU);
        mLooper->wake();
    }
    mLooperThread->join();
    if (mLooper != nullptr) {
        mLooper->decStrong(nullptr);
    }
    mLooper = nullptr;
}

bool Presentation::MessageProcess::notifyMessage(int what) {
    if (mLooper) {
        mLooper->sendMessage(this, Message(what));
    }
    return true;
}

void Presentation::MessageProcess::handleMessage(const Message& message) {
    return mIsExtSub ? handleExtSub(message) : handleStreamSub(message);
}


static std::list<std::shared_ptr<AML_SPUVAR>> computeShowingSpuList(
    std::list<std::shared_ptr<AML_SPUVAR>> &list, int64_t timestamp) {

    std::list<std::shared_ptr<AML_SPUVAR>> showingList;
    std::shared_ptr<AML_SPUVAR> spuLess;
    std::shared_ptr<AML_SPUVAR> spuBig;
    //spu list is sorted by pts
    // TODO: quick search
    for (auto it=list.begin(); it != list.end(); ++it) {
        uint64_t pts = convertDvbTime2Ns((*it)->pts);
        uint64_t ptsEnd = convertDvbTime2Ns((*it)->m_delay);
        if (timestamp >= pts && timestamp <= ptsEnd) {
            showingList.push_back(*it);
        }

        if (ptsEnd > timestamp) break;
    }
    return showingList;
}



void Presentation::MessageProcess::handleExtSub(const Message& message) {
    switch (message.what) {
        case MSG_PTS_TIME_CHECK_SPU: {
            uint64_t timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;

            // external sub always decoded all the subtitle items.
            mLooper->removeMessages(this, MSG_PTS_TIME_CHECK_SPU);

            //1. collect all decoded items! save in mEmittedShowingSpu!
            std::shared_ptr<AML_SPUVAR> spu;
            std::list<std::shared_ptr<AML_SPUVAR>> &spuList = mPresent->mEmittedShowingSpu;
            while ((spu = mPresent->mParser->tryConsumeDecodedItem()) != nullptr) {
                spu->m_delay = fixSpuDelay(spu->pts, spu->m_delay); // TODO: move to spu construct.
                spuList.push_back(spu);
                spuList.sort(cmpSpu);
            }

            // 2. find need showing SPUs by pts.
            std::list<std::shared_ptr<AML_SPUVAR>>  showing = computeShowingSpuList(spuList, timestamp);


            if (showing.size() > 0) {
                spu = getShowingSpuFromList(showing);

                // post to show.
                if (mLastShowingSpu != spu) {
                    mPresent->mEmittedFaddingSpu.clear();
                    mPresent->mRender->showSubtitleItem(spu, mPresent->mParser->getParseType());
                }
                mLastShowingSpu = spu;
            } else {
                if (mLastShowingSpu != nullptr) {
                    mPresent->mRender->resetSubtitleItem();
                }
                mLastShowingSpu = nullptr;
            }
        }
        break;

        case MSG_RESET_MESSAGE_QUEUE:
            mPresent->mEmittedFaddingSpu.clear();
            mPresent->mRender->resetSubtitleItem();
            break;

        default:
        break;
    }
}



// Stream sub decode and show the subtitle when received data .
void Presentation::MessageProcess::handleStreamSub(const Message& message) {
    switch (message.what) {
        case MSG_PTS_TIME_CHECK_SPU: {
            mLooper->removeMessages(this, MSG_PTS_TIME_CHECK_SPU);
            std::shared_ptr<AML_SPUVAR> spu = mPresent->mParser->tryConsumeDecodedItem();
            if (spu != nullptr) {
                // has subtitle to show! Post to render list
                ALOGD("Got  SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%lld(%lld) duration:%lld(%lld) data:%p(%p)",
                        ns2ms(mPresent->mCurrentPresentRelativeTime),
                        ns2ms(mPresent->mStartTimeModifier),
                        spu->pts, spu->pts/DVB_TIME_MULTI,
                        spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                        spu->spu_data, spu->spu_data);
                mPresent->mEmittedShowingSpu.push_back(spu);
                mPresent->mEmittedShowingSpu.sort(cmpSpu);
                // if emitt to much spu and not consumed, we delete the oldest, save memory
                while (mPresent->mEmittedShowingSpu.size() > MAX_ALLOWED_STREAM_SPU_NUM) {
                    mPresent->mEmittedShowingSpu.pop_front();
                }
                if (isMore32Bit(spu->pts)) {
                    mSubtitlePts32Bit = false;
                } else {
                    mSubtitlePts32Bit = true;
                }
            }

            // handle presentation showing.
            if (mPresent->mEmittedShowingSpu.size() > 0) {
                spu = mPresent->mEmittedShowingSpu.front();
                uint64_t timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;

                //in case seek done, then show out-of-date subtitle
                while (spu != nullptr && spu->m_delay > 0 && (ns2ms(timestamp) > spu->m_delay/DVB_TIME_MULTI) && spu->isExtSub) {
                    mPresent->mEmittedShowingSpu.pop_front();
                    spu = mPresent->mEmittedShowingSpu.front();
                    timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;
                    if (spu == nullptr) {// if the spu is nullptr then sendmessage to get subtitle spu
                        mLooper->sendMessageDelayed(ms2ns(100), this, Message(MSG_PTS_TIME_CHECK_SPU));
                        break;
                    }
                }

                if (spu != nullptr) {
                    uint64_t pts = convertDvbTime2Ns(spu->pts);
                    timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;

                    if (mPresent->compareBitAndSyncPts(spu, convertNs2DvbTime(timestamp))) {
                        ALOGD("after bit sync, subpts: %llu(%llu), vpts:%llu(%llu)",
                            spu->pts, spu->pts/DVB_TIME_MULTI, convertNs2DvbTime(timestamp), ns2ms(timestamp));
                        pts = convertDvbTime2Ns(spu->pts);
                    }

                    uint64_t tolerance = 33*1000*1000LL; // 33ms tolerance
                    uint64_t ptsDiff = (pts>timestamp) ? (pts-timestamp) : (timestamp-pts);
                    // The subtitle pts ahead more than 100S of video...maybe aheam more 20s
                    if ((ptsDiff >= 200*1000*1000*1000LL) && !(spu->isExtSub)) {
                        // we cannot check it's valid or not, so delay 1s(common case) and show
                        spu->pts = convertNs2DvbTime(timestamp+1*1000*1000*1000LL);
                        spu->m_delay = spu->pts + 10*1000*DVB_TIME_MULTI;
                        pts = convertDvbTime2Ns(spu->pts);
                    }

                    if (spu->m_delay <= 0) {
                        spu->m_delay = spu->pts + (ADDJUST_NO_PTS_MS * DVB_TIME_MULTI);
                    }

                    if (spu->isImmediatePresent || (pts <= (timestamp+tolerance))) {
                        mPresent->mEmittedShowingSpu.pop_front();
                        if (mPresent->mEmittedShowingSpu.size() > 0) {
                            std::shared_ptr<AML_SPUVAR> sencondSpu = mPresent->mEmittedShowingSpu.front();
                            if (mPresent->combinSamePtsSubtitle(spu, sencondSpu)) {
                                mPresent->mEmittedShowingSpu.pop_front();
                            }
                        }
                        ALOGD("Show SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%lld(%lld) duration:%lld(%lld) data:%p(%p)",
                                ns2ms(mPresent->mCurrentPresentRelativeTime),
                                ns2ms(mPresent->mStartTimeModifier),
                                spu->pts, spu->pts/DVB_TIME_MULTI,
                                spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                                spu->spu_data, spu->spu_data);
                        mPresent->mRender->showSubtitleItem(spu, mPresent->mParser->getParseType());

                        // fix fadding time, if not valid.
                        if (spu->isImmediatePresent) {
                            // immediatePresent no pts, this may affect the fading calculate.
                            // apply the real pts and fading delay.
                            spu->pts = convertNs2DvbTime(timestamp);
                            // translate to dvb time. add 10s time
                            spu->m_delay = spu->pts + (ADDJUST_NO_PTS_MS * DVB_TIME_MULTI);
                        } else if ((spu->m_delay == 0) || (spu->m_delay < spu->pts)
                                || (spu->m_delay-spu->pts) < 1000*DVB_TIME_MULTI) {
                            spu->m_delay = spu->pts + (ADDJUST_VERY_SMALL_PTS_MS * DVB_TIME_MULTI);
                        }

                        // add to fadding list. post for fadding
                        std::shared_ptr<AML_SPUVAR> cachedSpu = mPresent->mEmittedFaddingSpu.front();
                        if (cachedSpu != nullptr) {
                            mPresent->mEmittedFaddingSpu.pop_front();
                            mPresent->mRender->removeSubtitleItem(cachedSpu);
                        }
                        mPresent->mEmittedFaddingSpu.push_back(spu);
                    } else {
                        uint64_t delayTime = spu->isExtSub ? ms2ns(100):pts-timestamp;
                        mLooper->sendMessageDelayed(delayTime, this, Message(MSG_PTS_TIME_CHECK_SPU));
                    }
                } else {
                    ALOGE("Error! should not nullptr here!");
                }
            }

            // handle presentation fadding
            if (mPresent->mEmittedFaddingSpu.size() > 0) {
                spu = mPresent->mEmittedFaddingSpu.front();

                if (spu != nullptr) {
                    uint64_t delayed = convertDvbTime2Ns(spu->m_delay);
                    uint64_t timestamp = mPresent->mStartTimeModifier + mPresent->mCurrentPresentRelativeTime;
                    uint64_t ahead_delay_tor = ((spu->isExtSub)?5:100)*1000*1000*1000LL;
                    if ((delayed <= timestamp) && (delayed*5 > timestamp)) {
                        mPresent->mEmittedFaddingSpu.pop_front();
                        ALOGD("1 fade SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%lld(%lld) duration:%lld(%lld) data:%p(%p)",
                                ns2ms(mPresent->mCurrentPresentRelativeTime),
                                ns2ms(mPresent->mStartTimeModifier),
                                spu->pts, spu->pts/DVB_TIME_MULTI,
                                spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                                spu->spu_data, spu->spu_data);

                        if (spu->isKeepShowing == false) {
                            mPresent->mRender->hideSubtitleItem(spu);
                        } else {
                            mPresent->mRender->removeSubtitleItem(spu);
                        }
                   } else if  ((timestamp != 0) && ((delayed - timestamp) > ahead_delay_tor)) { //when the video gets to begin,to get rid of the sutitle data to avoid the memory leak
                        //because when pull out the cable , the video pts became zero. And the timestamp became zero.
                        //And then it would clear the subtitle data queue which may be used by the dtvkit.It may cause crash as the "bad file description".
                        //so add the "(timestamp != 0)"  condition check.
                        mPresent->mEmittedFaddingSpu.pop_front();
                        ALOGD("2 fade SPU: TimeStamp:%lld startAtPts=%lld ItemPts=%lld(%lld) duration:%lld(%lld) data:%p(%p)",
                                ns2ms(mPresent->mCurrentPresentRelativeTime),
                                ns2ms(mPresent->mStartTimeModifier),
                                spu->pts, spu->pts/DVB_TIME_MULTI,
                                spu->m_delay, spu->m_delay/DVB_TIME_MULTI,
                                spu->spu_data, spu->spu_data);
                        if (spu->isKeepShowing == false) {
                            mPresent->mRender->hideSubtitleItem(spu);
                        } else {
                            mPresent->mRender->removeSubtitleItem(spu);
                        }
                    }

                   // fire this. can be more than required
                   mLooper->sendMessageDelayed(ms2ns(100), this, Message(MSG_PTS_TIME_CHECK_SPU));
                } else {
                    ALOGE("Error! should not nullptr here!");
                }
            }
        }
        break;

        case MSG_RESET_MESSAGE_QUEUE:
            mPresent->mEmittedShowingSpu.clear();
            mPresent->mEmittedFaddingSpu.clear();
            mPresent->mRender->resetSubtitleItem();
            break;

        default:
        break;
    }
}

void Presentation::MessageProcess::looperLoop() {
    mLooper = new Looper(false);
    mLooper->sendMessageDelayed(100LL, this, Message(MSG_PTS_TIME_CHECK_SPU));
    mLooper->incStrong(nullptr);

    mPresent->mEmittedShowingSpu.clear();
    mPresent->mEmittedFaddingSpu.clear();


    while (!mRequestThreadExit) {
        int32_t ret = mLooper->pollAll(2000);
        switch (ret) {
            case -1:
                ALOGD("ALOOPER_POLL_WAKE\n");
                break;
            case -3: // timeout
                break;
            default:
                ALOGD("default ret=%d", ret);
                break;
        }
    }
}

