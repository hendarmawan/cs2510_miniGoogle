#include "mini_google_master_svr.h"
#include <string.h>
#include <errno.h>
#include <time.h>
#include <map>
#include "rpc_log.h"
#include "rpc_common.h"
#include "basic_proto.h"
#include "rpc_http.h"
#include "rpc_net.h"
#include "ezxml.h"
#include "index_common.h"
#include "file_mngr.h"

/***********************************************
 * mini_google_event
 ***********************************************/
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

void mini_google_event::process_put(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {
    RPC_DEBUG("get put request !!!, %lu", req_body.length());
    RPC_DEBUG("%s", req_body.c_str());

    index_task_t t;
    std::size_t pos = uri.find("url=");
    t.url = uri.substr(pos + strlen("url="));
    t.file_content = req_body;
    //t.uid = "1234566";
    //md5
    file_mngr *inst = file_mngr::create_instance();
    inst->get_file_id(t.uid, t.file_content);
    int ret = ((mini_google_svr*)m_svr)->put(t);
    if(ret != -1){
        rsp_body.assign(t.file_content);
        rsp_head = gen_http_head("200 Ok", rsp_body.size());
    }
    else{
        rsp_head = gen_http_head("404 Not Found", rsp_body.size());
    }
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
    RPC_DEBUG("get report request !!!, %lu", req_body.length());
    RPC_DEBUG("%s", req_body.c_str());
    int ret = ((mini_google_svr*)m_svr)->report(req_body);
    if (ret != -1) {
        rsp_head = gen_http_head("200 Ok", rsp_body.size());
    }
    else{
        rsp_head = gen_http_head("404 Not Found", rsp_body.size());
    }
}

void mini_google_event::process_query(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){
    RPC_DEBUG("get query request !!!, %lu", req_body.length());
    RPC_DEBUG("%s", req_body.c_str());
    std::vector<std::string> file_v;
    int ret = ((mini_google_svr*)m_svr)->query(uri, file_v);
    if (ret!=-1) {
        rsp_head = gen_http_head("200 Ok", rsp_body.size());
    }
    else{
        rsp_head = gen_http_head("404 Not Found", rsp_body.size());
    }
    
}

void mini_google_event::process_retrieve(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){

    std::size_t pos = uri.find("fid=");
    std::string file_id = uri.substr(pos + strlen("fid="));
    file_info_t file_info;
    int ret = ((mini_google_svr*)m_svr)->retrieve(file_id, file_info);
    if (ret != -1){
        process_default(uri, req_body, rsp_head, rsp_body, 
            "lookup failed, file_id not found in lookup table");
        return;
    }

    std::string req_head = gen_http_head(uri, file_info.ip, 0);
    http_talk(file_info.ip, file_info.port, req_head, req_body, rsp_head, rsp_body);
    return;
}

int mini_google_svr::retrieve(const std::string &file_id, file_info_t &file_info){
    lookup_table lookup_t;
    int ret = lookup_t.get_file_info(file_id, file_info);
    RPC_DEBUG("%s %d", file_info.ip.c_str(), file_info.port);
    exit(0);
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
    if (strcmp(table, "lookup_table") == 0) {
        ezxml_t file_info_list = ezxml_add_child(root, "file_info_list", 0);

        ezxml_t file_info = ezxml_add_child(file_info_list, "f", 0);
        ezxml_set_attr(file_info, "ip", "127.0.0.1");

        single_table_t *tab = lookup_tab.lock_group(group_id);
        if (NULL != tab) {
            lookup_tab.unlock_group(group_id);
        }
    } 
    else {
        const single_invert_table_t &tab = invert_tab.lock_group(group_id);
        invert_tab.unlock_group(group_id);
    }

    /* pack message */
    char *text = ezxml_toxml(root);
    rsp_body.assign(text);

    free(text);
    ezxml_free(root);

    rsp_head = gen_http_head("200 Ok", rsp_body.size());
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

int mini_google_svr::put(index_task_t &t) {
    m_queue_lock.lock();
    m_queue.push(t);
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
    else{
        RPC_INFO("There is no request in current queue.");
        ret = -1;
    }
    m_queue_lock.unlock();
    return ret;
}

int mini_google_svr::report(const std::string &req_body){
    //invert_table_lock.lock();
    //basic_proto bp (req_body.data(), req_body.size());
    //char* file_id;
    //char* slave_ip;
    //int file_id_len, slave_ip_len;
    //int slave_port;
    //int word_dict_size;
    //bp.read_string(file_id_len, file_id);
    //RPC_INFO("file id is: %s; slave ip is: %s; slave port is: %d; word dict size is: %d", file_id, slave_ip, slave_port, word_dict_size);
    //bp.read_string(slave_ip_len, slave_ip);
    //bp.read_int(slave_port);
    //bp.read_int(word_dict_size);
    ////std::map<std::string, int> word_dict;
    ////std::map<std::string, int>::iterator it = word_dict.begin();
    //std::map<std::string, std::list<std::pair<std::string, int> > >::iterator it;
    //for(int i=0; i<word_dict_size; i++){
    //    char* word;
    //    //std::string word_str;
    //    int word_len;
    //    int count;
    //    bp.read_string(word_len, word);
    //    bp.read_int(count);
    //    it = invert_table.find(word);
    //    if(it != invert_table.end()){ //the word has been contained in the table
    //        std::list<std::pair<std::string, int> > doc_list = invert_table.find(word)->second;
    //        std::list<std::pair<std::string, int> >::iterator iter;
    //        int flag = 0;
    //        for(iter = doc_list.begin(); iter != doc_list.end(); iter++){
    //            if(strcmp((iter->first).c_str(), file_id) == 0){
    //                flag = 1;
    //                break;
    //            }
    //        }
    //        if(flag == 0){
    //            for(iter = doc_list.begin(); iter != doc_list.end(); iter++){
    //                if(iter->second<count){
    //                    break;
    //                }
    //            }
    //            std::pair<std::string, int> p (file_id, count);
    //            doc_list.insert(iter, p);
    //            invert_table.erase(it);
    //            it = invert_table.begin();
    //            invert_table.insert(it, std::pair<std::string, std::list<std::pair<std::string, int> > > (word, doc_list));
    //        }
    //    }
    //    else{  // the word is not contained in the table
    //        std::list<std::pair<std::string, int> > d_list;
    //        std::list<std::pair<std::string, int> >::iterator iter;
    //        iter = d_list.begin();
    //        d_list.insert(iter, std::pair<std::string, int> (file_id, count));
    //        it = invert_table.begin();
    //        invert_table.insert(it, std::pair<std::string, std::list<std::pair<std::string, int> > > (word, d_list));
    //    }
    //    //word_dict.insert(it, std::pair<std::string, int>(word, count));
    //    
    //    //std::map<std::string, std::list<std::pair<std::string, int> > > invert_table;
    //    
    //    //std::map<std::string, file_info_t > lookup_table;
    //    
    //}
    //invert_table_lock.unlock();
    return 0;
}

int mini_google_svr::query(const std::string &uri, std::vector<std::string> &file_v){
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










