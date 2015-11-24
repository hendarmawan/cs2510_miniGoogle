#ifndef __UISHELL_H__
#define __UISHELL_H__

#include <string>
#include "index_common.h"
#include "mini_google_common.h"

class uishell {
    public:
        /**
         * @brief construct
         */
        uishell();

        /**
         * @brief destruct
         */
        virtual ~uishell();

    public:
        /**
         * @brief initialize a communication between uishell and master
         *
         * @param ip
         * @param port
         *
         * @return 
         */
        int open(const std::string &ip, unsigned short port);

        /**
         * @brief query for documents
         *
         * @param str_query
         * @param query_result
         *
         * @return 
         */
        int query(const std::string &str_query, 
                std::vector<query_result_t> &query_result);

        /**
         * @brief retrieve file content
         *
         * @param file_id
         * @param file_content
         *
         * @return 
         */
        int retrieve(const std::string &file_id, 
                std::string &file_content);

        /**
         * @brief close communication
         */
        void close();

    private:
        std::string m_ip;
        unsigned short m_port;

        std::string m_master_ip;
        unsigned short m_master_port;
};

#endif
