#ifndef __FILE_MNGR_H__
#define __FILE_MNGR_H__

#include <string>
#include <vector>
#include "rpc_lock.h"

class file_mngr {
    protected:
        /**
         * @brief construct
         *
         * @param bMkdir
         */
        file_mngr(bool bMkdir);

        /**
         * @brief destruct
         */
        virtual ~file_mngr();

    public:
        /**
         * @brief create instance
         *
         * @param bMkdir
         *
         * @return 
         */
        static file_mngr *create_instance(bool bMkdir = false);

        /**
         * @brief destroy instance
         */
        static void destroy_instance();

    public:
        /**
         * @brief load file content
         *
         * @param file_id
         * @param file_content
         *
         * @return 
         */
        int load(const std::string &file_id, std::string &file_content);

        /**
         * @brief save file content
         *
         * @param file_id
         * @param file_content
         *
         * @return 
         */
        int save(const std::string &file_id, const std::string &file_content);

    private:
        /**
         * @brief 
         *
         * @param file_id
         *
         * @return 
         */
        int get_lock_index(const std::string &file_id);

    private:
        static file_mngr *m_pthis;
        std::vector<rw_lock> m_locks;
};

#endif
