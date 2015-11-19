#include "mini_google_master_svr.h"
#include <string.h>
#include <errno.h>
#include <time.h>
#include <map>
#include <openssl/md5.h>
#include <algorithm>
#include "rpc_log.h"
#include "rpc_common.h"
#include "mini_google_common.h"
#include "basic_proto.h"
#include "rpc_http.h"
#include "rpc_net.h"
#include "ezxml.h"
#include "index_common.h"
#include "ezxml.h"

#define MAX(x,y) ((x)>(y)?(x):(y))
#define MIN(x,y) ((x)<(y)?(x):(y))

/**
 * @brief get file id
 *
 * @param file_id
 * @param file_content
 *
 * @return 
 */
static std::string get_file_id(const std::string &file_content) {

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
 * @brief split string into words
 *
 * @param str
 * @param split
 * @param words
 */
static void split_string(const std::string &str, 
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
 * @brief summarize html file
 *
 * @param file_content
 *
 * @return 
 */
static std::string summarize(const std::string &file_content) {
    int left = 0, word_count = 0;
    std::string current_word;
    std::string text;

    for (const char *ptr = file_content.c_str(); *ptr != '\0'; ++ptr) {
        if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= '0' && *ptr <= '9')) {
            if (*ptr >= 'A' && *ptr <= 'Z') {
                //current_word += *ptr - 'A' + 'a';
                current_word += *ptr;
            } else {
                current_word += *ptr;
            }
        } else {
            if (left == 0) {
                text += current_word + " ";
                if (++word_count > 100) {
                    break;
                }
            }

            if (*ptr == '<') {
                ++left;
            } else if (*ptr == '>') {
                --left;
            }

            current_word.clear();
        }
    }
    return text;
}

/***********************************************
 * mini_google_event
 ***********************************************/

/**
 * @brief construct
 *
 * @param svr):
 */
mini_google_event::mini_google_event(svr_base *svr): 
    http_event(svr) {
}

/**
 * @brief process callback
 */
void mini_google_event::on_process() {

    /* get request */
    std::string req_head = get_head();
    std::string req_body = get_body();

    /* handle process */
    RPC_DEBUG("ip=%s, port=%u, method=%s, uri=%s, version=%s", 
            get_ip().c_str(), get_port(), 
            get_method().c_str(), get_uri().c_str(), get_version().c_str());

    std::string rsp_head, rsp_body;
    this->dsptch_http_request(get_uri(), req_body, rsp_head, rsp_body);

    /* set response */
    this->set_response(rsp_head + rsp_body);

    this->set_state("write");
    this->set_io_type('o');
    m_svr->add_io_event(this);
}

/**
 * @brief default response, 404 + msg
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 * @param msg
 */
void mini_google_event::process_default(
        const std::string &uri,
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body,
        const char *msg) {

    RPC_WARNING("invalid request from client, uri=%s, ip=%s, port=%u, msg=%s", 
            uri.c_str(), get_ip().c_str(), get_port(), msg ? msg : "none");

    ezxml_t root = ezxml_new("message");
    ezxml_set_txt(root, msg ? msg : "none");

    char *text = ezxml_toxml(root);
    rsp_body.assign(text) ;

    free(text);
    ezxml_free(root);

    rsp_head = gen_http_head("404 Not Found", rsp_body.size());
}

/**
 * @brief append new task
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_put(
        const std::string &uri,
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body) {

    /* build a task */
    index_task_t task;
    task.url     = uri.substr(uri.find("url=") + strlen("url="));
    task.file_id = get_file_id(req_body);
    task.file_content = req_body;

    /* append task to the task queue */
    int ret = ((mini_google_svr*)m_svr)->put(task);
    if(ret != -1){
        rsp_head = gen_http_head("200 Ok", rsp_body.size());
    } else {
        rsp_head = gen_http_head("404 Not Found", rsp_body.size());
    }
}

/**
 * @brief pop a task from task queue
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_poll(
        const std::string &uri,
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body) {

    index_task_t task;
    int ret = ((mini_google_svr*)m_svr)->poll(task);

    if (ret >= 0) {
        rsp_body.assign(task.file_content);
        rsp_head = gen_http_head("200 Ok", rsp_body.size());
    }
    else{
        rsp_head = gen_http_head("404 Not Found", rsp_body.size());
    }
}

/**
 * @brief process report from workers
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_report(
        const std::string &uri, 
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body){

    basic_proto bp (req_body.data(), req_body.size());

    /* file id */
    char* sz_file_id;
    int file_id_len; 
    bp.read_string(file_id_len, sz_file_id);

    std::string file_id(sz_file_id, file_id_len);

    /* slave ip */
    char* sz_slave_ip;
    int slave_ip_len;
    bp.read_string(slave_ip_len, sz_slave_ip);

    std::string slave_ip(sz_slave_ip, slave_ip_len);

    /* slave port */
    int slave_port;
    bp.read_int(slave_port);

    RPC_INFO("incoming report request, file_id=%s, slave_ip=%s, slave_port=%u", 
            file_id.c_str(), slave_ip.c_str(), slave_port);

    mini_google_svr *svr = (mini_google_svr*)m_svr;

    /* build lookup table */
    file_info_t file_info(slave_ip, slave_port);
    int ret = svr->get_lookup_table().set_file_info(file_id, file_info);
    if (ret < 0) {
        RPC_WARNING("update lookup table error, file_id=%s", file_id.c_str());
    }

    /* build invert index */
    int word_dict_size; 
    bp.read_int(word_dict_size);

    for(int i=0; i < word_dict_size; i++){
        char* sz_word;
        int word_len, count;

        bp.read_string(word_len, sz_word);
        bp.read_int(count);

        std::string word(sz_word, word_len);

        int ret = svr->get_invert_table().update(word, file_id, count);
        if (ret < -1){
            RPC_WARNING("update invert table error, file_id=%s, word=%s, count=%d", 
                    file_id.c_str(), word.c_str(), count);
        }
    }
    rsp_head = gen_http_head("200 Ok", 0);
}

/**
 * @brief compare function
 *
 * @param a
 * @param b
 *
 * @return 
 */
static bool compare(const std::pair<std::string, file_rank_t> &a, const std::pair<std::string, file_rank_t> &b) {
    if (a.second.num > b.second.num) {
        return true;
    } else if (a.second.num < b.second.num) {
        return false;
    }
    return a.second.freq > b.second.freq;
}

/**
 * @brief query
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_query(
        const std::string &uri,
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body) {

    mini_google_svr *svr = (mini_google_svr*)m_svr;

    /* check parameters */
    int merge = 1;
    char sz_words[1024] = { 0 };
    sscanf(uri.c_str(), "/query?words=%[^&]&merge=%d", sz_words, &merge);

    if (strlen(sz_words) == 0) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "query failed, no word found in uri");
        return;
    }

    /* split into words */
    ezxml_t root = ezxml_new("message");

    std::vector<std::string> words;
    split_string(sz_words, "%20", words);

    /* query */
    invert_table &invert_tab = svr->get_invert_table();

    std::vector<file_freq_list_t> ffls;
    ffls.resize(words.size());

    for (int i = 0; i < words.size(); ++i) {
        file_freq_list_t &ffl = ffls[i];
        invert_tab.search(words[i], ffl);
    }

    std::map<std::string, file_rank_t> res_stat;
    std::vector<std::pair<std::string, file_rank_t> > res_rank;

    /* invert index */
    if (!merge) {
        for (int i = 0; i < words.size(); ++i) {
            file_freq_list_t &ffl = ffls[i];

            ezxml_t xml_word = ezxml_add_child(root, "term", 0);
            ezxml_set_attr(xml_word, "t", words[i].c_str());

            for (file_freq_list_t::iterator iter = ffl.begin(); iter != ffl.end(); ++iter){
                ezxml_t xml_file = ezxml_add_child(xml_word, "f", 0);
                ezxml_set_attr(xml_file, "file_id", iter->first.c_str());
                ezxml_set_attr(xml_file, "freq", iter->second.sz_freq.c_str());
            }
        }
    }
    /* merge data */
    else {
        /* file statistics */
        for (int i = 0; i < ffls.size(); ++i) {
            file_freq_list_t &ffl = ffls[i];

            for (file_freq_list_t::iterator iter = ffl.begin(); iter != ffl.end(); ++iter){
                if (!res_stat.count(iter->first)) {
                    res_stat[iter->first] = file_rank_t(1, iter->second.freq);
                } 
                else {
                    file_rank_t &fr = res_stat[iter->first];
                    fr.freq = MIN(fr.freq, iter->second.freq);
                    fr.num += 1;
                }
            }
        }
        /* rank files */
        res_rank.assign(res_stat.begin(), res_stat.end());
        std::sort(res_rank.begin(), res_rank.end(), compare);

        ezxml_t xml_word = ezxml_add_child(root, "term", 0);
        ezxml_set_attr(xml_word, "t", sz_words);

        for (std::vector<std::pair<std::string, file_rank_t> >::iterator iter = res_rank.begin(); iter != res_rank.end(); ++iter) {
            ezxml_t xml_file = ezxml_add_child(xml_word, "f", 0);
            iter->second.sz_num = num_to_str(iter->second.num);
            iter->second.sz_freq = num_to_str(iter->second.freq);
            ezxml_set_attr(xml_file, "file_id", iter->first.c_str());
            ezxml_set_attr(xml_file, "num", iter->second.sz_num.c_str());
            ezxml_set_attr(xml_file, "freq", iter->second.sz_freq.c_str());
        }
    }

    char *resp_text = ezxml_toxml(root);
    std::string data(resp_text);
    rsp_body.assign(data);

    free(resp_text);
    ezxml_free(root);

    rsp_head = gen_http_head("200 Ok", rsp_body.size(), "text/xml");
}

/**
 * @brief retrieve file
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_retrieve(
        const std::string &uri, 
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body) {

    /* check the param */
    std::size_t pos = uri.find("fid=");
    if (pos == std::string::npos) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "retrieve failed, no fid found in uri");
        return;
    }

    mini_google_svr *svr = (mini_google_svr*)m_svr;

    file_info_t file_info;
    std::string file_id = uri.substr(pos + strlen("fid="));
    int ret = svr->get_lookup_table().get_file_info(file_id, file_info);
    if (ret < 0){
        process_default(uri, req_body, rsp_head, rsp_body, 
            "retrieve failed, file_id not found in lookup table");
        return;
    }

    ret = retrieve_file(file_info.ip, file_info.port, file_id, rsp_body);
    if (ret < 0) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "retrieve failed, inner error");
        return;
    }

    RPC_INFO("incoming retrieve request, file_id=%s, slave_ip=%s, slave_port=%u",
            file_id.c_str(), file_info.ip.c_str(), file_info.port);
    rsp_head = gen_http_head("200 Ok", rsp_body.size());
}

/**
 * @brief search
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_search(
        const std::string &uri,
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body) {

    mini_google_svr *svr = (mini_google_svr*)m_svr;

    /* check parameters */
    char sz_words[1024] = { 0 };
    sscanf(uri.c_str(), "/search?words=%[^&]", sz_words);

    if (strlen(sz_words) == 0) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "query failed, no word found in uri");
        return;
    }

    /* split into words */
    ezxml_t root = ezxml_new("message");

    std::vector<std::string> words;
    split_string(sz_words, "%20", words);

    /* query */
    invert_table &invert_tab = svr->get_invert_table();

    std::vector<file_freq_list_t> ffls;
    ffls.resize(words.size());

    for (int i = 0; i < words.size(); ++i) {
        file_freq_list_t &ffl = ffls[i];
        invert_tab.search(words[i], ffl);
    }

    /* file statistics */
    std::map<std::string, file_rank_t> res_stat;
    std::vector<std::pair<std::string, file_rank_t> > res_rank;

    for (int i = 0; i < ffls.size(); ++i) {
        file_freq_list_t &ffl = ffls[i];

        for (file_freq_list_t::iterator iter = ffl.begin(); iter != ffl.end(); ++iter){
            if (!res_stat.count(iter->first)) {
                res_stat[iter->first] = file_rank_t(1, iter->second.freq);
            } 
            else {
                file_rank_t &fr = res_stat[iter->first];
                fr.freq = MIN(fr.freq, iter->second.freq);
                fr.num += 1;
            }
        }
    }

    /* rank files */
    res_rank.assign(res_stat.begin(), res_stat.end());
    std::sort(res_rank.begin(), res_rank.end(), compare);

    if (res_rank.size() > 20) {
        res_rank.resize(20);
    }

    /* retrieve files */
    std::vector<std::string> res_file;
    res_file.resize(res_rank.size());

    for (int i = 0; i < res_rank.size(); ++i) {
        file_info_t file_info;

        std::string &file_id = res_rank[i].first;
        std::string file_content;

        if (0 == svr->get_lookup_table().get_file_info(file_id, file_info)) {
            if (0 == retrieve_file(file_info.ip, file_info.port, file_id, file_content)) {
                RPC_DEBUG("retrieve_file successfully, file_id=%s", file_id.c_str());
            }
        }
        //res_file[i] = std::string("<![CDATA[") + summarize(file_content) + "]]>";
        res_file[i] = summarize(file_content);
    }

    /* response */
    ezxml_t xml_word = ezxml_add_child(root, "term", 0);
    ezxml_set_attr(xml_word, "t", sz_words);

    int i = 0;
    for (std::vector<std::pair<std::string, file_rank_t> >::iterator iter = res_rank.begin(); iter != res_rank.end(); ++iter, ++i) {
        ezxml_t xml_file = ezxml_add_child(xml_word, "f", 0);
        iter->second.sz_num = num_to_str(iter->second.num);
        iter->second.sz_freq = num_to_str(iter->second.freq);
        ezxml_set_attr(xml_file, "file_id", iter->first.c_str());
        ezxml_set_attr(xml_file, "num", iter->second.sz_num.c_str());
        ezxml_set_attr(xml_file, "freq", iter->second.sz_freq.c_str());
        ezxml_set_txt(xml_file, res_file[i].c_str());
    }

    char *resp_text = ezxml_toxml(root);
    std::string data(resp_text);
    rsp_body.assign(data);

    free(resp_text);
    ezxml_free(root);

    rsp_head = gen_http_head("200 Ok", rsp_body.size(), "text/xml");
}

/**
 * @brief backup
 *
 * @param uri [table|method|group_id]
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::process_backup(
        const std::string &uri, 
        const std::string &req_body, 
        std::string &rsp_head, 
        std::string &rsp_body) {

    /* parse the parameters in the uri */
    int  group_id = -1;
    char table[256] = { 0 }, method[256] = { 0 };
    sscanf(uri.c_str(), "/backup?table=%[^&]&method=%[^&]&group_id=%d", table, method, &group_id);

    RPC_INFO("incoming backup request, table=%s, method=%s, group_id=%d", 
        table, method, group_id);

    /* check parameters */
    if (strcmp(table, "lookup_table") && strcmp(table, "invert_table")) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "table: lookup_table|invert_table");
        return;
    }
    if (strcmp(method, "get_group_num") && strcmp(method, "get_group_data")) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "method: get_group_num|get_group_data");
        return;
    }
    if (strcmp(method, "get_group_data") == 0 && group_id < 0) {
        process_default(uri, req_body, rsp_head, rsp_body, 
            "group id should be integer");
        return;
    }

    /* get reference from mini_google_svr */
    ezxml_t root = ezxml_new("message");

    mini_google_svr *svr = (mini_google_svr*)m_svr;
    invert_table &invert_tab = svr->get_invert_table();
    lookup_table &lookup_tab = svr->get_lookup_table();

    /* get group num */
    if (strcmp(table, "lookup_table") == 0) {
        ezxml_set_txt(
            ezxml_add_child(root, "group_num", 0), 
            num_to_str(lookup_tab.get_group_num()).c_str()
        );
    } 
    else {
        ezxml_set_txt(
            ezxml_add_child(root, "group_num", 0), 
            num_to_str(invert_tab.get_group_num()).c_str()
        );
    }

    /* get group data */
    char sz_file_info_list_size[128] = { 0 };
    char sz_term_info_list_size[128] = { 0 };

    if (strcmp(method, "get_group_data") == 0) {
        if (strcmp(table, "lookup_table") == 0) {
            ezxml_t file_info_list = ezxml_add_child(root, "file_info_list", 0);

            single_lookup_table_t *s_table = lookup_tab.lock_group(group_id);
            if (NULL != s_table) {
                sprintf(sz_file_info_list_size, "%lu", s_table->size());
                ezxml_set_attr(file_info_list, "size", sz_file_info_list_size);

                for (single_lookup_table_t::iterator iter = s_table->begin(); iter != s_table->end(); ++iter) {
                    ezxml_t file_info = ezxml_add_child(file_info_list, "f", 0);
                    ezxml_set_attr(file_info, "file_id", iter->first.c_str());
                    ezxml_set_attr(file_info, "slave_ip", iter->second.ip.c_str());
                    ezxml_set_attr(file_info, "slave_port", iter->second.sz_port.c_str());
                }
                lookup_tab.unlock_group(group_id);
            }
        }
        else {
            ezxml_t term_info_list = ezxml_add_child(root, "term_info_list", 0);

            single_invert_table_t * s_table = invert_tab.lock_group(group_id);
            if (NULL != s_table) {
                sprintf(sz_term_info_list_size, "%lu", s_table->size());
                ezxml_set_attr(term_info_list, "size", sz_term_info_list_size);

                for (single_invert_table_t::iterator iter = s_table->begin(); iter != s_table->end(); ++iter) {
                    ezxml_t term_info = ezxml_add_child(term_info_list, "t", 0);
                    ezxml_set_attr(term_info, "term", iter->first.c_str());
                    
                    const file_freq_list_t &freq_list = iter->second;
                    for (file_freq_list_t::const_iterator sub_iter = freq_list.begin(); sub_iter != freq_list.end(); ++sub_iter) {
                        ezxml_t file_info = ezxml_add_child(term_info, "f", 0);
                        ezxml_set_attr(file_info, "file_id", sub_iter->first.c_str());
                        ezxml_set_attr(file_info, "freq", sub_iter->second.sz_freq.c_str());
                    }
                }

                invert_tab.unlock_group(group_id);
            }
        }
    }

    /* pack message */
    char *text = ezxml_toxml(root);
    rsp_body.assign(text);

    free(text);
    ezxml_free(root);

    rsp_head = gen_http_head("200 Ok", rsp_body.size(), "text/xml");
}

/**
 * @brief dispatcher
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void mini_google_event::dsptch_http_request(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    if (uri.find("/put") == 0) {
        process_put(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/poll") == 0){
        process_poll(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/report") ==0){
        process_report(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/query") == 0){
        process_query(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/retrieve") == 0){
        process_retrieve(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/search") == 0){
        process_search(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/backup") == 0){
        process_backup(uri, req_body, rsp_head, rsp_body);
    } else {
        process_default(uri, req_body, rsp_head, rsp_body);
    }
}

/***********************************************
 * mini_google_svr
 ***********************************************/
io_event *mini_google_svr::create_event(int fd,
        const std::string &ip, unsigned short port) {

    http_event *evt = new mini_google_event(this);
    evt->set_fd(fd);
    evt->set_ip(ip);
    evt->set_port(port);

    evt->set_io_type('i');
    evt->set_state("read_head");
    evt->set_timeout(RPC_RECV_TIMEOUT);
    return (io_event*)evt;
}

/**
 * @brief append task to task queue
 *
 * @param task
 *
 * @return number of tasks in the queue after put
 */
int mini_google_svr::put(const index_task_t &task) {
    int ret_val = -1;

    m_queue_lock.lock();
    m_queue.push(task);
    ret_val = m_queue.size();
    m_queue_lock.unlock();

    RPC_INFO("incoming put request, queue_size=%d, file_id=%s, file_size=%lu", 
            ret_val, task.file_id.c_str(), task.file_content.size());
    return ret_val;
}

/**
 * @brief pop a task from task queue
 *
 * @param task
 *
 * @return number of tasks after poll
 */
int mini_google_svr::poll(index_task_t &task) {
    int ret_val = -1;

    m_queue_lock.lock();
    if(!m_queue.empty()){
        task = m_queue.front();
        m_queue.pop();
        ret_val = m_queue.size();
    }
    m_queue_lock.unlock();

    if (ret_val >= 0) {
        RPC_INFO("incoming poll request, queue_size=%d, file_id=%s, file_size=%lu", 
                ret_val, task.file_id.c_str(), task.file_content.size());
    }
    return ret_val;
}
