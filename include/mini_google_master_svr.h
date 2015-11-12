#ifndef __MINI_GOOGLE_SVR_H__
#define __MINI_GOOGLE_SVR_H__

#include <string>
#include "http_event.h"
#include "svr_base.h"
#include <queue>
#include "index_common.h"

class mini_google_event: public http_event {
    public:
        mini_google_event(svr_base *svr);

    public:
        virtual void on_process();

        void dsptch_http_request(const std::string &uri, 
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);

    private:
        void process_default(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);

        void process_put(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);
        void process_poll(const std::string &uri,
                     const std::string &req_body, std::string &rsp_head, std::string &rsp_body);
};

class mini_google_svr: public svr_base {
    public:
        virtual io_event *create_event(int fd,
                const std::string &ip, unsigned short port);
        void put(index_task_t &t);
        int poll(index_task_t &t);

    private:
        mutex_lock m_queue_lock;
        std::queue<index_task_t> m_queue;
        //mutex_lock m_lookup_tbl_lock;
    
        // lookup table

        // lock for queue
        // task queue m_queue
};

#endif
