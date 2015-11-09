#include "task_consumer.h"
#include <errno.h>
#include <stdlib.h>
#include "ezxml.h"
#include "basic_proto.h"
#include "rpc_log.h"
#include "rpc_http.h"
#include "file_mngr.h"
#include "mini_google_common.h"

/**
 * @brief construct
 */
task_consumer::task_consumer(): 
    m_slave_port(0),
    m_running(true),
    m_thrds_num(0){
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
void *task_consumer::run_routine(void *args) {
    task_consumer *pthis = (task_consumer*)args;
    pthis->run_routine();
    return NULL;
}

/**
 * @brief 
 */
void task_consumer::run_routine() {
    RPC_DEBUG("RUNNING");
    while (is_running()) {
        int ret = consume();
        if (ret < 0) {
            sleep(m_thrds_num * 1);
        }
    }
}

/**
 * @brief wordcount
 *
 * @param file_content
 * @param word_dict
 */
static int wordcount(const std::string &file_content,
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
    return 0;
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

//int get_insts_by_id(const std::string &ip, unsigned short port,
//        int id, const std::string &version,
//        std::vector<svr_inst_t> &svr_insts_list);

    /* poll */
    if (-1 == poll_task_from_master("136.142.35.172", 80, file_content)) {
        return -1;
    }

    /* word count */
    std::map<std::string, int> word_dict;
    if (-1 == wordcount(file_content, word_dict)) {
        return -1;
    }

    /* save file */
    file_mngr *inst = file_mngr::create_instance();
    inst->get_file_id(file_id, file_content);
    inst->save(file_id, file_content);

    /* report, <file_id, slave_ip:slave_port, word_dict> */
    std::string req_head, req_body;
    pack_report(req_body, file_id, m_slave_ip, m_slave_port, word_dict);
    req_head = gen_http_head("/report", "127.0.0.1", req_body.size());

    RPC_DEBUG("report: %s", req_head.c_str());

    return -1;
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
        int ret = pthread_create(&m_thrd_ids[i], NULL, run_routine, this);
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
