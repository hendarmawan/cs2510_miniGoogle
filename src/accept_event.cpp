#include "accept_event.h"
#include <string.h>
#include "rpc_net.h"
#include "rpc_log.h"
#include "svr_base.h"
#include "http_event.h"

/**************************************
 * accept_event
 **************************************/

/**
 * @brief construct
 *
 * @param svr
 */
accept_event::accept_event(svr_base *svr):
    io_event(svr){
    }

/**
 * @brief destruct
 */
accept_event::~accept_event() {
}

/**
 * @brief bind to a specific port
 *
 * @param port
 * @param backlog
 *
 * @return 
 */
int accept_event::bind(unsigned short port, int backlog) {

    /* create socket */
    m_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == m_fd) {
        RPC_WARNING("socket() error, errno=%d", errno);
        return -1;
    }
    RPC_DEBUG("svr socket created, fd=%d", m_fd);

    /* set to nonblock socket */
    if (-1 == set_nonblock(m_fd)) {
        RPC_WARNING("set_nonblock() error, errno=%d", errno);
        return -1;
    }

    /* set reuseaddr */
    int val = 1;
    int ret = setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    if (-1 == ret) {
        RPC_WARNING("setsockopt() error, errno=%d", errno);
        return -1;
    }

    /* bind to port */
    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    ret = ::bind(m_fd, (struct sockaddr*)&my_addr, sizeof(my_addr));
    if (-1 == ret) {
        RPC_WARNING("bind() error, errno=%d", errno);
        return -1;
    }
    RPC_DEBUG("svr socket binded, port=%u", port);

    /* listen */
    ret = listen(m_fd, backlog);
    if (-1 == ret) {
        RPC_WARNING("listen() error, errno=%d", errno);
        return -1;
    }
    RPC_DEBUG("svr socket is listening, backlog=%d", backlog);

    RPC_INFO("svr binded succ, fd=%d, port=%u, backlog=%d", m_fd, port, backlog);

    this->set_io_type('i');
    return 0;
}

/**
 * @brief notify incoming io event
 */
void accept_event::on_event() {

    /* accept new connection */
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len = sizeof(remote_addr);
    memset(&remote_addr, 0, sizeof(remote_addr));

    int fd = accept(m_fd, (struct sockaddr*)&remote_addr, &remote_addr_len);
    if (-1 == fd) {
        RPC_WARNING("accept() error, errno=%d", errno);
        return;
    }

    /* set fd to nonblock */
    if (-1 == set_nonblock(fd)) {
        RPC_WARNING("set_nonblock() error, errno=%d", errno);
        close(fd);
        return;
    }

    /* extract ip and port from remote_addr */
    std::string ip(inet_ntoa(remote_addr.sin_addr));
    unsigned short port = ntohs(remote_addr.sin_port);

    RPC_DEBUG("accept new connection, fd=%d, ip=%s, port=%u",
            fd, ip.c_str(), port);

    /* create new io event, bind fd with it */
    io_event *evt = m_svr->create_event(fd, ip, port);
    m_svr->add_io_event(this);
    m_svr->add_io_event(evt);
}
