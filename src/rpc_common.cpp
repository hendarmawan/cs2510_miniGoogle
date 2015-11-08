#include "rpc_common.h"
#include "time.h"
#include "ezxml.h"
#include "rpc_log.h"
#include "rpc_http.h"

/**
 * @brief 
 *
 * @param num
 *
 * @return 
 */
std::string num_to_str(int num) {
    char buf[1024] = { 0 };
    sprintf(buf, "%d", num);
    return buf;
}

/**
 * @brief register to directory server
 *
 * @param ip
 * @param port
 * @param my_id
 * @param my_name
 * @param my_version
 * @param my_ip
 * @param my_port
 *
 * @return 
 */
int register_information(const std::string &ip, unsigned short port,
        int my_id, const std::string &my_name, const std::string &my_version,
        const std::string &my_ip, unsigned short my_port) {

    std::string str_id = num_to_str(my_id);
    std::string str_port = num_to_str(my_port);

    ezxml_t root = ezxml_new("server");
    ezxml_set_txt(ezxml_add_child(root, "id", 0), str_id.c_str());
    ezxml_set_txt(ezxml_add_child(root, "name", 0), my_name.c_str());
    ezxml_set_txt(ezxml_add_child(root, "version", 0), my_version.c_str());
    ezxml_set_txt(ezxml_add_child(root, "ip", 0), my_ip.c_str());
    ezxml_set_txt(ezxml_add_child(root, "port", 0), str_port.c_str());

    std::string data(ezxml_toxml(root));
    ezxml_free(root);

    /* talk to directory server */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    req_head = gen_http_head("/register", ip, data.size());
    req_body = data;
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to directory server error");
        return -1;
    }
    return 0;
}

/**
 * @brief unregister to directory server
 *
 * @param ip
 * @param port
 * @param my_id
 * @param my_name
 * @param my_version
 * @param my_ip
 * @param my_port
 *
 * @return 
 */
int unregister_information(const std::string &ip, unsigned short port,
        int my_id, const std::string &my_name, const std::string &my_version,
        const std::string &my_ip, unsigned short my_port) {

    std::string str_id = num_to_str(my_id);
    std::string str_port = num_to_str(my_port);

    ezxml_t root = ezxml_new("server");
    ezxml_set_txt(ezxml_add_child(root, "id", 0), str_id.c_str());
    ezxml_set_txt(ezxml_add_child(root, "name", 0), my_name.c_str());
    ezxml_set_txt(ezxml_add_child(root, "version", 0), my_version.c_str());
    ezxml_set_txt(ezxml_add_child(root, "ip", 0), my_ip.c_str());
    ezxml_set_txt(ezxml_add_child(root, "port", 0), num_to_str(my_port).c_str());

    std::string data(ezxml_toxml(root));
    ezxml_free(root);

    /* talk to directory server */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    req_head = gen_http_head("/unregister", ip, data.size());
    req_body = data;
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to directory server error");
        return -1;
    }
    return 0;
}

/**
 * @brief locate server instances
 *
 * @param ip address of directory server
 * @param port end point of directory server
 * @param id
 * @param version
 * @param svr_insts_list
 *
 * @return 
 */
int get_insts_by_id(const std::string &ip, unsigned short port,
        int id, const std::string &version,
        std::vector<svr_inst_t> &svr_insts_list) {

    /* talk to directory server */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    char head_buf[1024] = { 0 };
    sprintf(head_buf, "GET /get_insts_by_id?id=%d&version=%s HTTP/1.1\r\nHost: %s\r\n\r\n", 
            id, version.c_str(), ip.c_str());

    req_head.assign(head_buf);
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to directory server error");
        return -1;
    }

    /* parse xml data */
    ezxml_t root = ezxml_parse_str(
            (char*)rsp_body.c_str(), rsp_body.length());

    for (ezxml_t child = ezxml_child(root, "server"); child != NULL; child = child->next) {
        svr_inst_t svr_inst;
        svr_inst.id = atoi(ezxml_child(child, "id")->txt);
        svr_inst.name = ezxml_child(child, "name")->txt;
        svr_inst.version = ezxml_child(child, "version")->txt;
        svr_inst.ip = ezxml_child(child, "ip")->txt;
        svr_inst.port = atoi(ezxml_child(child, "port")->txt);

        svr_insts_list.push_back(svr_inst);
        RPC_DEBUG("list svr, id=%d, name=%s, version=%s,ip=%s, port=%u", 
                svr_inst.id, svr_inst.name.c_str(), svr_inst.version.c_str(),
                svr_inst.ip.c_str(), svr_inst.port);
    }
    ezxml_free(root);
    return 0;
}

/**
 * @brief get server information
 *
 * @param ip
 * @param port
 * @param id
 * @param version
 *
 * @return 
 */
int get_svr_id(const std::string &ip, unsigned short port,
        int &id, std::string &version) {

    /* talk to server */
    std::string req_head, req_body;
    std::string rsp_head, rsp_body;

    char head_buf[1024] = { 0 };
    sprintf(head_buf, "GET /get_svr_id HTTP/1.1\r\nHost: %s\r\n\r\n", ip.c_str());

    req_head.assign(head_buf);
    int ret = http_talk(ip, port, req_head, req_body, rsp_head, rsp_body);
    if (0 > ret) {
        RPC_WARNING("http_talk() to directory server error");
        return -1;
    }

    /* parse xml data */
    ezxml_t root = ezxml_parse_str(
            (char*)rsp_body.c_str(), rsp_body.length());

    id = atoi(ezxml_child(root, "id")->txt);
    version = ezxml_child(root, "version")->txt;

    RPC_DEBUG("get svr, id=%d, version=%s,ip=%s, port=%u", 
            id, version.c_str(), ip.c_str(), port);

    ezxml_free(root);
    return 0;
}

/**
 * @brief get and verify server
 *
 * @param ip
 * @param port
 * @param id
 * @param version
 * @param svr_inst
 *
 * @return 
 */
int get_and_verify_svr(const std::string &ip, unsigned short port,
        int id, const std::string &version, svr_inst_t &svr_inst) {

    int ret_val = 0;

    /* locate server from directory service */
    std::vector<svr_inst_t> svr_insts_list;
    ret_val = get_insts_by_id(ip, port, id, version, svr_insts_list);
    if (svr_insts_list.size() == 0) {
        RPC_WARNING("no server located, id=%d, version=%s", id, version.c_str());
        return -1;
    }
    srand((unsigned int )time(NULL));

    int index = rand() % svr_insts_list.size();
    svr_inst = svr_insts_list[index];
    RPC_INFO("server located, index=%d, svr_insts_list.size=%lu, id=%d, version=%s, ip=%s, port=%u",
            index, svr_insts_list.size(), svr_inst.id, svr_inst.version.c_str(),
            svr_inst.ip.c_str(), svr_inst.port);

    /* verify server version */
    int remote_id = 0;
    std::string remote_version;
    ret_val = get_svr_id(svr_inst.ip, svr_inst.port, remote_id, remote_version);
    if (id != remote_id || version != remote_version) {
        RPC_WARNING("error in server version, id=%d, version=%s, expect_id=%d, expect_version=%s", 
                remote_id, remote_version.c_str(), id, version.c_str());
        return -1;
    }
    RPC_INFO("server verified, id=%d, version=%s, ip=%s, port=%u",
            svr_inst.id, svr_inst.version.c_str(),
            svr_inst.ip.c_str(), svr_inst.port);

    return 0;
}
