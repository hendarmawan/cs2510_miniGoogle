#ifndef __INVERT_TABLE_H__
#define __INVERT_TABLE_H__

#include <map>
#include <vector>
#include <list>
#include "rpc_lock.h"
#include "index_common.h"

#define INVERT_TABLE_GROUP_NUM 1000

typedef std::map<std::string, int > file_freq_list_t;
typedef std::map<std::string, file_freq_list_t> single_invert_table_t;

class invert_table {
    public:
        /**
         * @brief construct
         */
        invert_table();

        /**
         * @brief destruct
         */
        virtual ~invert_table();

    public:
        /**
         * @brief group total group number
         *
         * @return 
         */
        int get_group_num() { return INVERT_TABLE_GROUP_NUM; }

        /**
         * @brief lock group, return the single_table_t data structure
         *
         * @param group_id
         *
         * @return 
         */
        single_invert_table_t *lock_group(int group_id);

        /**
         * @brief unlock group
         *
         * @param group_id
         */
        void unlock_group(int group_id);

    public:
        /**
         * @brief update invert table
         *
         * @param term
         * @param file_id
         * @param freq
         */
        int update(const std::string &term,
                const std::string &file_id, int freq);

        /**
         * @brief search invert table
         *
         * @param term
         * @param file_list
         *
         * @return 
         */
        bool search(const std::string &term,
                file_freq_list_t &file_list);

    public:
        /**
         * @brief get group id by file_id
         *
         * @param term
         *
         * @return 
         */
        int get_group_id(const std::string &term);

    private:
        std::vector<rw_lock> m_locks;
        std::vector<single_invert_table_t> m_tables;
};

#endif
