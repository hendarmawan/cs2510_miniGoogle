#ifndef __MINI_GOOGLE_SLAVE_SVR_H__
#define __MINI_GOOGLE_SLAVE_SVR_H__

#include <string>
#include "http_event.h"
#include "svr_base.h"

class mini_google_slave_event: public http_event {
    public:
        mini_google_slave_event(svr_base *svr);

    public:
        virtual void on_process();

        void dsptch_http_request(const std::string &uri, 
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);

    private:
        void process_default(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body, const char *msg);

        void process_retrieve(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);
};

class mini_google_slave_svr: public svr_base {
    public:
        virtual io_event *create_event(int fd,
                const std::string &ip, unsigned short port);
};

#endif
