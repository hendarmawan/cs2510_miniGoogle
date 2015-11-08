#include "svr_base.h"
#include <stdlib.h>
#include <string.h>
#include "rpc_log.h"
#include "rpc_net.h"
#include "rpc_http.h"
#include "rpc_common.h"
#include "accept_event.h"
#include "http_event.h"

/**************************************
 * svr_base
 **************************************/
/**
 * @brief construct
 */
svr_base::svr_base() {
    m_last_check_msec = 0;
    m_lock.init();
}

/**
 * @brief destruct
 */
svr_base::~svr_base() {
    std::list<io_event*>::iterator iter;

    iter = m_evts.begin();
    for (; iter != m_evts.end(); ++iter) {
        (*iter)->release();
    }
    iter = m_evts_appd.begin();
    for (; iter != m_evts_appd.end(); ++iter) {
        (*iter)->release();
    }
    m_lock.destroy();
}

/**
 * @brief create process threads
 *
 * @param thrds_num
 *
 * @return 
 */
int svr_base::run(int thrds_num) {
    m_thrds_pool.resize(thrds_num);
    for (int i = 0; i < (int)m_thrds_pool.size(); ++i) {
        m_thrds_pool[i] = new svr_thrd;
        m_thrds_pool[i]->run();
    }
    return 0;
}

/**
 * @brief stop process threads
 */
void svr_base::stop() {
    for (int i = 0; i < (int)m_thrds_pool.size(); ++i) {
        m_thrds_pool[i]->stop();
    }
}

/**
 * @brief join process threads
 */
void svr_base::join() {
    for (int i = 0; i < (int)m_thrds_pool.size(); ++i) {
        m_thrds_pool[i]->join();
        delete m_thrds_pool[i];
    }
    m_thrds_pool.clear();
}

/**
 * @brief bind to specific port
 *
 * @param port
 * @param backlog
 *
 * @return 
 */
int svr_base::bind(uint16_t port, int backlog) {
    accept_event *acc_evt = new accept_event(this);
    if (0 > acc_evt->bind(port, backlog)) {
        RPC_WARNING("accept event bind() error, port=%u, backlog=%d", 
                port, backlog);
        return -1;
    }
    this->add_io_event(acc_evt);
    return 0;
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
io_event *svr_base::create_event(int fd,
        const std::string &ip, unsigned short port) {

    http_event *evt = new http_event(this);
    evt->set_fd(fd);
    evt->set_ip(ip);
    evt->set_port(port);

    evt->set_io_type('i');
    evt->set_state("read_head");
    evt->set_timeout(RPC_RECV_TIMEOUT);
    return evt;
}

/**
 * @brief add io event to select queue
 *
 * @param evt
 */
void svr_base::add_io_event(io_event* evt) {
    evt->add_ref();

    m_lock.lock();
    m_evts_appd.push_back(evt);
    m_lock.unlock();
}

/**
 * @brief dispatch task into thread
 *
 * @param evt
 */
void svr_base::on_dispatch_task(io_event *evt) {

    int i = rand() % m_thrds_pool.size();
    RPC_DEBUG("dispatch task to [%d]", i);
    m_thrds_pool[i]->add_task(evt);
}

/**
 * @brief routint
 *
 * @param timeout_ms
 */
void svr_base::run_routine(int timeout_ms){
    /* merge list */
    m_lock.lock();
    std::list<io_event*>::iterator iter = m_evts_appd.begin();
    for (; iter != m_evts_appd.end(); ++iter) {
        m_evts.push_back(*iter);
    }
    m_evts_appd.clear();
    m_lock.unlock();

    /* create rfds and wfds */
    fd_set rfds, wfds;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    int max_fd = -1;
    iter = m_evts.begin();
    for (; iter != m_evts.end(); ++iter) {
        io_event *evt = *iter;
        if ('i' == evt->get_io_type()) {
            FD_SET(evt->get_fd(), &rfds);
        } else {
            FD_SET(evt->get_fd(), &wfds);
        }
        if (evt->get_fd() > max_fd) {
            max_fd = evt->get_fd();
        }
    }

    /*set timeout*/
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = timeout_ms % 1000 * 1000;

    /* wait for io event */
    int ret = select(1 + max_fd, &rfds, &wfds, NULL, &tv);
    if (ret < 0) {
        RPC_WARNING("select() error, errno=%d", errno);
        return;
    }

    /* run state machine */
    if (ret > 0) {
        iter = m_evts.begin();
        while (iter != m_evts.end()) {
            io_event *evt = *iter;
            fd_set *fds = NULL;
            if ('i' == evt->get_io_type()) {
                fds = &rfds;
            } else {
                fds = &wfds;
            }
            if (FD_ISSET(evt->get_fd(), fds)) {
                iter = m_evts.erase(iter);
                evt->on_event();
                evt->release();
            } else {
                ++iter;
            }
        }
    }

    /* check for timeout */
    unsigned long long cur_msec = get_cur_msec();
    if (cur_msec - m_last_check_msec >= 1000) {
        /* check timeout every seconds */
        m_last_check_msec = cur_msec;

        iter = m_evts.begin();
        while (iter != m_evts.end()) {
            io_event *evt = *iter;
            if (evt->is_timeout()) {
                iter = m_evts.erase(iter);
                evt->on_timeout();
                evt->release();
            } else {
                ++iter;
            }
        }
    } 
}
