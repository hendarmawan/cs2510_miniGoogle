#include "uishell.h"
#include <stdlib.h>
#include "rpc_log.h"

/**
 * @brief construct
 */
uishell::uishell(): 
    m_last_cache_msec(0) {
}

/**
 * @brief destruct
 */
uishell::~uishell() {
}

/**
 * @brief initialize a communication between uishell and master
 *
 * @param ip
 * @param port
 *
 * @return 
 */
int uishell::open(const std::string &ip, unsigned short port) {
    m_master_ip = ip;
    m_master_port = port;
    return 0;
}

/**
 * @brief query for documents
 *
 * @param str_query
 * @param query_result
 *
 * @return 
 */
int uishell::query(const std::string &str_query, 
        std::vector<query_result_t> &query_result) {

    this->cache_slave_address();

    int ind = random(), ret = -1;

    for (int i = 0; i < m_slaves_list.size(); ++i) {
        std::pair<std::string, unsigned short> &pr = m_slaves_list[(i + ind) % m_slaves_list.size()];
        ret = query_for_files_from_master(pr.first, pr.second, str_query, query_result);
        if (ret == -1) {
            RPC_WARNING("query failed %d, ip=%s, port=%u", 1 + i, pr.first.c_str(), pr.second);
        } else {
            RPC_INFO("query %d, ip=%s, port=%u", 1 + i, pr.first.c_str(), pr.second);
            break;
        }
    }
    return ret;
}

/**
 * @brief retrieve file content
 *
 * @param file_id
 * @param file_content
 *
 * @return 
 */
int uishell::retrieve(const std::string &file_id, 
        std::string &file_content) {

    this->cache_slave_address();

    int ind = random(), ret = -1;
    for (int i = 0; i < m_slaves_list.size(); ++i) {
        std::pair<std::string, unsigned short> &pr = m_slaves_list[(i + ind) % m_slaves_list.size()];
        ret = retrieve_file(pr.first, pr.second, file_id, file_content);
        if (ret == -1) {
            RPC_WARNING("retrieve failed %d, ip=%s, port=%u", 1 + i, pr.first.c_str(), pr.second);
        } else {
            RPC_INFO("retrieve %d, ip=%s, port=%u", 1 + i, pr.first.c_str(), pr.second);
            break;
        }
    }
    return ret;
}

/**
 * @brief close communication
 */
void uishell::close() {
}

/**
 * @brief cache slave address
 */
void uishell::cache_slave_address() {

    unsigned long long cur_msec = get_cur_msec();

    if (cur_msec - m_last_cache_msec >= 20 * 1000) {

        m_last_cache_msec = cur_msec;

        /* update slave address */
        slaves_list_t slaves_list;
        int ret = get_slave_from_master(m_master_ip, m_master_port, slaves_list);
        if (ret < 0) {
            RPC_WARNING("get slave from master failed");
            return;
        }

        /* update local cache */
        m_slaves_list = slaves_list;
        m_slaves_list.push_back(std::pair<std::string, unsigned short>(m_master_ip, m_master_port));

        char msg[1024] = { 0 };
        int start = 0;
        for (int i = 0; i < m_slaves_list.size(); ++i) {
            start += sprintf(msg + start, "%s:%u, ", m_slaves_list[i].first.c_str(), m_slaves_list[i].second);
        }
        RPC_INFO("cached servers: %s", msg);
    }
}
