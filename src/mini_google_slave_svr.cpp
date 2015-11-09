#include "mini_google_slave_svr.h"
#include "rpc_log.h"
#include "rpc_common.h"
#include "rpc_http.h"
#include "ezxml.h"
#include "file_mngr.h"

/***********************************************
 * mini_google_slave_event
 ***********************************************/
mini_google_slave_event::mini_google_slave_event(svr_base *svr): http_event(svr) {
}

void mini_google_slave_event::on_process() {

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

void mini_google_slave_event::process_default(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    RPC_WARNING("invalid request from client, uri=%s, ip=%s, port=%u", 
            uri.c_str(), get_ip().c_str(), get_port());

    ezxml_t root = ezxml_new("message");
    ezxml_set_txt(root, "invalid request");
    rsp_body = ezxml_toxml(root);
    ezxml_free(root);

    rsp_head = gen_http_head("404 Not Found", rsp_body.size());
}

void mini_google_slave_event::process_retrieve(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    RPC_DEBUG("retrieve request from master, uri=%s, ip=%s, port=%u", 
            uri.c_str(), get_ip().c_str(), get_port());

    /* load file */
    std::string file_id = uri.c_str() + strlen("/retrieve?fid=");
    file_mngr *inst = file_mngr::create_instance();
    inst->load(file_id, rsp_body);

    rsp_head = gen_http_head("200 Ok", rsp_body.size());
}

void mini_google_slave_event::dsptch_http_request(const std::string &uri, 
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    if (uri.find("/retrieve") == 0) {
        process_retrieve(uri, req_body, rsp_head, rsp_body);
    } else {
        process_default(uri, req_body, rsp_head, rsp_body);
    }
}

/***********************************************
 * mini_google_slave_svr
 ***********************************************/
io_event *mini_google_slave_svr::create_event(int fd,
        const std::string &ip, unsigned short port) {

    http_event *evt = new mini_google_slave_event(this);
    evt->set_fd(fd);
    evt->set_ip(ip);
    evt->set_port(port);

    evt->set_io_type('i');
    evt->set_state("read_head");
    evt->set_timeout(RPC_RECV_TIMEOUT);
    return (io_event*)evt;
}
