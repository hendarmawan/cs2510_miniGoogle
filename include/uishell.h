#ifndef __UISHELL_H__
#define __UISHELL_H__

#include <string>
#include "index_common.h"
#include "mini_google_common.h"

typedef std::vector<std::pair<std::string, unsigned short> > slaves_list_t;

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
        /**
         * @brief cache slave address
         */
        void cache_slave_address();

    private:
        /* last cache msec */
        unsigned long long m_last_cache_msec;

        /* master address */
        std::string m_master_ip;
        unsigned short m_master_port;

        /* slave list */
        slaves_list_t m_slaves_list;
};

#endif
