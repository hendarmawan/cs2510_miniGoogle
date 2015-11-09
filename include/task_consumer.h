#ifndef __TASK_CONSUMER_H__
#define __TASK_CONSUMER_H__

#include <pthread.h>
#include <map>
#include "rpc_common.h"

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
         * @brief set slave address
         *
         * @param ip
         * @param port
         */
        void set_slave_addr(const std::string &ip, unsigned short port) {
            m_slave_ip = ip; m_slave_port = port;
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
        static void *run_routine(void *args);

        /**
         * @brief 
         */
        void run_routine();

    private:
        std::string m_slave_ip;
        unsigned short m_slave_port;

        bool m_running;
        int m_thrds_num;
        std::vector<pthread_t> m_thrd_ids;
};

#endif
