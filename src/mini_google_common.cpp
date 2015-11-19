#include "mini_google_common.h"
#include <string.h>
#include "rpc_log.h"
#include "rpc_http.h"

int poll_task_from_master(const std::string &ip, 
        unsigned short port, std::string &file_content) {

    /* talk to server */
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
        return -1;
    }
    return 0;
}

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
        RPC_INFO("retrieve_file failed, file_id=%s, ip=%s, port=%u",
                file_id.c_str(), ip.c_str(), port);
        return -1;
    }
    return 0;
}
