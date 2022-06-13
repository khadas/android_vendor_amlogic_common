#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "WebTask.h"
#include <curl/curl.h>
#include "MyLog.h"


static int get_filesize(const char* fpath) {
   return 1;
}

static size_t web_write_mem(void *ptr, size_t size, size_t nmemb, void *data) {
    //ignore data
    if (!data) return size * nmemb;

    WebBuffer *buf = (WebBuffer *)data;

    size_t left_size = buf->size - buf->datalen;
    size_t copy_size = size * nmemb > left_size ? left_size : size * nmemb;

    memcpy(buf->buf + buf->datalen, ptr, copy_size);
    buf->datalen += copy_size;
    return size * nmemb;
}

static size_t web_write_file(void *ptr, size_t size, size_t nmemb, void *stream) {
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

WebTask::WebTask() {
    m_formpost = m_lastptr = NULL;
    m_headerlist = NULL;
    memset(&mWebBuffer, 0, sizeof(mWebBuffer));
    mDoneCB = NULL;
    mFILE = NULL;
    m_is_getfile = 0;
    m_do_func_called = false;

    _init();
}

WebTask::~WebTask() {
    if (mWebBuffer.buf) {
        free(mWebBuffer.buf);
    }
}


void WebTask::SetUrl(const char* url) {
    curl_easy_setopt(mCURL, CURLOPT_URL, url);
}

void WebTask::SetCallback(onWebReqDoneCB cb, void* para) {
    mDoneCB = cb;
    mDoneCbPara = para;
}

void WebTask::_init() {
    mCURL = curl_easy_init();

    /* no progress meter please */
    curl_easy_setopt(mCURL, CURLOPT_NOPROGRESS, 1L);
    /* connect time out set */
    curl_easy_setopt(mCURL, CURLOPT_CONNECTTIMEOUT, 10L);
    /* no signal */
    curl_easy_setopt(mCURL, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(mCURL, CURLOPT_LOW_SPEED_LIMIT, 1024);
    curl_easy_setopt(mCURL, CURLOPT_LOW_SPEED_TIME, 30);
    curl_easy_setopt(mCURL, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(mCURL, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(mCURL, CURLOPT_CAINFO, "/tmp/ca.pem");
    curl_easy_setopt(mCURL, CURLOPT_DNS_CACHE_TIMEOUT, 60*60*72);
}

void WebTask::_add_post_file(const char* item_name, const char* file_path,
        const char* file_name,  const char* content_type) {
    curl_formadd(&m_formpost,
                 &m_lastptr,
                 CURLFORM_COPYNAME, item_name,
                 CURLFORM_FILE, file_path,
                 CURLFORM_FILENAME, file_name,
                 CURLFORM_CONTENTTYPE, content_type,
                 CURLFORM_END);
}

void WebTask::AddPostString(const char* item_name, const char* item_data) {
    curl_formadd(&m_formpost,
                 &m_lastptr,
                 CURLFORM_COPYNAME, item_name,
                 CURLFORM_COPYCONTENTS, item_data,
                 CURLFORM_END);
}

int WebTask::DoGetString() {
    if (m_formpost) curl_easy_setopt(mCURL, CURLOPT_HTTPPOST, m_formpost);

    /* send all data to this function  */
    curl_easy_setopt(mCURL, CURLOPT_WRITEFUNCTION, web_write_mem);

    /* we want the headers be written to this file handle */
    curl_easy_setopt(mCURL, CURLOPT_WRITEHEADER, NULL);

    mWebBuffer.size = 200*1024;
    mWebBuffer.buf = (char*)malloc(mWebBuffer.size);
    mWebBuffer.datalen = 0;
    memset(mWebBuffer.buf, 0, mWebBuffer.size);

    /* we want the body be written to this file handle instead of stdout */
    curl_easy_setopt(mCURL, CURLOPT_WRITEDATA, &mWebBuffer);

    m_do_func_called = true;
    return 0;
}

int WebTask::WaitTaskDone() {
    /* get it! */
    CURLcode res = curl_easy_perform(mCURL);
    ALOGD("curl_easy_perform:%s ret=%d", mCURL, res);

    res = (CURLcode)_on_work_done((int)res);
    ALOGD("_on_work_done ret=%d", res);

    //clean up
    if (m_formpost) curl_formfree(m_formpost);

    /* free slist */
    if (m_headerlist) curl_slist_free_all(m_headerlist);

    curl_easy_cleanup(mCURL);
    return (int)res;
}

int WebTask::DoGetFile(const char *filename, const char* range) {
    if (m_formpost) {
        curl_easy_setopt(mCURL, CURLOPT_HTTPPOST, m_formpost);
    }

    if (range) {
        curl_easy_setopt(mCURL, CURLOPT_RANGE, range);
    }

    mFILE = fopen(filename, "wb");
    if (mFILE == nullptr) {
        ALOGE("Error! cannot open tmp file:%s errno=%d", filename, errno);
        return -1;
    }
    m_is_getfile = 1;

    mFilepath = filename;

    /* send all data to this function  */
    curl_easy_setopt(mCURL, CURLOPT_WRITEFUNCTION, web_write_file);
    /* we want the headers be written to this file handle */
    curl_easy_setopt(mCURL, CURLOPT_WRITEHEADER, NULL);
    /* we want the body be written to this file handle instead of stdout */
    curl_easy_setopt(mCURL, CURLOPT_WRITEDATA, mFILE);
    curl_easy_setopt(mCURL, CURLOPT_FOLLOWLOCATION, 1);

    m_do_func_called = true;
    return 0;
}

int WebTask::_on_work_done(int result) {
    if (m_is_getfile == 0 && mWebBuffer.buf) {
        if (mWebBuffer.buf) {
            if (mWebBuffer.datalen > 0) {
                mWebBuffer.buf[mWebBuffer.size-1] = '\0';
            }
            std::string get_string = mWebBuffer.buf;

            if (mDoneCB) mDoneCB(result, get_string, mDoneCbPara);
       }
    } else if (m_is_getfile) {
        double filesize = 0;
        curl_easy_getinfo(mCURL, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize);

        if (mFILE) fclose(mFILE);

        if(mDoneCB) mDoneCB(result, mFilepath, mDoneCbPara);
    }
    return result;
}
const char*  WebTask::GetResultString() {
    if (m_is_getfile == 0 && mWebBuffer.buf) {
        return mWebBuffer.buf;
    } else {
        return NULL;
    }
}

void WebTask::SetConnectTimeout(int timeo) {
    /* connect time out set */
    curl_easy_setopt(mCURL, CURLOPT_CONNECTTIMEOUT, timeo);
}

