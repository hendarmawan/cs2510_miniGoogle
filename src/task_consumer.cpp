#include "task_consumer.h"
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include "ezxml.h"
#include "basic_proto.h"
#include "rpc_log.h"
#include "rpc_http.h"
#include "rpc_net.h"
#include "file_mngr.h"
#include "mini_google_common.h"

/**
 * @brief construct
 */
task_consumer::task_consumer(): 
    m_master_port(0),
    m_slave_port(0),
    m_running(true),
    m_thrds_num(0) {
}

/**
 * @brief destruct
 */
task_consumer::~task_consumer() {
}

/**
 * @brief 
 *
 * @param args
 *
 * @return 
 */
void *task_consumer::task_routine(void *args) {
    task_consumer *pthis = (task_consumer*)args;
    pthis->task_routine();
    return NULL;
}

/**
 * @brief 
 */
void task_consumer::task_routine() {
    RPC_DEBUG("RUNNING");
    while (is_running()) {
        int ret = consume();
        if (ret < 0) {
            usleep(500 * 1000);
        }
    }
}

/**
 * @brief wordcount
 *
 * @param file_content
 * @param word_dict
 */
static void wordcount(const std::string &file_content,
        std::map<std::string, int> &word_dict) {

    int left = 0;
    std::string current_word;

    for (const char *ptr = file_content.c_str(); *ptr != '\0'; ++ptr) {
        if (*ptr == '<' || *ptr == '>') {
            /* exclude words in '<>' pair */
            if (*ptr == '<') {
                ++left;
            } else {
                --left;
            }
        } 
        else if (0 == left) {
            if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= '0' && *ptr <= '9')) {
                if (*ptr >= 'A' && *ptr <= 'Z') {
                    current_word += *ptr - 'A' + 'a';
                } else {
                    current_word += *ptr;
                }
            } else {
                if (!word_dict.count(current_word)) {
                    word_dict[current_word] = 0;
                }
                word_dict[current_word] += 1;
                current_word.clear();
            }
        }
    }
}

/**
 * @brief pack report
 *
 * @param report
 * @param file_id
 * @param slave_ip
 * @param slave_port
 * @param word_dict
 */
static void pack_report(std::string &report, const std::string &file_id,
        const std::string &slave_ip, unsigned short slave_port,
        std::map<std::string, int> &word_dict) {
    
    basic_proto bp;
    bp.add_string(file_id.length(), file_id.c_str());
    bp.add_string(slave_ip.length(), slave_ip.c_str());
    bp.add_int(slave_port);
    bp.add_int(word_dict.size());

    std::map<std::string, int>::iterator iter; 
    for (iter = word_dict.begin(); iter != word_dict.end(); ++iter) {
        bp.add_string(iter->first.length(), iter->first.c_str());
        bp.add_int(iter->second);
    }
    report.assign(bp.get_buf(), bp.get_buf_len());
}

/**
 * @brief try to consume a task
 *
 * @return 
 */
int task_consumer::consume() {
    std::string file_id, file_content;

    svr_inst_t master;
    master.ip = m_master_ip;
    master.port = m_master_port;

    /* poll task and compute wordcount */
    if (-1 == poll_task_from_master( master.ip, master.port, file_content)) {
        RPC_DEBUG("poll task failed, master_ip=%s, master_port=%u", 
                master.ip.c_str(), master.port);
        return -1;
    }

    std::map<std::string, int> word_dict;
    wordcount(file_content, word_dict);

    /* save file */
    file_mngr *inst = file_mngr::create_instance();
    inst->get_file_id(file_id, file_content);
    inst->save(file_id, file_content);

    /* report, <file_id, slave_ip:slave_port, word_dict> */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    pack_report(req_body, file_id, m_slave_ip, m_slave_port, word_dict);

    req_head = gen_http_head("/report", master.ip, req_body.size());
    int ret = http_talk(master.ip, master.port, 
            req_head, req_body, rsp_head, rsp_body);

    if (ret < 0) {
        RPC_WARNING("report to master error, master_ip=%s, master_port=%u, file_id=%s", 
                master.ip.c_str(), master.port, file_id.c_str());
        return -1;
    }

    RPC_INFO("report to master succ, master_ip=%s, master_port=%u, file_id=%s, file_size=%lu, worddict_size=%lu", 
            master.ip.c_str(), master.port, file_id.c_str(), file_content.size(), word_dict.size());
    return 0;
}

/**
 * @brief create working threads
 *
 * @param thrds_num
 *
 * @return 
 */
int task_consumer::run(int thrds_num) {
    m_thrds_num = thrds_num;
    m_thrd_ids.resize(thrds_num);

    for (int i = 0; i < (int)m_thrd_ids.size(); ++i) {
        /* create thread */
        int ret = pthread_create(&m_thrd_ids[i], NULL, task_routine, this);
        if (0 != ret) {
            RPC_WARNING("pthread_create() error, errno=%d", errno);
            return -1;
        }
    }
    return 0;
}

/**
 * @brief join
 */
void task_consumer::join() {
    for (int i = 0; i < (int)m_thrd_ids.size(); ++i) {
        pthread_join(m_thrd_ids[i], NULL);
    }
}
