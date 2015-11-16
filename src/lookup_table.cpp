#include "lookup_table.h"

/**
 * @brief construct
 */
lookup_table::lookup_table() {
    m_locks.resize(LOOKUP_TABLE_GROUP_NUM);
    m_tables.resize(LOOKUP_TABLE_GROUP_NUM);
}

/**
 * @brief destruct
 */
lookup_table::~lookup_table() {
}

/**
 * @brief lock group, return the single_lookup_table_t data structure
 *
 * @param group_id
 *
 * @return 
 */
single_lookup_table_t *lookup_table::lock_group(int group_id) {
    if (group_id < 0 || group_id >= LOOKUP_TABLE_GROUP_NUM) {
        return NULL;
    }
    m_locks[group_id].rdlock();
    return &m_tables[group_id];
}

/**
 * @brief unlock group
 *
 * @param group_id
 */
void lookup_table::unlock_group(int group_id) {
    if (group_id < 0 || group_id >= LOOKUP_TABLE_GROUP_NUM) {
        return;
    }
    m_locks[group_id].unlock();
}

/**
 * @brief get file info
 *
 * @param file_id
 * @param file_info
 *
 * @return 
 */
int lookup_table::get_file_info(const std::string &file_id, 
        file_info_t &file_info) {

    int group_id = get_group_id(file_id);
    if (group_id < 0 || group_id >= LOOKUP_TABLE_GROUP_NUM) {
        return -1;
    }

    auto_rdlock al(&m_locks[group_id]);

    single_lookup_table_t &s_table = m_tables[group_id];
    if (s_table.count(file_id)) {
        file_info = s_table[file_id];
        return 0;
    }

    return -1;
}

/**
 * @brief set file info
 *
 * @param file_id
 * @param file_info
 *
 * @return 
 */
int lookup_table::set_file_info(const std::string &file_id, 
        const file_info_t &file_info) {

    int group_id = get_group_id(file_id);
    if (group_id < 0 || group_id >= LOOKUP_TABLE_GROUP_NUM) {
        return -1;
    }

    auto_wrlock al(&m_locks[group_id]);

    single_lookup_table_t &s_table = m_tables[group_id];
    s_table[file_id] = file_info;
    return 0;
}

/**
 * @brief get group id by file_id
 *
 * @param file_id
 *
 * @return 
 */
int lookup_table::get_group_id(const std::string &file_id) {
    std::hash<std::string> hash_file;
    std::size_t file_ind = (hash_file(file_id)) % LOOKUP_TABLE_GROUP_NUM;
    return (int)file_ind;
}
