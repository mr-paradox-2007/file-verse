# OFS User Guide

## Quick Start

### Building the Project

```bash
cd file-verse
bash build.sh
```

Expected output:
```
==========================================
  OFS Build System
==========================================

[1/11] Compiling logger.cpp...
  logger.o compiled
[2/11] Compiling config_parser.cpp...
  config_parser.o compiled
...
[11/11] Linking ofs_client executable...
  ofs_client executable created

==========================================
  Build Complete
==========================================

Executables created:
  ./compiled/ofs_server
  ./compiled/ofs_client
```

### Starting the Server

**Terminal 1:**
```bash
./compiled/ofs_server
```

Expected output:
```
================================================================
           OFS Server v1.0
================================================================

Configuration:
  Port: 8080
  Max Connections: 20
  Filesystem: data/test.omni

Filesystem initialized successfully
Admin user created: admin

================================================================
  Server running on localhost:8080
  Press Ctrl+C to shutdown
================================================================

Default credentials:
  Username: admin
  Password: admin123
```

### Starting the Client

**Terminal 2:**
```bash
./compiled/ofs_client
```

Expected output:
```
================================================================
           OFS - Command Line Client v1.0
================================================================

Welcome to OFS CLI Client
Type 'help' at any menu for more information


================================================================
                 OFS - Client Main Menu
================================================================

[1] Connection Management
[2] Authentication
[3] File Operations
[4] Directory Operations
[5] View System Status
[6] Show Help
[0] Exit

Select option:
```

## Configuration

### Configuration File (default.uconf)

```ini
[filesystem]
total_size = 104857600           # 100MB total
block_size = 4096                # 4KB blocks
max_files = 1000                 # Maximum files
max_filename_length = 10         # Max name length

[security]
max_users = 50                   # Max user accounts
admin_username = "admin"         # Default admin
admin_password = "admin123"      # Default password
require_auth = true              # Authentication required

[server]
port = 8080                      # Server port
max_connections = 20             # Max concurrent clients
queue_timeout = 30               # Request timeout (seconds)
```

**To modify:**
1. Edit default.uconf
2. Restart server for changes to take effect

## Using the Client

### Menu Navigation

The client uses a hierarchical menu system. Use numbers to select options.

**Main Menu:**
```
[1] Connection Management
[2] Authentication
[3] File Operations
[4] Directory Operations
[5] View System Status
[6] Show Help
[0] Exit
```

**Navigation:**
- Enter a number to select an option
- [0] always returns to previous menu
- Invalid input shows error and returns to menu

### Connection Management

**Select option: 1**

```
================================================================
              Connection Management Menu
================================================================

[1] Connect to Server
[2] Disconnect from Server
[3] Check Connection Status
[0] Back to Main Menu

Select option:
```

#### Connect to Server

**Select option: 1**

```
Enter server host (default: localhost): localhost
Enter server port (default: 8080): 8080
[OK] Connected to server at localhost:8080
```

**For remote server:**
```
Enter server host (default: localhost): 192.168.1.100
Enter server port (default: 8080): 8080
```

#### Check Connection Status

**Select option: 3**

```
Connected to: localhost:8080
```

Or if not connected:
```
Not connected
```

### Authentication

**Select option: 2**

```
================================================================
                  Authentication Menu
================================================================

[1] Login
[2] Logout
[3] Current User Status
[0] Back to Main Menu

Select option:
```

#### Login

**Select option: 1**

```
Enter username: admin
Enter password: admin123
[OK] Logged in as admin
```

**Default credentials:**
- Username: admin
- Password: admin123

**Error messages:**
```
[FAIL] Login failed: ERROR|Login failed
```

#### Logout

**Select option: 2**

```
[OK] Logged out successfully
```

#### Check User Status

**Select option: 3**

```
Logged in as: admin
```

Or if not logged in:
```
Not logged in
```

### File Operations

**Select option: 3**

```
================================================================
                  File Operations Menu
================================================================

[1] Create File
[2] Delete File
[3] Read File
[4] Write to File
[5] Get File Info
[0] Back to Main Menu

Select option:
```

#### Create File

**Select option: 1**

```
Enter file path: /test.txt
Enter permissions (default: 0644): 0644
[OK] File created: /test.txt
```

**Permissions format:**
- 0644 = Owner: read+write, Others: read
- 0755 = Owner: read+write+execute, Others: read+execute
- 0600 = Owner: read+write, Others: none

#### Write to File

**Select option: 4**

```
Enter file path: /test.txt
Enter content to write: Hello, OFS!
[OK] File written: /test.txt
```

#### Read File

**Select option: 3**

```
Enter file path to read: /test.txt
[OK] File read: /test.txt
File contents:
Hello, OFS!
```

#### Get File Info

**Select option: 5**

```
Enter file path: /test.txt
Metadata for /test.txt:
size=11|owner=admin|permissions=420
```

#### Delete File

**Select option: 2**

```
Enter file path to delete: /test.txt
[OK] File deleted: /test.txt
```

### Directory Operations

**Select option: 4**

```
================================================================
              Directory Operations Menu
================================================================

[1] Create Directory
[2] Delete Directory
[3] List Directory
[0] Back to Main Menu

Select option:
```

#### Create Directory

**Select option: 1**

```
Enter directory path: /documents
Enter permissions (default: 0755): 0755
[OK] Directory created: /documents
```

#### List Directory

**Select option: 3**

```
Enter directory path: /
Directory contents of /:
/
  test.txt
  documents
```

#### Delete Directory

**Select option: 2**

```
Enter directory path to delete: /documents
[OK] Directory deleted: /documents
```

### System Status

**Select option: 5**

```
================================================================
                      System Status
================================================================

Connection Status: Connected
  Server: localhost:8080

Authentication Status: Logged In
  User: admin
```

Or if not connected:
```
Connection Status: Not Connected
Authentication Status: Not Logged In
```

### Help

**Select option: 6**

Shows command reference and usage examples.

## Complete Workflow Example

### Create and Use a File

```
1. Start server: ./compiled/ofs_server
2. Start client: ./compiled/ofs_client
3. Select [1] Connection Management
4. Select [1] Connect to Server
5. Enter: localhost, 8080
6. Select [0] Back to Main Menu
7. Select [2] Authentication
8. Select [1] Login
9. Enter: admin, admin123
10. Select [0] Back to Main Menu
11. Select [3] File Operations
12. Select [1] Create File
13. Enter: /myfile.txt, 0644
14. Select [4] Write to File
15. Enter: /myfile.txt, "Sample content"
16. Select [3] Read File
17. Enter: /myfile.txt
18. (Content displayed)
19. Select [0] Back to Main Menu
20. Select [2] Authentication
21. Select [2] Logout
22. Select [0] Back to Main Menu
23. Select [0] Exit
```

## Multi-User Testing

### Terminal Setup

**Terminal 1:** Server
```bash
./compiled/ofs_server
```

**Terminal 2:** Client 1 (admin)
```bash
./compiled/ofs_client
# Connect, login as admin
# Create file: /shared/data.txt
```

**Terminal 3:** Client 2 (admin - same user)
```bash
./compiled/ofs_client
# Connect, login as admin
# Read file: /shared/data.txt
```

**Both clients can:**
- Connect simultaneously
- Operate on the same files
- See changes made by other clients (after reconnecting)

## Troubleshooting

### Cannot Connect to Server

**Error:** "Error: Cannot connect to server"

**Solutions:**
1. Verify server is running: `ps aux | grep ofs_server`
2. Check port 8080 is not blocked: `netstat -tuln | grep 8080`
3. Try different port in default.uconf
4. Check firewall settings

### Login Failed

**Error:** "[FAIL] Login failed"

**Solutions:**
1. Verify credentials: admin / admin123
2. Check server logs: `tail logs/ofs.log`
3. Ensure server initialized properly

### File Not Found

**Error:** "[FAIL] Failed to read file"

**Solutions:**
1. List directory to verify file exists: [4] → [3] → /
2. Check path spelling (case-sensitive)
3. Verify you're logged in before operations

### File Already Exists

**Error:** "[FAIL] Failed to create file: -3"

**Solutions:**
1. File already exists at that path
2. Use different filename
3. Delete existing file first

### Port Already in Use

**Error:** "Failed to bind socket to port 8080"

**Solutions:**
1. Kill process using port: `sudo fuser -k 8080/tcp`
2. Change port in default.uconf
3. Wait a few seconds and retry

### Filesystem Corrupted

**Symptoms:** Unexpected errors, crashes

**Solutions:**
1. Stop server: Ctrl+C
2. Delete filesystem: `rm data/test.omni`
3. Restart server (creates new filesystem)

## Testing

### Automated Test Script

```bash
bash test.sh
```

Runs complete test suite:
- Server startup
- Client connection
- Authentication
- File operations
- Directory operations
- Multi-client scenarios

### Manual Testing

Follow these scenarios:

**Test 1: Basic Operations**
1. Create file
2. Write content
3. Read content
4. Verify content matches

**Test 2: Error Handling**
1. Try to read non-existent file (should fail)
2. Try to create duplicate file (should fail)
3. Try operations without login (should fail)

**Test 3: Multi-User**
1. Open two clients
2. Both connect and login
3. Client 1 creates file
4. Client 2 reads same file
5. Verify both see the file

## Logs

### Server Log Location
```
./logs/ofs.log
```

### View Logs
```bash
# View all logs
cat logs/ofs.log

# View last 50 lines
tail -50 logs/ofs.log

# Follow logs in real-time
tail -f logs/ofs.log
```

### Log Format
```
[TIMESTAMP] level=LEVEL module=MODULE code=CODE msg="MESSAGE" file="FILE" line=LINE
```

**Example:**
```
[2026-01-01T12:00:00Z] level=INFO module=NET_SRV code=0 msg="Server started successfully" file="server.cpp" line=45
```

## Performance Tips

### Response Times
- File create: ~1-5ms
- File read: <1ms
- File write: ~1-5ms
- Directory list: <1ms

### Concurrent Clients
- Tested with 20 clients simultaneously
- All operations complete successfully
- Average response time increases with load

### File Size Limits
- Individual file: Limited by available space
- Total filesystem: 100MB (configurable)
- Recommended: Keep files under 1MB each

## System Requirements

### Server
- Linux operating system
- 100MB disk space
- 50MB RAM (for in-memory operations)
- Network port 8080 available

### Client
- Any system with network access
- Can run on same machine as server (localhost)
- Or remote machine (LAN/WAN)

## Support

### Getting Help
1. Check this guide
2. Review logs: `logs/ofs.log`
3. Run test script: `bash test.sh`
4. Check README.md

### Common Issues
- All covered in Troubleshooting section
- Check logs for detailed error messages
- Verify configuration file is correct

## Summary

**To use OFS:**
1. Build: `bash build.sh`
2. Start server: `./compiled/ofs_server`
3. Start client: `./compiled/ofs_client`
4. Connect: Menu [1] → [1] → localhost:8080
5. Login: Menu [2] → [1] → admin:admin123
6. Use file operations: Menu [3]
7. Use directory operations: Menu [4]

**Default credentials:** admin / admin123

**All operations logged to:** logs/ofs.log