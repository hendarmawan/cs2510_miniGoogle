#include "uishell.h"

/**
 * @brief construct
 */
uishell::uishell() {
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
    m_ip = m_master_ip = ip;
    m_port = m_master_port = port;
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

    int ret = query_for_files_from_master(m_ip, m_port, 
            str_query, query_result);
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
    int ret = retrieve_file(m_ip, m_port, file_id, file_content);
    return ret;
}

/**
 * @brief close communication
 */
void uishell::close() {
}
