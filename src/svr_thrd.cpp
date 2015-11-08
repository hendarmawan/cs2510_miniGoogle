#include "svr_thrd.h"
#include <assert.h>
#include <stdlib.h>
#include "rpc_log.h"
#include "rpc_net.h"
#include "io_event.h"

/**************************************
 * svr_thrd
 **************************************/

/**
 * @brief construct
 */
svr_thrd::svr_thrd(): m_running(true){
}

/**
 * @brief distruct
 */
svr_thrd::~svr_thrd() {
}

/**
 * @brief 
 *
 * @param args
 *
 * @return 
 */
void *svr_thrd::run_routine(void *args) {
    svr_thrd *pthis = (svr_thrd*)args;
    pthis->run_routine();
    return NULL;
}

/**
 * @brief 
 */
void svr_thrd::run_routine() {
    RPC_DEBUG("thread started");
    while (this->is_running()) {

        io_event *evt = NULL;

        /* pop a task */
        pthread_mutex_lock(&m_mutex);
        if (m_tasks.size() > 0) {
            evt = m_tasks.front();
            m_tasks.pop_front();
        } else {
            struct timespec to;
            to.tv_sec = time(NULL) + 1;
            to.tv_nsec = 0;

            pthread_cond_timedwait(&m_cond, &m_mutex, &to);
            if (m_tasks.size() > 0) {
                evt = m_tasks.front();
                m_tasks.pop_front();
            }
        }
        pthread_mutex_unlock(&m_mutex);

        /* computing */
        if (evt) {
            evt->on_process();
            evt->release();
        }
    }
    RPC_DEBUG("thread stoped");
}

/**
 * @brief join this thread
 */
void svr_thrd::join() {
    pthread_join(m_thrd_id, NULL);
    pthread_cond_destroy(&m_cond);
    pthread_mutex_destroy(&m_mutex);
}

/**
 * @brief create computing thread
 *
 * @return 
 */
int svr_thrd::run() {
    /* init cond */
    if (0 != pthread_cond_init(&m_cond, NULL)) {
        RPC_WARNING("pthread_cond_init() error");
        return -1;
    }
    /* init mutex */
    if (0 != pthread_mutex_init(&m_mutex, NULL)) {
        RPC_WARNING("pthread_mutex_init() error");
        return -1;
    }

    /* create thread */
    int ret = pthread_create(&m_thrd_id, NULL, run_routine, this);
    if (0 != ret) {
        RPC_WARNING("pthread_create() error, errno=%d", errno);
        return -1;
    }
    RPC_DEBUG("pthread_create() succ");
    return 0;
}

/**
 * @brief create a computing task
 *
 * @param evt
 */
void svr_thrd::add_task(io_event *evt) {

    evt->add_ref();

    pthread_mutex_lock(&m_mutex);

    /* add to task queue */
    m_tasks.push_back(evt);

    pthread_mutex_unlock(&m_mutex);
    pthread_cond_signal(&m_cond);
}
