#ifndef __MINI_GOOGLE_COMMON_H__
#define __MINI_GOOGLE_COMMON_H__

#include <string>

int poll_task_from_master(const std::string &ip, 
        unsigned short port, std::string &file_content);

int retrieve_file(const std::string &ip, unsigned short port, 
        const std::string &file_id, std::string &file_content);

#endif
