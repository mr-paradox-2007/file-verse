#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include "ofs_types.hpp"
#include <string>
#include <map>
#include <memory>
#include <cstdint>

typedef void* OFS_Session;

int user_login(OFS_Session* session, const std::string& username,
               const std::string& password);

int user_logout(OFS_Session session);

int user_create(OFS_Session admin_session, const std::string& username,
                const std::string& password, UserRole role);

int user_create(const std::string& username, const std::string& password);

int user_delete(OFS_Session admin_session, const std::string& username);

int user_list(OFS_Session admin_session, UserInfo** out_users, int* out_count);

int get_session_info(OFS_Session session, SessionInfo* info);

int verify_session(OFS_Session session, std::string& out_username, UserRole& out_role);

int save_users();

int load_users();

#endif
