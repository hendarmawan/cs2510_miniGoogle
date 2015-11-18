#ifndef __MINI_GOOGLE_SVR_H__
#define __MINI_GOOGLE_SVR_H__

#include <string>
#include "http_event.h"
#include "svr_base.h"
#include <queue>
#include "index_common.h"
#include <map>
#include "lookup_table.h"
#include "invert_table.h"

class mini_google_event: public http_event {
    public:
        /**
         * @brief construct
         *
         * @param svr
         */
        mini_google_event(svr_base *svr);

    public:
        /**
         * @brief process callback
         */
        virtual void on_process();

        /**
         * @brief dispatcher
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
         * @brief default response, 404 + msg
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         * @param msg
         */
        void process_default(
                const std::string &uri,
                const std::string &req_body, 
                std::string &rsp_head, 
                std::string &rsp_body,
                const char *msg = NULL);

    private:
        void process_query(const std::string &uri,
                           const std::string &req_body, std::string &rsp_head, std::string &rsp_body);
        void process_retrieve(const std::string &uri,
                        const std::string &req_body, std::string &rsp_head, std::string &rsp_body);

    private:
        /**
         * @brief append new task
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void process_put(
                const std::string &uri,
                const std::string &req_body, 
                std::string &rsp_head, 
                std::string &rsp_body);

        /**
         * @brief pop a task from task queue
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void process_poll(
                const std::string &uri,
                const std::string &req_body, 
                std::string &rsp_head, 
                std::string &rsp_body);

        /**
         * @brief process report from workers
         *
         * @param uri
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void process_report(
                const std::string &uri,
                const std::string &req_body, 
                std::string &rsp_head, 
                std::string &rsp_body);

        /**
         * @brief backup
         *
         * @param uri params:[table|method|group_id]
         * @param req_body
         * @param rsp_head
         * @param rsp_body
         */
        void process_backup(
                const std::string &uri, 
                const std::string &req_body, 
                std::string &rsp_head, 
                std::string &rsp_body);
};

class mini_google_svr: public svr_base {
    public:
        virtual io_event *create_event(int fd,
                const std::string &ip, unsigned short port);

        /**
         * @brief append task to task queue
         *
         * @param task
         *
         * @return number of tasks after put
         */
        int put(const index_task_t &task);

        /**
         * @brief pop a task from task queue
         *
         * @param task
         *
         * @return number of tasks after poll
         */
        int poll(index_task_t &task);

        int query(const std::string &uri);
        int retrieve(const std::string &file_id, file_info_t &file_info);
    
    public:
        /**
         * @brief get invert table
         *
         * @return 
         */
        invert_table &get_invert_table() {
            return m_invert_table;
        }

        /**
         * @brief get lookup table
         *
         * @return 
         */
        lookup_table &get_lookup_table() {
            return m_lookup_table;
        }
    
    private:
        mutex_lock m_queue_lock;
        std::queue<index_task_t> m_queue;
        mutex_lock invert_table_lock;
        //std::map<std::string, std::list<std::pair<std::string, int> > > invert_table;  //inverted table
        mutex_lock lookup_lock;
        //std::map<std::string, file_info_t > lookup_table;
        //mutex_lock m_lookup_tbl_lock;
    
        // lookup table

        // lock for queue
        // task queue m_queue
        invert_table m_invert_table;
        lookup_table m_lookup_table;
};

#endif
