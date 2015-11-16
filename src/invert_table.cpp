#include "invert_table.h"

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
const single_invert_table_t &invert_table::lock_group(int group_id) {
    rw_lock rwLock = m_locks.at(group_id);
    rwLock.wrlock();
    return m_tables.at(group_id);
}

/**
 * @brief unlock group
 *
 * @param group_id
 */
void invert_table::unlock_group(int group_id) {
    rw_lock rwLock = m_locks.at(group_id);
    rwLock.unlock();
}

/**
 * @brief update invert table
 *
 * @param term
 * @param file_id
 * @param freq
 */
void invert_table::update(const std::string &term, 
        const std::string &file_id, int freq) {
    int group_id = get_group_id(term);
    rw_lock rwLock = m_locks.at(group_id);
    rwLock.wrlock();
    single_invert_table_t s_invert_table = m_tables.at(group_id);
    if(s_invert_table.count(term)){  //if has such term
        file_freq_list_t ffl = s_invert_table[term];
        ffl[file_id] = freq;
    }
    else{
        file_freq_list_t freq_list;
        freq_list[file_id] = freq;
        s_invert_table[term] = freq_list;
    }
    rwLock.unlock();
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
    rw_lock rwLock = m_locks.at(group_id);
    rwLock.rdlock();
    single_invert_table_t s_invert_table = m_tables.at(group_id);
    if(!s_invert_table.count(term)){
        return false;
    }
    else{
        file_list = s_invert_table[term];
    }
    rwLock.unlock();
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
    return (int) term_ind;
    return 0;
}
