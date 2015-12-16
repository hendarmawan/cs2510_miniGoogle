#include "http_event.h"
#include <stdlib.h>
#include <string.h>
#include "rpc_net.h"
#include "rpc_log.h"
#include "rpc_common.h"
#include "svr_base.h"

/**************************************
 * http_event
 **************************************/
/**
 * @brief construct
 *
 * @param svr
 */
http_event::http_event(svr_base *svr):
    io_event(svr),
    m_port(0),
    m_content_len(0) {
}

/**
 * @brief destruct
 */
http_event::~http_event() {
}

/**
 * @brief notify incoming io event
 */
void http_event::on_event() {
    if (this->get_state() == "read_head") {
        on_read_head();
    } else if (this->get_state() == "read_body") {
        on_read_body();
    } else {
        on_write();
    }
}

/**
 * @brief notify incoming read head event
 */
void http_event::on_read_head() {
    char buf[128] = { 0 };
    int len = recv(m_fd, buf, sizeof(buf), 0);
    if (len == 0) { 
        RPC_WARNING("client closed socket, fd=%d, ip=%s, port=%u", 
                m_fd, m_ip.c_str(), m_port);
        return;
    } 
    if (len < 0) { 
        RPC_WARNING("recv() error, fd=%d, errno=%d, ip=%s, port=%u", 
                m_fd, errno, m_ip.c_str(), m_port);
        return;
    } 

    //RPC_DEBUG("recv head, fd=%d, read_len=%d", m_fd, len);
    m_head.append(buf, len);

    std::size_t pos = m_head.find("\r\n\r\n");

    if (pos == std::string::npos) {
        /* read head not done */
        this->set_state("read_head");
        m_svr->add_io_event(this);
    } else {
        /* read head done */
        RPC_DEBUG("read head done, fd=%d", m_fd);

        m_body = m_head.substr(pos + strlen("\r\n\r\n"));
        m_head = m_head.substr(0, pos);

        this->parse_http_head();
        if (m_body.size() < m_content_len) {
            RPC_DEBUG("body_size=%u content_len=%u", m_body.size(), m_content_len);
            this->set_state("read_body");
            m_svr->add_io_event(this);
        } else {
            m_body.resize(m_content_len);
            this->set_state("compute");
            this->on_cmplt_pkg();
        }
    }
}

/**
 * @brief notify incoming read body event
 */
void http_event::on_read_body() {
    std::string buf;
    buf.resize(m_content_len - m_body.size());

    int len = recv(m_fd, (char*)buf.data(), buf.size(), 0);
    if (len == 0) { 
        RPC_WARNING("client closed socket, fd=%d, ip=%s, port=%u", 
                m_fd, m_ip.c_str(), m_port);
        return;
    } 
    if (len < 0) { 
        RPC_WARNING("recv() error, fd=%d, errno=%d, ip=%s, port=%u", 
                m_fd, errno, m_ip.c_str(), m_port);
        return;
    }
    buf.resize(len);
    m_body.append(buf);
    if (m_body.size() < m_content_len) {
        this->set_state("read_body");
        m_svr->add_io_event(this);
    } else {
        m_body.resize(m_content_len);
        this->set_state("compute");
        this->on_cmplt_pkg();
    }
}

/**
 * @brief notify incoming write event
 */
void http_event::on_write() {
    int len = send(m_fd, (char*)m_resp_data.data(), m_resp_data.size(), 0);
    if (len == 0) { 
        RPC_WARNING("send() error, fd=%d, errno=%d, ip=%s, port=%u, forget resp_data?", 
                m_fd, errno, m_ip.c_str(), m_port);
        return;
    }
    if (len < 0) { 
        RPC_WARNING("send() error, fd=%d, errno=%d, ip=%s, port=%u", 
                m_fd, errno, m_ip.c_str(), m_port);
        return;
    }
    //RPC_DEBUG("send, fd=%d, send_len=%d", m_fd, len);
    m_resp_data = m_resp_data.substr(len);
    if (m_resp_data.size() > 0) {
        this->set_state("write");
        m_svr->add_io_event(this);
    } else {
        RPC_DEBUG("session over, fd=%d", m_fd);
    }
}

/**
 * @brief parse http head
 */
void http_event::parse_http_head() {
    /* parse content length */
    m_content_len = 0;
    char *pos_contlen = strcasestr((char*)m_head.c_str(), "content-length");
    if (NULL != pos_contlen) {
        char *pos_colon = strcasestr(pos_contlen, ":");
        if (NULL != pos_colon) {
            m_content_len = atoi(pos_colon + 1);
        }
    }
    RPC_DEBUG("extracted content_len, fd=%d, content_len=%d",
            m_fd, m_content_len);

    /* parse method and uri */
    std::string line, method, uri;
    std::size_t pos_line = m_head.find("\r\n");
    if (std::string::npos == pos_line) {
        line = m_head;
    } else {
        line = m_head.substr(0, pos_line);
    }
    const char *ch = line.c_str();
    for (; *ch != '\0' && *ch != ' '; ++ch) {
        m_method += *ch;
    }
    for (; *ch != '\0' && *ch == ' '; ++ch) {
    }
    for (; *ch != '\0' && *ch != ' '; ++ch) {
        m_uri += *ch;
    }
    for (; *ch != '\0' && *ch == ' '; ++ch) {
    }
    for (; *ch != '\0' && *ch != ' '; ++ch) {
        m_version += *ch;
    }
}

/**
 * @brief notify http packet complete event
 */
void http_event::on_cmplt_pkg() {
    m_svr->on_dispatch_task(this);
}

/**
 * @brief notify process event(multi-threads call)
 */
void http_event::on_process() {
    RPC_DEBUG("compute %s", m_head.c_str());
    this->set_response(m_head + "\r\n\r\n" + m_body);

    this->set_io_type('o');
    this->set_state("write");
    this->set_timeout(RPC_SEND_TIMEOUT);
    m_svr->add_io_event(this);
}
