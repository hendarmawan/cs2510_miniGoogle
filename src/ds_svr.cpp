#include "ds_svr.h"
#include <string.h>
#include "rpc_net.h"
#include "rpc_log.h"
#include "rpc_http.h"

/**************************************
 * ds_event
 **************************************/

/**
 * @brief construct
 *
 * @param svr
 */
ds_event::ds_event(svr_base *svr): http_event(svr) {
}

/**
 * @brief notify process event(multi-threads call)
 */
void ds_event::on_process() {

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
 * @brief default process
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void ds_event::process_default(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    RPC_WARNING("invalid request from client, uri=%s, ip=%s, port=%u", 
            uri.c_str(), get_ip().c_str(), get_port());

    ezxml_t root = ezxml_new("message");
    ezxml_set_txt(root, "invalid request");

    char *text = ezxml_toxml(root);
    rsp_body.assign(text);

    free(text);
    ezxml_free(root);

    rsp_head = gen_http_head("404 Not Found", rsp_body.size());
}

/**
 * @brief process register request
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 * @param flag
 */
void ds_event::process_register(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body, bool flag) {

    /* parse parameter in the xml */
    ezxml_t root = ezxml_parse_str(
            (char*)req_body.data(), req_body.size());
    if (NULL == root) {
        /* xml parse error */
        process_default(uri, req_body, rsp_head, rsp_body);
        return;
    } 

    /* validation */
    if (!ezxml_child(root, "id")
            || !ezxml_child(root, "name")
            || !ezxml_child(root, "version")
            || !ezxml_child(root, "ip")
            || !ezxml_child(root, "port")) {

        ezxml_free(root);

        process_default(uri, req_body, rsp_head, rsp_body);
        RPC_WARNING("missing data in the register request, %s", 
                req_body.c_str());
        return;
    }

    /* register */
    svr_inst_t svr;
    svr.id = atoi(ezxml_child(root, "id")->txt);
    svr.name = ezxml_child(root, "name")->txt;
    svr.version = ezxml_child(root, "version")->txt;
    svr.ip = ezxml_child(root, "ip")->txt;
    svr.port = atoi(ezxml_child(root, "port")->txt);

    ezxml_free(root);

    /* succ */
    if (flag) {
        ((ds_svr*)m_svr)->do_register(svr);
    } else {
        ((ds_svr*)m_svr)->do_unregister(svr);
    }

    root = ezxml_new("message");
    ezxml_set_txt(root, "succ");

    char *text = ezxml_toxml(root);
    rsp_body.assign(text);

    free(text);
    ezxml_free(root);

    rsp_head = gen_http_head("200 OK", rsp_body.size());
}

/**
 * @brief process get_insts_by_id request
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void ds_event::process_get_insts_by_id(const std::string &uri,
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    /* invoke get insts by id */
    int id = 0;
    char version[1024] = { 0 };
    sscanf(uri.c_str(), "/get_insts_by_id?id=%d&version=%s", &id, version);

    std::vector<svr_inst_t> insts_list;
    ((ds_svr*)m_svr)->do_get_insts_by_id(id, version, insts_list);

    /* create response */
    ezxml_t root = ezxml_new("service");
    std::vector<std::string> ports;
    for (int i = 0; i < (int)insts_list.size(); ++i) {
        ports.push_back(num_to_str(insts_list[i].port));
    }
    for (int i = 0; i < (int)insts_list.size(); ++i) {
        const svr_inst_t &svr = insts_list[i];
        ezxml_t child = ezxml_add_child(root, "server", i);

        ezxml_set_txt(ezxml_add_child(child, "id", 0), num_to_str(svr.id).c_str());
        ezxml_set_txt(ezxml_add_child(child, "name", 0), svr.name.c_str());
        ezxml_set_txt(ezxml_add_child(child, "version", 0), svr.version.c_str());
        ezxml_set_txt(ezxml_add_child(child, "ip", 0), svr.ip.c_str());
        ezxml_set_txt(ezxml_add_child(child, "port", 0), ports[i].c_str());
    }
    char *text = ezxml_toxml(root);
    rsp_body.assign(text);

    free(text);
    ezxml_free(root);

    rsp_head = gen_http_head("200 OK", rsp_body.size());
}

/**
 * @brief route http request to spesific function
 *
 * @param uri
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 */
void ds_event::dsptch_http_request(const std::string &uri, 
        const std::string &req_body, std::string &rsp_head, std::string &rsp_body) {

    if (uri.find("/register") == 0) {
        process_register(uri, req_body, rsp_head, rsp_body, true);
    } else if (uri.find("/unregister") == 0) {
        process_register(uri, req_body, rsp_head, rsp_body, false);
    } else if (uri.find("/get_insts_by_id?") == 0) {
        process_get_insts_by_id(uri, req_body, rsp_head, rsp_body);
    } else {
        process_default(uri, req_body, rsp_head, rsp_body);
    }
}

/**************************************
 * ds_svr
 **************************************/
/**
 * @brief construct
 */
ds_svr::ds_svr() {
    m_lock.init();
}

/**
 * @brief destruct
 */
ds_svr::~ds_svr() {
    m_lock.destroy();
}

/**
 * @brief create event
 *
 * @param fd
 * @param ip
 * @param port
 *
 * @return 
 */
io_event *ds_svr::create_event(int fd,
        const std::string &ip, unsigned short port) {

    ds_event *evt = new ds_event(this);
    evt->set_fd(fd);
    evt->set_ip(ip);
    evt->set_port(port);

    evt->set_io_type('i');
    evt->set_state("read_head");
    evt->set_timeout(RPC_RECV_TIMEOUT);
    return (io_event*)evt;
}

/**
 * @brief register svr
 *
 * @param svr
 */
void ds_svr::do_register(svr_inst_t &svr) {
    /* insert server into the hash */
    char key[1024] = { 0 };
    sprintf(key, "%d::%s", svr.id, svr.version.c_str());

    m_lock.lock();
    if (0 == m_svc_map.count(key)) {
        svr_insts_list_t insts_list;
        m_svc_map[key] = insts_list;
    }
    svr_insts_list_t &insts_list = m_svc_map[key];
    svr_insts_list_t::iterator iter;
    for (iter = insts_list.begin(); iter != insts_list.end(); ++iter) {
        if (iter->first.ip == svr.ip && iter->first.port == svr.port) {
            break;
        }
    }
    if (iter == insts_list.end()) {
        insts_list.push_back(std::pair<svr_inst_t, unsigned long long>(
                    svr, get_cur_msec()));
    } else {
        iter->second = get_cur_msec();
    }
    m_lock.unlock();

    /* record log */
    RPC_INFO("register succ, id=%d, name=%s, version=%s, ip=%s, port=%u", 
            svr.id, svr.name.c_str(), svr.version.c_str(),
            svr.ip.c_str(), svr.port);
}

/**
 * @brief unregister svr
 *
 * @param svr
 */
void ds_svr::do_unregister(svr_inst_t &svr) {
    /* remove server from the hash */
    char key[1024] = { 0 };
    sprintf(key, "%d::%s", svr.id, svr.version.c_str());

    m_lock.lock();
    if (m_svc_map.count(key) > 0) {
        svr_insts_list_t &insts_list = m_svc_map[key];
        svr_insts_list_t::iterator iter;
        for (iter = insts_list.begin(); iter != insts_list.end(); ++iter) {
            if (iter->first.ip == svr.ip && iter->first.port == svr.port) {
                insts_list.erase(iter);
                break;
            }
        }
        if (insts_list.size() == 0) {
            m_svc_map.erase(key);
        }
    }
    m_lock.unlock();

    /* record log */
    RPC_INFO("unregister succ, id=%d, name=%s, version=%s, ip=%s, port=%u", 
            svr.id, svr.name.c_str(), svr.version.c_str(),
            svr.ip.c_str(), svr.port);
}

/**
 * @brief get server insts by id of service
 *
 * @param id
 * @param version
 * @param svr_insts_list
 */
void ds_svr::do_get_insts_by_id(int id, const std::string &version,
        std::vector<svr_inst_t> &svr_insts_list) {
    /* get insts by id */
    char key[1024] = { 0 };
    sprintf(key, "%d::%s", id, version.c_str());

    m_lock.lock();
    if (m_svc_map.count(key) > 0) {
        svr_insts_list_t &insts_list = m_svc_map[key];
        svr_insts_list_t::iterator iter;
        for (iter = insts_list.begin(); iter != insts_list.end(); ++iter) {
            const svr_inst_t &svr = iter->first;
            svr_insts_list.push_back(svr);
            RPC_DEBUG("get insts by id, id=%d, name=%s, version=%s, ip=%s, port=%u", 
                    svr.id, svr.name.c_str(), svr.version.c_str(),
                    svr.ip.c_str(), svr.port);
        }
    }
    m_lock.unlock();

    /* record log */
    RPC_INFO("get_insts_by_id succ, id=%d, version=%s, alive_svrs=%lu", 
            id, version.c_str(), svr_insts_list.size());
}

/**
 * @brief check timeout
 */
void ds_svr::check_timeout() {
    m_lock.lock();

    unsigned long long cur_msec = get_cur_msec();

    svc_map_t::iterator iter = m_svc_map.begin();
    for (; iter != m_svc_map.end();) {
        svr_insts_list_t &insts_list = iter->second;
        svr_insts_list_t::iterator sub_iter = insts_list.begin();

        for (; sub_iter != insts_list.end();) {
            if (cur_msec - sub_iter->second >= 30 * 1000) {
                RPC_INFO("check_timeout, id::version=%s, ip=%s, port=%d", 
                        iter->first.c_str(), sub_iter->first.ip.c_str(), sub_iter->first.port);
                insts_list.erase(sub_iter++);
            } else {
                ++sub_iter;
            }
        }

        if (insts_list.size() == 0) {
            m_svc_map.erase(iter++);
        } else {
            ++iter;
        }
    }
    m_lock.unlock();
}
