#include "invert_table.h"
#include "rpc_log.h"
#include <functional>

/**
 * @brief construct
 */
invert_table::invert_table() {
    m_locks.resize(INVERT_TABLE_GROUP_NUM);
    m_tables.resize(INVERT_TABLE_GROUP_NUM);
}

/**
 * @brief destruct
 */
invert_table::~invert_table() {
}

/**
 * @brief lock group, return the single_table_t data structure
 *
 * @param group_id
 *
 * @return 
 */
single_invert_table_t *invert_table::lock_group(int group_id) {
    if (group_id < 0 || group_id >= INVERT_TABLE_GROUP_NUM) {
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
void invert_table::unlock_group(int group_id) {
    if (group_id < 0 || group_id >= INVERT_TABLE_GROUP_NUM) {
        return;
    }
    m_locks[group_id].unlock();
}

/**
 * @brief update invert table
 *
 * @param term
 * @param file_id
 * @param freq
 */
int invert_table::update(const std::string &term,
        const std::string &file_id, int freq) {

    int group_id = get_group_id(term);
    auto_wrlock al(&m_locks[group_id]);
    
    single_invert_table_t &s_invert_table = m_tables[group_id];
    if (!s_invert_table.count(term)) {
        file_freq_list_t freq_list;
        s_invert_table[term] = freq_list;
    }
    s_invert_table[term][file_id] = file_freq_t(freq);

    return 0;
}

/**
 * @brief search invert table
 *
 * @param term
 * @param file_list
 *
 * @return 
 */
bool invert_table::search(const std::string &term,
        file_freq_list_t &file_list) {

    int group_id = get_group_id(term);
    auto_rdlock al(&m_locks[group_id]);

    single_invert_table_t &s_invert_table = m_tables[group_id];
    if(!s_invert_table.count(term)){
        return false;
    }
    file_list = s_invert_table[term];
    return true;
}

/**
 * @brief get group id by file_id
 *
 * @param term
 *
 * @return 
 */
int invert_table::get_group_id(const std::string &term) {
    std::hash<std::string> hash_file;
    std::size_t term_ind = (hash_file(term)) % INVERT_TABLE_GROUP_NUM;
    return (int)term_ind;
}
