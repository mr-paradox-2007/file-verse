#ifndef USER_MANAGER_HPP
#define USER_MANAGER_HPP

#include "ofs_types.hpp"
#include <string>
#include <map>
#include <memory>
#include <cstdint>

/**
 * User session handle (opaque)
 */
typedef void* OFS_Session;

/**
 * User management for OFS
 * 
 * Responsible for:
 * - User authentication and login
 * - User creation and deletion (admin only)
 * - Session management
 * - User persistence to disk
 */

/**
 * User login - creates authenticated session
 * 
 * session: Output pointer to session handle
 * username: Username to log in with
 * password: Password (will be hashed and compared)
 * 
 * Returns: SUCCESS if login successful, error code otherwise
 */
int user_login(OFS_Session* session, const std::string& username,
               const std::string& password);

/**
 * User logout - ends session and cleans up
 */
int user_logout(OFS_Session session);

/**
 * Create new user (admin only)
 * 
 * admin_session: Administrator session
 * username: New username to create
 * password: Initial password
 * role: User role (NORMAL or ADMIN)
 * 
 * Returns: SUCCESS if created, error code otherwise
 */
int user_create(OFS_Session admin_session, const std::string& username,
                const std::string& password, UserRole role);

/**
 * Delete user (admin only)
 */
int user_delete(OFS_Session admin_session, const std::string& username);

/**
 * List all users (admin only)
 * 
 * Allocates array internally (must be freed with free_buffer)
 */
int user_list(OFS_Session admin_session, UserInfo** out_users, int* out_count);

/**
 * Get current session information
 */
int get_session_info(OFS_Session session, SessionInfo* info);

/**
 * Verify if session is valid and return username
 * Returns SUCCESS if valid
 */
int verify_session(OFS_Session session, std::string& out_username, UserRole& out_role);

/**
 * Save all users to disk (called during shutdown)
 */
int save_users();

/**
 * Load all users from disk (called during init)
 */
int load_users();

#endif
