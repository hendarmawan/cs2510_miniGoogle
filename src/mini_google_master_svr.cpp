#include "mini_google_master_svr.h"
#include <string.h>
#include <errno.h>
#include <time.h>
#include <map>
#include <openssl/md5.h>
#include "rpc_log.h"
#include "rpc_common.h"
#include "basic_proto.h"
#include "rpc_http.h"
#include "rpc_net.h"
#include "ezxml.h"
#include "index_common.h"
#include "ezxml.h"

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

/***********************************************
 * mini_google_event
 ***********************************************/

std::string num_to_str(int num) {
    char buf[1024] = { 0 };
    sprintf(buf, "%d", num);
    return buf;
}


mini_google_event::mini_google_event(svr_base *svr): http_event(svr) {
}

void mini_google_event::on_process() {

    /* get request */
    std::string req_head = get_head();
    RPC_DEBUG("%s", req_head.c_str());
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

    RPC_INFO("incoming put request, file_id=%s, file_size=%lu", 
            task.file_id.c_str(), req_body.size());
}

void mini_google_event::process_poll(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){
    RPC_DEBUG("get poll request !!!, %lu", req_body.length());
    RPC_DEBUG("%s", req_body.c_str());
    index_task_t t;
    int ret = ((mini_google_svr*)m_svr)->poll(t);
    if (ret != -1) {
        rsp_body.assign(t.file_content);
        rsp_head = gen_http_head("200 Ok", rsp_body.size());
    }
    else{
        rsp_head = gen_http_head("404 Not Found", rsp_body.size());
    }
}

void mini_google_event::process_report(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){
    basic_proto bp (req_body.data(), req_body.size());
    char* file_id;
    char* slave_ip;
    int file_id_len, slave_ip_len;
    int slave_port;
    int word_dict_size;
    bp.read_string(file_id_len, file_id);
    bp.read_string(slave_ip_len, slave_ip);
    bp.read_int(slave_port);
    int ret = ((mini_google_svr*) m_svr)->reportLookup(file_id, slave_ip, slave_port);
    RPC_INFO("incoming report request, slave_ip=%s, slave_port=%u, ret=%d", slave_ip, slave_port, ret);

    bp.read_int(word_dict_size);
    for(int i=0;i<word_dict_size;i++){
        char* word;
        int word_len;
        int count;
        bp.read_string(word_len,word);
        bp.read_int(count);
        int ret = ((mini_google_svr*) m_svr)->reportInvert(file_id, word, count);
        if(ret==-1){
            RPC_INFO("update unsucessfully.\n");
        }
    }
}

int mini_google_svr::reportInvert(const std::string &file_id, const std::string word, int count){
    //invert_table invert_t;
    int ret = m_invert_table.update(word, file_id, count);
    return ret;
}

int mini_google_svr::reportLookup(const std::string &file_id, const std::string & slave_ip, int slave_port){
    file_info_t file_info;
    file_info.ip = slave_ip;
    file_info.port = (unsigned short)slave_port;
    file_info.sz_port = num_to_str(slave_port);
    int ret = m_lookup_table.set_file_info(file_id, file_info);
    return ret;
}

void mini_google_event::process_query(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){
    invert_table &invert_t = ((mini_google_svr*)m_svr)->get_invert_table();
    std::size_t pos = uri.find("word=");
    std::string word_query = uri.substr(pos+strlen("word="));
    std::size_t start = pos + strlen("word=");
    std::size_t found = word_query.find("+");
    int count = 0;
    ezxml_t root = ezxml_new("query_message");
    while(found != std::string::npos){
        std::string key_word = word_query.substr(start, found);
        file_freq_list_t ffl;
        bool ret = invert_t.search(key_word, ffl);
        if(ret){
            ezxml_set_txt(root, key_word.c_str());
            file_freq_list_t::iterator iter;
            ezxml_t file_kw = ezxml_add_child(root, "keyword", 0);
            for(iter = ffl.begin(); iter!=ffl.end(); iter++){
                ezxml_set_txt(ezxml_add_child(file_kw, "file_id", 0),(iter->first).c_str());
                ezxml_set_txt(ezxml_add_child(file_kw, "frequency", 0), num_to_str(iter->second).c_str());
            }
        }
        count++;
        found = word_query.find("+", found+1);
        start = found+1;
    }
    if(count==0){
        std::string keyword = word_query;
        file_freq_list_t ffl;
        bool ret = invert_t.search(keyword,ffl);
        if(ret){
            ezxml_set_txt(root, keyword.c_str());
            file_freq_list_t::iterator iter;
            ezxml_t file_kw = ezxml_add_child(root, "keyword", 0);
            for(iter = ffl.begin(); iter!=ffl.end(); iter++){
                ezxml_set_txt(ezxml_add_child(file_kw, "file_id", 0),(iter->first).c_str());
                ezxml_set_txt(ezxml_add_child(file_kw, "frequency", 0), num_to_str(iter->second).c_str());
            }
        }
    }
    std::string data(ezxml_toxml(root));
    rsp_body.assign(data);
    ezxml_free(root);
}

void mini_google_event::process_retrieve(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){

    std::size_t pos = uri.find("fid=");
    std::string file_id = uri.substr(pos + strlen("fid="));
    file_info_t file_info;
    int ret = ((mini_google_svr*)m_svr)->retrieve(file_id, file_info);
    if (ret == -1){
        process_default(uri, req_body, rsp_head, rsp_body, 
            "lookup failed, file_id not found in lookup table");
        return;
    }

    std::string req_head = gen_http_head(uri, file_info.ip, 0);
    http_talk(file_info.ip, file_info.port, req_head, req_body, rsp_head, rsp_body);

    rsp_head = gen_http_head("200 Ok", rsp_body.size());
    return;
}

int mini_google_svr::retrieve(const std::string &file_id, file_info_t &file_info){
    int ret = m_lookup_table.get_file_info(file_id, file_info);
    if(ret == -1){
        return -1;
    }
    return 0;
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
    if (strcmp(method, "get_group_data") == 0) {
        if (strcmp(table, "lookup_table") == 0) {
            ezxml_t file_info_list = ezxml_add_child(root, "file_info_list", 0);

            single_lookup_table_t *s_table = lookup_tab.lock_group(group_id);
            if (NULL != s_table) {
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
            single_invert_table_t * tab = invert_tab.lock_group(group_id);
            if (NULL != tab) {
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
 * @return 
 */
int mini_google_svr::put(index_task_t &task) {
    m_queue_lock.lock();
    m_queue.push(task);
    m_queue_lock.unlock();
    return 0;
}

int mini_google_svr::poll(index_task_t &t) {
    int ret = -1;
    m_queue_lock.lock();
    if(!m_queue.empty()){
        RPC_INFO("queue not empty.");
        t = m_queue.front();
        m_queue.pop();
        ret = 0;
    }
    m_queue_lock.unlock();
    return ret;
}

int mini_google_svr::query(const std::string &uri){
    //invert_table_lock.lock();
    //std::size_t pos = uri.find("word=");
    //std::string word_query = uri.substr(pos + strlen("word="));
    //std::size_t start = pos + strlen("word=");
    //std::size_t found = word_query.find("_");
    //int count = 0;
    //std::map<std::string, int> file_set;
    //std::map<std::string, int>::iterator it;
    //while(found != std::string::npos){
    //    std::string key_word = word_query.substr(start, found);
    //    std::list<std::pair<std::string, int> > ll = invert_table.find(key_word)->second;
    //    std::list<std::pair<std::string, int> >::iterator iter1;
    //    for(iter1 = ll.begin(); iter1 != ll.end(); iter1 ++){
    //        it = file_set.find(iter1->first);
    //        if(it == file_set.end()){
    //            file_set.insert(std::pair<std::string, int>(iter1->first, 1));
    //        }
    //        else{
    //            int cc = it->second;
    //            file_set.erase(it);
    //            file_set.insert(std::pair<std::string, int>(iter1->first, cc+1));
    //        }
    //    }
    //    count++;
    //    found = word_query.find("_", found+1);
    //    start = found+1;
    //}
    //if(count==0){
    //    std::string one_word = word_query;
    //    std::list<std::pair<std::string, int> > ll = invert_table.find(one_word)->second;
    //    std::list<std::pair<std::string, int> >::iterator iter1;
    //    for(iter1 = ll.begin(); iter1 != ll.end(); iter1 ++){
    //        it = file_set.find(iter1->first);
    //        if(it == file_set.end()){
    //            file_set.insert(std::pair<std::string, int>(iter1->first, 1));
    //        }
    //        else{
    //            int cc = it->second;
    //            file_set.erase(it);
    //            file_set.insert(std::pair<std::string, int>(iter1->first, cc+1));
    //        }
    //    }
    //}
    //if(!file_set.empty()){
    //    int max_count = file_set.begin()->second;
    //    for(it = file_set.begin();it != file_set.end(); it ++){
    //        if(it->second < max_count){
    //            break;
    //        }
    //        else{
    //            file_v.push_back(it->first);
    //        }
    //    }
    //}
    //invert_table_lock.unlock();
    return 0;
}










