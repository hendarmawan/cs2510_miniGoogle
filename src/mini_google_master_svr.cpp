#include "mini_google_master_svr.h"
#include <string.h>
#include "rpc_log.h"
#include "rpc_common.h"
#include "rpc_http.h"
#include "ezxml.h"
#include "index_common.h"

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
    t.uid = "1234566";
    //md5
    rsp_body = "put successfully.";
    rsp_head = gen_http_head("200 Ok", rsp_body.size());
    ((mini_google_svr*)m_svr)->put(t);
    RPC_INFO("put into the queue successfully.");
}

void mini_google_event::process_poll(const std::string &uri, const std::string &req_body, std::string &rsp_head, std::string &rsp_body){
    RPC_DEBUG("get poll request !!!, %lu", req_body.length());
    RPC_DEBUG("%s", req_body.c_str());
    index_task_t t;
    int ret = ((mini_google_svr*)m_svr)->poll(t);
    if (ret != -1) {
        rsp_body.assign(t.file_content);
    }
}

void mini_google_event::dsptch_http_request(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    if (uri.find("/put") == 0) {
        process_put(uri, req_body, rsp_head, rsp_body);
    } else if (uri.find("/poll") == 0){
        process_poll(uri, req_body, rsp_head, rsp_body);
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

void mini_google_svr::put(index_task_t &t) {
    m_queue_lock.lock();
    m_queue.push(t);
    m_queue_lock.unlock();
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
