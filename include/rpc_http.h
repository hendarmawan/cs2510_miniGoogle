#ifndef __RPC_HTTP_H__
#define __RPC_HTTP_H__

#include <string>
#include <vector>

/**
 * @brief generate http head with specific http_code and content_len
 *
 * @param http_code
 * @param content_len
 *
 * @return 
 */
std::string gen_http_head(
        const std::string &http_code, int content_len);

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
        const std::string &host, int content_len);

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
        const std::string &body, int send_timeout_ms);

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
        std::string &body, int recv_timeout_ms);

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
        int conn_timeout_ms = 1000,
        int send_timeout_ms = 1000, 
        int recv_timeout_ms = 1000);

#endif
