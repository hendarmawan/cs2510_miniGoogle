#ifndef __INDEX_COMMON_H__
#define __INDEX_COMMON_H__

#include <string>

struct index_task_t {
    std::string file_content;
    std::string url;
    std::string uid;
};

struct file_task_t {
    std::string ip;
    unsigned short port;
    std::string status;
    std::string time_stamp;
};

#endif
