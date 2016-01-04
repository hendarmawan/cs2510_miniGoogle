#include "rpc_http.h"
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "rpc_net.h"
#include "rpc_log.h"

#define MAX(x,y) ((x)>(y)?(x):(y))

/**
 * @brief generate http head with specific http_code and content_len
 *
 * @param http_code
 * @param content_len
 *
 * @return 
 */
std::string gen_http_head(
        const std::string &http_code, int content_len) {

    char buf[512] = { 0 };
    sprintf(buf, "HTTP/1.1 %s\r\nContent-Length: %d\r\n\r\n", 
            http_code.c_str(), content_len);

    return buf;
}

/**
 * @brief generate http head with specific http_code and content_len
 *
 * @param http_code
 * @param content_len
 * @param content_type
 *
 * @return 
 */
std::string gen_http_head(
        const std::string &http_code, int content_len, 
        const std::string &content_type) {

    char buf[512] = { 0 };
    sprintf(buf, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", 
            http_code.c_str(), content_type.c_str(), content_len);

    return buf;
}

/**
 * @brief generate http head with specific uri, host, and content_len
 *
 * @param uri
 * @param host
 * @param content_len
 *
 * @return 
 */
std::string gen_http_head(const std::string &uri, 
        const std::string &host, int content_len) {
    char buf[512] = { 0 };
    sprintf(buf, "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\n\r\n", 
            uri.c_str(), host.c_str(), content_len);

    return buf;
}

/**
 * @brief send to fd
 *
 * @param fd
 * @param head
 * @param body
 * @param send_timeout_ms
 *
 * @return 
 */
int http_send(int fd, const std::string &head, 
        const std::string &body, int send_timeout_ms) {

    std::string data = head + body;
    int ret = send_ex(fd, (char*)data.data(), data.size(), 
            0, send_timeout_ms);
    if (ret < 0) {
        RPC_WARNING("send_ex() error, fd=%d", fd);
        return -1;
    }
    return 0;
}

/**
 * @brief recv from fd
 *
 * @param fd
 * @param head
 * @param body
 * @param send_timeout_ms
 *
 * @return 
 */
int http_recv(int fd, std::string &head, 
        std::string &body, int recv_timeout_ms) {

    int prev_ms = recv_timeout_ms;
    char buf[32] = { 0 };

    /* read head */
    bool read_head_done = false;
    while (!read_head_done) {
        unsigned long long prev_msec = get_cur_msec();

        int ret = wait_for_io(fd, 'i', recv_timeout_ms);
        recv_timeout_ms = MAX(0, recv_timeout_ms - (get_cur_msec() - prev_msec));

        if (-1 == ret) {
            /* time expired or error */
            RPC_WARNING("wait_for_io() timeout, fd=%d", fd);
            return -1;
        } else {
            /* handle I/O event */
            int len = recv(fd, buf, sizeof(buf), 0);
            if (len == 0) { 
                RPC_WARNING("client closed socket, fd=%d", fd);
                return -1;
            } else if (len < 0) { 
                RPC_WARNING("recv() error, fd=%d, errno=%d", fd, errno);
                return -1;
            } else {
                head.append(buf, len);
                std::size_t pos = head.find("\r\n\r\n");

                if (pos != std::string::npos) {
                    read_head_done = true;
                    body = head.substr(pos + strlen("\r\n\r\n"));
                    head = head.substr(0, pos);
                }
            }
        }
    }

    /* read body */
    int content_len = 0;
    char *pos = strcasestr((char*)head.c_str(), "content-length");
    if (NULL != pos) {
        char *pos_colon = strcasestr(pos, ":");
        if (NULL != pos_colon) {
            content_len = atoi(pos_colon + 1);
            RPC_DEBUG("extracted content_len, fd=%d, content_len=%d",
                    fd, content_len);

            int recv_len = body.length();
            body.resize(content_len);

            int ret = recv_ex(fd, (char*)body.data() + recv_len, 
                    content_len - recv_len, 0, recv_timeout_ms);
            if (ret < 0) {
                RPC_WARNING("recv_ex() error, fd=%d", fd);
                return -1;
            }
        }
    }
    return 0;
}

/**
 * @brief http talk
 *
 * @param ip
 * @param port
 * @param req_head
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 * @param conn_timeout_ms
 * @param send_timeout_ms
 * @param recv_timeout_ms
 *
 * @return 
 */
int http_talk(const std::string &ip, unsigned short port,
        const std::string &req_head, const std::string &req_body,
        std::string &rsp_head, std::string &rsp_body,
        int conn_timeout_ms, int send_timeout_ms, int recv_timeout_ms) {

    /* connect to remote server */
    int fd = connect_ex((char*)ip.c_str(), port, conn_timeout_ms);
    if (-1 == fd) {
        RPC_WARNING("connect_ex() failed, fd=%d, ip=%s, port=%u", 
                fd, ip.c_str(), port);
        return -1;
    }

    /* send data to the remote server */
    int ret = http_send(fd, req_head, req_body, send_timeout_ms);
    if (ret < 0) {
        RPC_WARNING("http_send() error, fd=%d", fd);
        close(fd);
        return -1;
    }

    /* recv response from remote server */
    ret = http_recv(fd, rsp_head, rsp_body, recv_timeout_ms);
    if (ret < 0) {
        RPC_WARNING("http_recv() error, fd=%d", fd);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

/**
 * @brief http talk
 *
 * @param ips_list available ips of the service
 * @param port service port
 * @param req_head
 * @param req_body
 * @param rsp_head
 * @param rsp_body
 * @param conn_timeout_ms
 * @param send_timeout_ms
 * @param recv_timeout_ms
 *
 * @return 
 */
int http_talk(const std::vector<std::string> &ips_list, unsigned short port,
        const std::string &req_head, const std::string &req_body,
        std::string &rsp_head, std::string &rsp_body,
        int conn_timeout_ms, int send_timeout_ms, int recv_timeout_ms) {

    /* connect to remote server */
    int fd = -1;
    for (int i = 0; i < (int)ips_list.size(); ++i) {
        fd = connect_ex((char*)ips_list[i].c_str(), port, conn_timeout_ms);
        if (-1 == fd) {
            RPC_WARNING("connect_ex() failed, fd=%d, ip=%s, port=%u", 
                    fd, ips_list[i].c_str(), port);
        } else {
            break;
        }
    }

    /* send data to the remote server */
    int ret = http_send(fd, req_head, req_body, send_timeout_ms);
    if (ret < 0) {
        RPC_WARNING("http_send() error, fd=%d", fd);
        return -1;
    }

    /* recv response from remote server */
    ret = http_recv(fd, rsp_head, rsp_body, recv_timeout_ms);
    if (ret < 0) {
        RPC_WARNING("http_recv() error, fd=%d", fd);
        return -1;
    }

    RPC_DEBUG("%s", rsp_head.c_str());
    return 0;
}
