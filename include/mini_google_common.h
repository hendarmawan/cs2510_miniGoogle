#ifndef __MINI_GOOGLE_COMMON_H__
#define __MINI_GOOGLE_COMMON_H__

#include <string>
#include <vector>

/* query result */
struct query_result_t {
    std::string file_id;
    int num, freq;
    query_result_t(const std::string &file_id, int num, int freq) {
        this->file_id = file_id;
        this->num = num;
        this->freq = freq;
    }
};

/**
 * @brief put a task to the master
 *
 * @param ip
 * @param port
 * @param file_content
 *
 * @return -1: network error
 */
int put_task_to_master(const std::string &ip,
        unsigned short port, const std::string &file_content);

/**
 * @brief poll a task from the master
 *
 * @param ip
 * @param port
 * @param file_content
 *
 * @return -1: network error; -2: no task available
 */
int poll_task_from_master(const std::string &ip, 
        unsigned short port, std::string &file_content);

/**
 * @brief query for files from master
 *
 * @param ip
 * @param port
 * @param query_str
 * @param query_result
 *
 * @return -1: network error
 */
int query_for_files_from_master(const std::string &ip, 
        unsigned short port, const std::string &query_str,
        std::vector<query_result_t> &query_result);

/**
 * @brief retrieve file content from master/worker
 *
 * @param ip
 * @param port
 * @param file_id
 * @param file_content
 *
 * @return -1: network error; -2: file not found
 */
int retrieve_file(const std::string &ip, unsigned short port, 
        const std::string &file_id, std::string &file_content);


/*******************************************
 * util functions
 *******************************************/

/**
 * @brief split string into words
 *
 * @param str
 * @param split
 * @param words
 */
void split_string(const std::string &str, 
        const std::string &split, std::vector<std::string> &words);

/**
 * @brief get file id
 *
 * @param file_content
 *
 * @return 
 */
std::string get_file_id(const std::string &file_content);

/**
 * @brief split html file content into words
 *
 * @param file_content
 * @param word_list
 */
void html_to_words(const std::string &file_content,
        std::vector<std::string> &word_list);

/**
 * @brief split html file content into sentence
 *
 * @param file_content
 * @param sentence_list
 */
void html_to_sentences(const std::string &file_content,
        std::vector<std::string> &sentence_list);

#endif
