#include "mini_google_common.h"
#include <openssl/md5.h>
#include <string.h>
#include "rpc_log.h"
#include "rpc_http.h"
#include "rpc_common.h"
#include "ezxml.h"

/**
 * @brief register to master server
 *
 * @param ip
 * @param port
 * @param my_ip
 * @param my_port
 *
 * @return 
 */
int register_to_master(const std::string &ip, unsigned short port,
        const std::string &my_ip, unsigned short my_port) {

    std::string str_port = num_to_str(my_port);

    ezxml_t root = ezxml_new("server");
    ezxml_set_txt(ezxml_add_child(root, "ip", 0), my_ip.c_str());
    ezxml_set_txt(ezxml_add_child(root, "port", 0), str_port.c_str());

    char *text = ezxml_toxml(root);
    std::string data(text);

    free(text);
    ezxml_free(root);

    /* talk to master server */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    req_head = gen_http_head("/register", ip, data.size());
    req_body = data;
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to master server error");
        return -1;
    }
    return 0;
}

/**
 * @brief unregister to master server
 *
 * @param ip
 * @param port
 * @param my_ip
 * @param my_port
 *
 * @return 
 */
int unregister_to_master(const std::string &ip, unsigned short port,
        const std::string &my_ip, unsigned short my_port) {

    std::string str_port = num_to_str(my_port);

    ezxml_t root = ezxml_new("server");
    ezxml_set_txt(ezxml_add_child(root, "ip", 0), my_ip.c_str());
    ezxml_set_txt(ezxml_add_child(root, "port", 0), num_to_str(my_port).c_str());

    char *text = ezxml_toxml(root);
    std::string data(text);

    free(text);
    ezxml_free(root);

    /* talk to master server */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    req_head = gen_http_head("/unregister", ip, data.size());
    req_body = data;
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to master server error");
        return -1;
    }
    return 0;
}

/**
 * @brief get slave from master
 *
 * @param ip
 * @param port
 * @param std::vector
 *
 * @return 
 */
int get_slave_from_master(const std::string &ip, unsigned short port,
        std::vector<std::pair<std::string, unsigned short> > &slave_list) {

    /* talk to master server */
    std::string req_head;
    std::string rsp_head, rsp_body;

    req_head = gen_http_head("/query-slave", ip, 0);
    int ret = http_talk(ip, port, req_head, "", rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to master server error");
        return -1;
    }

    /* parse the data */
    ezxml_t root = ezxml_parse_str((char*)rsp_body.c_str(), rsp_body.length());

    for (ezxml_t slave = ezxml_child(root, "single_slave"); slave != NULL; slave = slave->next) {
        slave_list.push_back(std::pair<std::string, unsigned short>(
                ezxml_attr(slave, "slave_ip"), atoi(ezxml_attr(slave, "slave_port"))));
    }
    ezxml_free(root);
    return 0;
}

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
        unsigned short port, const std::string &file_content) {

    std::string req_head;
    std::string rsp_head, rsp_body;

    char head_buf[1024] = { 0 };
    sprintf(head_buf, "POST /put HTTP/1.1\r\nHost: %s\r\nContent-Length: %lu\r\n\r\n", 
            ip.c_str(), file_content.size());

    req_head.assign(head_buf);
    int ret = http_talk(ip, port, req_head, file_content, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to master server error");
        return -1;
    }
    if (strcasestr(rsp_head.c_str(), "HTTP/1.1 200 Ok") != rsp_head.c_str()) {
        RPC_DEBUG("http_talk() to master server error");
        return -2;
    }
    return 0;
}

/**
 * @brief poll a task from the master
 *
 * @param ip
 * @param port
 * @param file_content
 *
 * @return 
 */
int poll_task_from_master(const std::string &ip, 
        unsigned short port, std::string &file_content) {

    std::string req_head, req_body;
    std::string rsp_head;

    char head_buf[1024] = { 0 };
    sprintf(head_buf, "GET /poll HTTP/1.1\r\nHost: %s\r\n\r\n", ip.c_str());

    req_head.assign(head_buf);
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, file_content);
    if (0 > ret) {
        RPC_WARNING("http_talk() to master server error");
        return -1;
    }
    if (strcasestr(rsp_head.c_str(), "HTTP/1.1 200 Ok") != rsp_head.c_str()) {
        RPC_DEBUG("http_talk() to master server error");
        return -2;
    }
    return 0;
}

/**
 * @brief query for files from the master
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
        std::vector<query_result_t> &query_result) {

    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    char head_buf[1024] = { 0 };
    sprintf(head_buf, "GET /query?words=%s HTTP/1.1\r\nHost: %s\r\n\r\n", 
            query_str.c_str(), ip.c_str());

    req_head.assign(head_buf);
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to master server error");
        return -1;
    }
    if (strcasestr(rsp_head.c_str(), "HTTP/1.1 200 Ok") != rsp_head.c_str()) {
        RPC_DEBUG("http_talk() to master server error");
        return -2;
    }

    /* parse the data */
    ezxml_t root = ezxml_parse_str(
            (char*)rsp_body.c_str(), rsp_body.length());

    for (ezxml_t term = ezxml_child(root, "term"); term != NULL; term = term->next) {
        for (ezxml_t f = ezxml_child(term, "f"); f != NULL; f = f->next) {
            query_result.push_back(query_result_t(ezxml_attr(f, "file_id"), atoi(ezxml_attr(f, "num")), atoi(ezxml_attr(f, "freq"))));
        }
    }
    ezxml_free(root);

    return 0;
}

/**
 * @brief retrieve file content from master/worker
 *
 * @param ip
 * @param port
 * @param file_id
 * @param file_content
 *
 * @return 
 */
int retrieve_file(const std::string &ip, unsigned short port, 
        const std::string &file_id, std::string &file_content) {

    char head_buf[1024] = { 0 };
    sprintf(head_buf, "GET /retrieve?fid=%s HTTP/1.1\r\nHost: %s\r\n\r\n", 
            file_id.c_str(), ip.c_str());

    std::string rsp_head;
    int ret = http_talk(ip, port, head_buf, "", rsp_head, file_content);
    if (ret < 0) {
        RPC_WARNING("retrieve_file error, ip=%s, port=%u, file_id=%s",
                ip.c_str(), port, file_id.c_str());
        return -1;
    }
    if (strcasestr(rsp_head.c_str(), "HTTP/1.1 200 Ok") != rsp_head.c_str()) {
        RPC_DEBUG("file not existed, file_id=%s, ip=%s, port=%u",
                file_id.c_str(), ip.c_str(), port);
        return -2;
    }
    return 0;
}

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
        const std::string &split, std::vector<std::string> &words) {

    std::size_t start = 0;
    std::size_t pos = str.find(split, start);

    std::string word;
    while (pos != std::string::npos) {
        word = str.substr(start, pos);
        if (word.length()) {
            words.push_back(str.substr(start, pos - start));
        }
        start = pos + split.length();
        pos = str.find(split, start);
    }
    word = str.substr(start, pos);
    if (word.length()) {
        words.push_back(str.substr(start));
    }
}

/**
 * @brief get file id
 *
 * @param file_content
 *
 * @return 
 */
std::string get_file_id(const std::string &file_content) {
    unsigned char md5[16] = { 0 };
    MD5((const unsigned char*)file_content.c_str(), file_content.length(), md5);

    std::string file_id;

    for (int i = 0; i < 16; ++i) {
        int low = md5[i] & 0x0f;
        int hig = (md5[i] & 0xf0) >> 4;
        if (hig < 10) {
            file_id += hig + '0';
        } else {
            file_id += hig - 10 + 'a';
        }
        if (low < 10) {
            file_id += low + '0';
        } else {
            file_id += low - 10 + 'a';
        }
    }
    return file_id;
}

/**
 * @brief split html file content into words
 *
 * @param file_content
 * @param word_list
 */
void html_to_words(const std::string &file_content,
        std::vector<std::string> &word_list) {

    int left = 0;
    std::string word;

    bool skip_tag = false;

    for (const char *ptr = file_content.c_str(); *ptr != '\0'; ++ptr) {
        if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= '0' && *ptr <= '9')) {
            if (*ptr >= 'A' && *ptr <= 'Z') {
                word += *ptr - 'A' + 'a';
            } else {
                word += *ptr;
            }
        } else {
            if (left == 0) {
                if (word.length() && !skip_tag) {
                    word_list.push_back(word);
                }
            } else {
                if (word == "style" || word == "script") {
                    skip_tag = true;
                }
            }
            if (*ptr == '<') {
                ++left;
                skip_tag = false;
            } else if (*ptr == '>') {
                --left;
            }
            word.clear();
        }
    }
}

/**
 * @brief split html file content into sentence
 *
 * @param file_content
 * @param sentence_list
 */
void html_to_sentences(const std::string &file_content,
        std::vector<std::string> &sentence_list) {

    int left = 0;
    std::string sentence;

    bool skip_tag = false;

    for (const char *ptr = file_content.c_str(); *ptr != '\0'; ++ptr) {
        if (*ptr == '<' || *ptr == '>') {
            if (left == 0) {
                std::string str;
                for (int i = 0; i < sentence.length(); ++i) {
                    char ch = sentence[i];
                    if (str.length() == 0) {
                        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') str += ch;
                    } else {
                        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                            if (str[str.length() - 1] != ' ') {
                                str += ' ';
                            }
                        } else {
                            str += ch;
                        }
                    }
                }
                if (str.length() > 0 && !skip_tag) {
                    sentence_list.push_back(str);
                }
            } else {
                if (sentence.substr(0, 5) == "style" || sentence.substr(0, 6) == "script") {
                    skip_tag = true;
                }
            }
            sentence.clear();

            if (*ptr == '<') {
                ++left;
                skip_tag = false;
            } else if (*ptr == '>') {
                --left;
            }
        } else {
            sentence += *ptr;
        }
    }
    if (left == 0) {
        std::string str;
        for (int i = 0; i < sentence.length(); ++i) {
            char ch = sentence[i];
            if (str.length() == 0) {
                if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') str += ch;
            } else {
                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                    if (str[str.length() - 1] != ' ') {
                        str += ' ';
                    }
                } else {
                    str += ch;
                }
            }
        }
        if (str.length() > 0 && !skip_tag) {
            sentence_list.push_back(str);
        }
    }
}
