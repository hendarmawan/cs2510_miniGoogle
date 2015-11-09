#ifndef __DS_SVR_H__
#define __DS_SVR_H__

#include <map>
#include <list>
#include "ezxml.h"
#include "http_event.h"
#include "rpc_common.h"
#include "svr_base.h"

/* id + version -> svr_insts_list_t */
typedef std::list<std::pair<svr_inst_t, unsigned long long> > svr_insts_list_t;
typedef std::map<std::string, svr_insts_list_t> svc_map_t;

class ds_event: public http_event {
    public:
        /**
         * @brief construct
         *
         * @param svr
         */
        ds_event(svr_base *svr);

    public:
        /**
         * @brief notify process event(multi-threads call)
         */
        virtual void on_process();

        /**
         * @brief route http request to spesific function
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void dsptch_http_request(const std::string &uri, 
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);

    private:
        /**
         * @brief default process
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void process_default(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);

        /**
         * @brief process register request
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         * @param flag: true for register and false for unregister
         */
        void process_register(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body, bool flag);

        /**
         * @brief process get_insts_by_id request
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void process_get_insts_by_id(const std::string &uri,
                const std::string &req_body, std::string &rsp_head, std::string &rsp_body);
};

class ds_svr: public svr_base {
    public:
        /**
         * @brief construct
         */
        ds_svr();

        /**
         * @brief destruct
         */
        virtual ~ds_svr();

    public:
        /**
         * @brief create event
         *
         * @param fd
         * @param ip
         * @param port
         *
         * @return 
         */
        virtual io_event *create_event(int fd,
                const std::string &ip, unsigned short port);

    public:
        /**
         * @brief register svr
         *
         * @param svr
         */
        void do_register(svr_inst_t &svr);

        /**
         * @brief unregister svr
         *
         * @param svr
         */
        void do_unregister(svr_inst_t &svr);

        /**
         * @brief get server insts by id of service
         *
         * @param id
         * @param version
         * @param svr_insts_list
         */
        void do_get_insts_by_id(int id, const std::string &version,
                std::vector<svr_inst_t> &svr_insts_list);

        /**
         * @brief check timeout
         */
        void check_timeout();

    private:
        spin_lock m_lock;
        svc_map_t m_svc_map;
};

#endif
