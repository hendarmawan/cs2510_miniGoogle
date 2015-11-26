#ifndef __RPC_COMMON_H__
#define __RPC_COMMON_H__

#include <string>
#include <vector>

#define RPC_CONN_TIMEOUT 5000
#define RPC_RECV_TIMEOUT 5000
#define RPC_SEND_TIMEOUT 5000

struct svr_inst_t {
    int id;
    std::string name;
    std::string version;
    std::string ip;
    unsigned short port;
    std::string sz_port;
};

/**
 * @brief 
 *
 * @param num
 *
 * @return 
 */
std::string num_to_str(int num);

/**
 * @brief register to directory server
 *
 * @param ip
 * @param port
 * @param my_id
 * @param my_name
 * @param my_version
 * @param my_ip
 * @param my_port
 *
 * @return 
 */
int register_information(const std::string &ip, unsigned short port,
        int my_id, const std::string &my_name, const std::string &my_version,
        const std::string &my_ip, unsigned short my_port);

/**
 * @brief unregister to directory server
 *
 * @param ip
 * @param port
 * @param my_id
 * @param my_name
 * @param my_version
 * @param my_ip
 * @param my_port
 *
 * @return 
 */
int unregister_information(const std::string &ip, unsigned short port,
        int my_id, const std::string &my_name, const std::string &my_version,
        const std::string &my_ip, unsigned short my_port);

/**
 * @brief locate server instances
 *
 * @param ip address of directory server
 * @param port end point of directory server
 * @param id
 * @param version
 * @param svr_insts_list
 *
 * @return 
 */
int get_insts_by_id(const std::string &ip, unsigned short port,
        int id, const std::string &version,
        std::vector<svr_inst_t> &svr_insts_list);

/**
 * @brief get server information
 *
 * @param ip address of remote server
 * @param port end point of remote server
 * @param id
 * @param version
 *
 * @return 
 */
int get_svr_id(const std::string &ip, unsigned short port,
        int &id, std::string &version);

/**
 * @brief get and verify server
 *
 * @param ip address of directory server
 * @param port end point of directory server
 * @param id
 * @param version
 * @param svr_inst
 *
 * @return 
 */
int get_and_verify_svr(const std::string &ip, unsigned short port, 
        int id, const std::string &version, svr_inst_t &svr_inst);

#endif
