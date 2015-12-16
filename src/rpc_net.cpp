#include "rpc_net.h"
#include <sys/time.h>
#include <string.h>
#include "rpc_log.h"

#define MAX(x,y) ((x)>(y)?(x):(y))

/**
 * @brief set nonblock flag for file descriptor
 *
 * @param fd
 *
 * @return 
 */
int set_nonblock(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (-1 == flag) {
        RPC_WARNING("fcntl() getfl error, errno=%d", errno);
        return -1;
    }
    int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
    if (-1 == ret) {
        RPC_WARNING("fcntl() setfl error, errno=%d", errno);
        return -1;
    }
    return 0;
}

/**
 * @brief get current msec time tick
 *
 * @return 
 */
unsigned long long get_cur_msec() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * @brief using select to wait for io ready event
 *
 * @param fd
 * @param io_type can be 'i', 'o' for input/output event
 * @param timeout_ms
 *
 * @return 
 */
int wait_for_io(int fd, char io_type, int timeout_ms) {
    if ('i' != io_type && 'o' != io_type) {
        RPC_WARNING("invalid value for io_type");
        return -1;
    }

    /* create fds */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* set timeout */
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = timeout_ms % 1000 * 1000;

    int ret = 0;
    if ('i' == io_type) {
        ret = select(fd + 1, &fds, NULL, NULL, &tv);
    } else {
        ret = select(fd + 1, NULL, &fds, NULL, &tv);
    }

    if (ret < 0) {
        RPC_WARNING("select() error, fd=%d, errno=%d", fd, errno);
        return -1;
    }
    if (0 == ret) {
        RPC_WARNING("select() timeout, fd=%d, errno=%d", fd, errno);
        return -1;
    }
    return 0;
}

/**
 * @brief wrapper for connect api
 *
 * @param ip
 * @param port
 * @param timeout_ms
 *
 * @return 
 */
int connect_ex(char *ip, unsigned short port, 
        int timeout_ms) {

    /* create socket */
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (-1 == fd) {
        RPC_WARNING("socket() error, errno=%d", errno);
        return -1;
    }
    RPC_DEBUG("socket created, fd=%d", fd);

    /* set to nonblock socket */
    if (-1 == set_nonblock(fd)) {
        RPC_WARNING("set_nonblock() error, errno=%d", errno);
        close(fd);
        return -1;
    }

    /* connect to remote server */
    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    remote_addr.sin_addr.s_addr = inet_addr(ip);

    int ret = connect(fd, (struct sockaddr*)&remote_addr, sizeof(remote_addr));
    if (0 == ret) {
        RPC_DEBUG("connect_ex() succ, fd=%d, ip=%s, port=%u", fd, ip, port);
        return fd;
    }
    if (-1 != ret || EINPROGRESS != errno) {
        RPC_WARNING("connect() error, ret=%d, errno=%d", ret, errno);
        close(fd);
        return -1;
    }

    /* wait until fd is writable */
    ret = wait_for_io(fd, 'o', timeout_ms);
    if (ret < 0) {
        RPC_WARNING("wait_for_io() timeout, fd=%d", fd);
        close(fd);
        return -1;
    }

    /* check socket status */
    int socket_res;
    socklen_t res_len = sizeof(socket_res);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_res, &res_len) < 0) {
        RPC_WARNING("getsockopt() error, fd=%d, errno=%d", fd, errno);
        close(fd);
    }
    if (0 != socket_res) {
        RPC_WARNING("connect() error, remote not available, fd=%d", fd);
        close(fd);
        return -1;
    }
    RPC_DEBUG("connect_ex() succ, fd=%d, ip=%s, port=%u", fd, ip, port);
    return fd;
}

/**
 * @brief wrapper for send api
 *
 * @param fd
 * @param buf
 * @param expect_len
 * @param flags
 * @param timeout_ms
 *
 * @return 
 */
int send_ex(int fd, char *buf, int expect_len,
        int flags, int timeout_ms) {

    int prev_ms = timeout_ms;

    int send_len = 0;
    while (send_len < expect_len) {
        unsigned long long prev_msec = get_cur_msec();

        int ret = wait_for_io(fd, 'o', timeout_ms);
        timeout_ms = MAX(0, timeout_ms - (get_cur_msec() - prev_msec));

        if (-1 == ret) {
            /* time expired or error */
            RPC_WARNING("wait_for_io() timeout, fd=%d", fd);
            return -1;
        } else {
            /* handle I/O event */
            int len = send(fd, buf + send_len, expect_len - send_len, flags);
            if (len > 0) { 
                send_len += len;
            } else {
                RPC_WARNING("send() error, fd=%d, errno=%d, msg=%s, sent=%lu, expect=%lu", 
                        fd, errno, strerror(errno), send_len, expect_len);
                return -1;
            }
        }
    }
    RPC_DEBUG("send_ex() succ, fd=%d, send_len=%d, cost_ms=%d", 
            fd, send_len, prev_ms - timeout_ms);
    return send_len;
}

/**
 * @brief wrapper for recv api
 *
 * @param fd
 * @param buf
 * @param expect_len
 * @param flags
 * @param timeout_ms
 *
 * @return 
 */
int recv_ex(int fd, char *buf, int expect_len, 
        int flags, int timeout_ms) {

    int prev_ms = timeout_ms;

    int recv_len = 0;
    while (recv_len < expect_len) {
        unsigned long long prev_msec = get_cur_msec();

        int ret = wait_for_io(fd, 'i', timeout_ms);
        timeout_ms = MAX(0, timeout_ms - (get_cur_msec() - prev_msec));

        if (-1 == ret) {
            /* time expired or error */
            RPC_WARNING("wait_for_io() timeout, fd=%d", fd);
            return -1;
        } else {
            /* handle I/O event */
            int len = recv(fd, buf + recv_len, expect_len - recv_len, flags);
            if (len == 0) { 
                RPC_WARNING("client closed socket, fd=%d", fd);
                return -1;
            } else if (len < 0) { 
                RPC_WARNING("recv() error, fd=%d, errno=%d", fd, errno);
                return -1;
            } else {
                recv_len += len;
            }
        }
    }
    RPC_DEBUG("recv_ex() succ, fd=%d, recv_len=%d, cost_ms=%d", 
            fd, recv_len, prev_ms - timeout_ms);
    return recv_len;
}
