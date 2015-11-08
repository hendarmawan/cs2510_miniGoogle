#include "io_event.h"
#include <stdlib.h>
#include "rpc_net.h"
#include "rpc_log.h"
#include "svr_base.h"

/**************************************
 * io_event
 **************************************/
/**
 * @brief construct
 *
 * @param svr
 */
io_event::io_event(svr_base *svr): 
    m_svr(svr), 
    m_fd(-1), 
    m_io_type('i'),
    m_ref(0),
    m_timeout_ms(0) { 

    m_lock.init();
}

/**
 * @brief destruct
 */
io_event::~io_event() { 
    RPC_DEBUG("destroy fd, fd=%d", m_fd);
    if (m_fd != -1) {
        close(m_fd);
    }
    m_fd = -1;

    m_lock.destroy();
}

/**
 * @brief notify incoming io event
 */
void io_event::on_event() { 
    RPC_DEBUG("on_event");
}

/**
 * @brief notify process event(multi-threads call)
 */
void io_event::on_process() {
    RPC_DEBUG("on_process");
}

/**
 * @brief notify timeout event
 */
void io_event::on_timeout() {
    RPC_DEBUG("on_timeout");
}

/**
 * @brief add reference
 */
void io_event::add_ref() {

    m_lock.lock();
    ++m_ref;
    m_lock.unlock();
}

/**
 * @brief release reference, delete if it equals 0
 */
void io_event::release() {

    m_lock.lock();

    if (--m_ref == 0) {
        delete this;
        /* because the instance is already deleted, m_lock is not valid, 'return' here */
        return;
    }

    m_lock.unlock();
}
