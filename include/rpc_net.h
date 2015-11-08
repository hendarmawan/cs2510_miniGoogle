#ifndef __RPC_NET__
#define __RPC_NET__

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/**
 * @brief set nonblock flag for file descriptor
 *
 * @param fd
 *
 * @return 
 */
int set_nonblock(int fd);

/**
 * @brief get current msec time tick
 *
 * @return 
 */
unsigned long long get_cur_msec();

/**
 * @brief using select to wait for io ready event
 *
 * @param fd
 * @param io_type can be 'i', 'o' for input/output event
 * @param timeout_ms
 *
 * @return 
 */
int wait_for_io(int fd, char io_type, int timeout_ms);

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
        int timeout_ms);

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
        int flags, int timeout_ms);

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
        int flags, int timeout_ms);

#endif
