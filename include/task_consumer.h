#ifndef __TASK_CONSUMER_H__
#define __TASK_CONSUMER_H__

#include <pthread.h>
#include <map>
#include "rpc_lock.h"
#include "rpc_common.h"

#define DS_IP "127.0.0.1"
#define DS_PORT 8000
#define RPC_MASTER_ID 520
#define RPC_MASTER_VERSION "1.0.0"
#define RPC_MASTER_NAME "mini_google_master"
#define RPC_SLAVE_ID 521
#define RPC_SLAVE_NAME "mini_google_slave"
#define RPC_SLAVE_VERSION "1.0.0"

class task_consumer {
    public:
        /**
         * @brief construct
         */
        task_consumer();

        /**
         * @brief destruct
         */
        virtual ~task_consumer();

    public:
        /**
         * @brief create working threads
         *
         * @param thrds_num
         *
         * @return 
         */
        int run(int thrds_num);

        /**
         * @brief stop
         */
        void stop() { m_running = false; }

        /**
         * @brief join
         */
        void join();

        /**
         * @brief check whether is running
         *
         * @return 
         */
        bool is_running() { return m_running; }

    public:
        /**
         * @brief set master address
         *
         * @param ip
         * @param port
         */
        void set_master_addr(const std::string &ip, 
                unsigned short port) {
            m_master_ip = ip; 
            m_master_port = port;
        }

        /**
         * @brief set slave address
         *
         * @param ip
         * @param port
         */
        void set_slave_addr(const std::string &ip, 
                unsigned short port) {
            m_slave_ip = ip; 
            m_slave_port = port;
        }

    private:
        /**
         * @brief try to consume a task
         *
         * @return 
         */
        int consume();

    private:
        /**
         * @brief 
         *
         * @param args
         *
         * @return 
         */
        static void *task_routine(void *args);

        /**
         * @brief 
         */
        void task_routine();

    private:
        /* master address */
        std::string m_master_ip;
        unsigned short m_master_port;

        /* slave address */
        std::string m_slave_ip;
        unsigned short m_slave_port;

        /* running flag */
        bool m_running;

        /* thread handles */
        int m_thrds_num;
        std::vector<pthread_t> m_thrd_ids;
};

#endif
