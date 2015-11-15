#include "invert_table.h"

/**
 * @brief construct
 */
invert_table::invert_table() {
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
}

/**
 * @brief unlock group
 *
 * @param group_id
 */
void invert_table::unlock_group(int group_id) {
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
    return false;
}

/**
 * @brief get group id by file_id
 *
 * @param term
 *
 * @return 
 */
int invert_table::get_group_id(const std::string &term) {
    return 0;
}
