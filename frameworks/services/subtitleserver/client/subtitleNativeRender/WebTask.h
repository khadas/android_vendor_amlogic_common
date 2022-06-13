#pragma once
#include <string>
#include <curl/curl.h>


struct WebBuffer {
    char* buf;
    //buffer orignal size
    int size;
    //length of data that wrote to buffer
    int datalen;
};


typedef void (*onWebReqDoneCB)(int, const std::string&, void*);

class WebTask {
 public:
    WebTask();
    ~WebTask();


    void SetUrl(const char* url);
    void SetCallback(onWebReqDoneCB cb, void* para = NULL);
    void AddPostString(const char* item_name,  const char* item_data);

    int  DoGetString();
    int  DoGetFile(const char *filename, const char* range=NULL);

    int  WaitTaskDone();
    const char* GetResultString();

    void SetConnectTimeout(int timeo);

private:
    void _init();
    void _add_post_file(const char* item_name, const char* file_path, const char* file_name,  const char* content_type);
    int _on_work_done(int result);

    // do not support these construct
    WebTask(const WebTask&);
    WebTask& operator=(const WebTask&);

    CURL  *mCURL;

    // if call DoGetString or DoGetFile
    bool   m_do_func_called;
    int    m_is_getfile;

    //for post
    struct curl_httppost *m_formpost;
    struct curl_httppost *m_lastptr;
    struct curl_slist *m_headerlist;

    onWebReqDoneCB mDoneCB;
    void* mDoneCbPara;

    //for download file
    FILE* mFILE;
    std::string mFilepath;

    WebBuffer mWebBuffer;
};

