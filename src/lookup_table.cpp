#include "lookup_table.h"

/**
 * @brief construct
 */
lookup_table::lookup_table() {
}

/**
 * @brief destruct
 */
lookup_table::~lookup_table() {
}

/**
 * @brief lock group, return the single_table_t data structure
 *
 * @param group_id
 *
 * @return 
 */
const single_table_t &lookup_table::lock_group(int group_id) {
}

/**
 * @brief unlock group
 *
 * @param group_id
 */
void lookup_table::unlock_group(int group_id) {
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
    return 0;
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
    return 0;
}
