#ifndef __LOOKUP_TABLE_H__
#define __LOOKUP_TABLE_H__

#include <map>
#include <vector>
#include "rpc_lock.h"
#include "index_common.h"

#define LOOKUP_TABLE_GROUP_NUM 100

typedef std::map<std::string, file_info_t> single_lookup_table_t;

class lookup_table {
    public:
        /**
         * @brief construct
         */
        lookup_table();

        /**
         * @brief destruct
         */
        virtual ~lookup_table();

    public:
        /**
         * @brief group total group number
         *
         * @return 
         */
        int get_group_num() { return LOOKUP_TABLE_GROUP_NUM; }

        /**
         * @brief lock group, return the single_lookup_table_t data structure
         *
         * @param group_id
         *
         * @return 
         */
        single_lookup_table_t *lock_group(int group_id);

        /**
         * @brief unlock group
         *
         * @param group_id
         */
        void unlock_group(int group_id);

    public:
        /**
         * @brief get file info
         *
         * @param file_id
         * @param file_info
         *
         * @return 
         */
        int get_file_info(const std::string &file_id, file_info_t &file_info);

        /**
         * @brief set file info
         *
         * @param file_id
         * @param file_info
         *
         * @return 
         */
        int set_file_info(const std::string &file_id, const file_info_t &file_info);

    private:
        /**
         * @brief get group id by file_id
         *
         * @param file_id
         *
         * @return 
         */
        int get_group_id(const std::string &file_id);
        
    private:
        std::vector<rw_lock> m_locks;
        std::vector<single_lookup_table_t> m_tables;
};

#endif
