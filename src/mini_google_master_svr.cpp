#include "mini_google_master_svr.h"
#include <string.h>
#include <errno.h>
#include <time.h>
#include <map>
#include "rpc_log.h"
#include "rpc_common.h"
#include "basic_proto.h"
#include "rpc_http.h"
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

void mini_google_event::process_default(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    RPC_WARNING("invalid request from client, uri=%s, ip=%s, port=%u", 
            uri.c_str(), get_ip().c_str(), get_port());

    ezxml_t root = ezxml_new("message");
    ezxml_set_txt(root, "invalid request");
    rsp_body = ezxml_toxml(root);
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

void mini_google_event::dsptch_http_request(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    if (uri.find("/put") == 0) {
        process_put(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/poll") == 0){
        process_poll(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/report") ==0){
        process_report(uri, req_body, rsp_head, rsp_body);
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
    basic_proto bp (req_body.data(), req_body.size());
    char* file_id;
    char* slave_ip;
    int file_id_len, slave_ip_len;
    int slave_port;
    int word_dict_size;
    bp.read_string(file_id_len, file_id);
    RPC_DEBUG("file id is: %s", file_id);
    bp.read_string(slave_ip_len, slave_ip);
    RPC_DEBUG("slave ip is: %s", slave_ip);
    bp.read_int(slave_port);
    RPC_DEBUG("slave port is: %d", slave_port);
    bp.read_int(word_dict_size);
    RPC_DEBUG("word dict size is: %d", word_dict_size);
    std::map<std::string, int> word_dict;
    std::map<std::string, int>::iterator it = word_dict.begin();
    for(int i=0; i<word_dict_size; i++){
        char* word;
        //std::string word_str;
        int word_len;
        int count;
        bp.read_string(word_len, word);
        bp.read_int(count);
        word_dict.insert(it, std::pair<std::string, int>(word, count));
    }
    return 0;
}









