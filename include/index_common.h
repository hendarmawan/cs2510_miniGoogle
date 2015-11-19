#ifndef __INDEX_COMMON_H__
#define __INDEX_COMMON_H__

#include <string>
#include "rpc_net.h"
#include "rpc_common.h"

struct index_task_t {
    std::string file_id;
    std::string url;
    std::string file_content;
};

/* data structure used in lookup table */
struct file_info_t {
    file_info_t() {
    }
    file_info_t(const std::string &ip, unsigned short port) {
        this->ip = ip;
        this->port = port;
        this->sz_port = num_to_str(port);
        this->time_stamp = get_cur_msec();
    }
    std::string ip;
    unsigned short port;
    std::string sz_port;
    std::string status;
    unsigned long long time_stamp;
};

/* data structure used in invert index */
struct file_freq_t {
    file_freq_t() {
    }
    file_freq_t(int freq) {
        this->freq = freq;
        this->sz_freq = num_to_str(freq);
    }
    int freq;
    std::string sz_freq;
};

/* data structure used in ranking */
struct file_rank_t {
    file_rank_t() {
    }
    file_rank_t(int num, int freq) {
        this->num = num;
        this->freq = freq;
    }
    int num;
    int freq;
    std::string sz_freq;
    std::string sz_num;
};

#endif
