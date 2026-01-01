# OFS Project - Build Instructions

## Prerequisites

- Linux/Unix environment
- g++ compiler (C++17 or later)
- OpenSSL development libraries (libssl-dev, libcrypto)
- Standard C++ libraries

### Install Dependencies (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y build-essential g++ libssl-dev libcrypto++-dev
```

## Building the Project

### Quick Build

```bash
cd file-verse
bash build.sh
```

This will:
1. Compile all core components (logger, config parser, filesystem)
2. Compile user authentication system
3. Compile file operations system
4. Compile network server and FIFO queue
5. Compile CLI client and server main
6. Run all test suites
7. Create executable binaries in `compiled/` directory

### Build Output

```
compiled/
├── format_test          # Filesystem format validation test
├── user_auth_test       # User authentication test
├── fs_core_test         # File system operations test
├── network_test         # Network server and FIFO queue test
├── ofs_server           # OFS Network Server (main executable)
└── ofs_client           # OFS CLI Client (main executable)
```

## Running the System

### Start the Server

```bash
cd file-verse
./compiled/ofs_server
```

Expected output:
```
╔════════════════════════════════════════════════════════╗
║         OFS Server v1.0                                  ║
╚════════════════════════════════════════════════════════╝

Configuration loaded from: default.uconf
  Port: 8080
  Max Connections: 10

Starting server...
✓ Server running on localhost:8080
  Press Ctrl+C to shutdown
```

### Run the CLI Client

In another terminal:

```bash
cd file-verse
./compiled/ofs_client
```

Expected output:
```
╔════════════════════════════════════════════════════════╗
║         OFS - Command Line Client v1.0                 ║
╚════════════════════════════════════════════════════════╝
Type 'help' for commands, 'exit' to quit

> 
```

### Client Commands

```
connect <host> [port]     - Connect to server (default: localhost 8080)
login <user> <pass>       - Login with credentials
logout                    - Logout from server
create <path> [perms]     - Create file (default permissions: 0644)
delete <path>             - Delete file
read <path>               - Read file contents
write <path> <data>       - Write to file
mkdir <path> [perms]      - Create directory (default permissions: 0755)
rmdir <path>              - Delete directory
ls <path>                 - List directory contents
info <path>               - Get file metadata
status                    - Show connection and login status
help                      - Show available commands
exit                      - Exit client
```

### Example Session

```bash
> connect localhost 8080
✓ Connected to server at localhost:8080

> login admin password123
✓ Logged in as admin

> create /documents 0755
✓ Directory created: /documents

> create /test.txt 0644
✓ File created: /test.txt

> write /test.txt "Hello, World!"
✓ File written: /test.txt

> read /test.txt
✓ File read: /test.txt

> ls /
Directory contents of /:
/
  ..

> logout
✓ Logged out successfully

> exit
Goodbye!
```

## Running Tests

All tests run automatically during `bash build.sh`. To run specific tests:

```bash
./compiled/format_test         # Test filesystem format
./compiled/user_auth_test      # Test user authentication
./compiled/fs_core_test        # Test file operations
./compiled/network_test        # Test network server
```

## Integration Testing

Run the integration test script:

```bash
bash integration_test.sh
```

This will:
1. Start the server in background
2. Run CLI client test session
3. Shut down the server
4. Generate logs in `logs/` directory

## Configuration

Edit `default.uconf` to customize:

```ini
[filesystem]
total_size=104857600      # 100MB filesystem
block_size=4096           # 4KB blocks
max_files=1024            # Maximum files
max_filename_length=255   # Max filename length

[security]
max_users=100             # Maximum users
admin_username=admin      # Default admin user
admin_password=password123  # Default admin password
require_auth=true         # Require authentication

[server]
port=8080                 # Server port
max_connections=10        # Max concurrent clients
queue_timeout=30          # Request queue timeout (seconds)
```

## Project Structure

```
file-verse/
├── source/
│   ├── include/           # Header files
│   ├── core/              # Filesystem implementation
│   ├── network/           # Network server
│   ├── client/            # CLI client
│   └── utils/             # Logger, config, auth
├── tests/                 # Test suites
├── compiled/              # Built executables
├── logs/                  # Log files
├── data/                  # Filesystem data (test.omni)
├── documentation/         # Documentation files
├── build.sh               # Build script
├── integration_test.sh     # Integration test
└── default.uconf          # Configuration file
```

## Troubleshooting

### Build Errors

If `build.sh` fails:
1. Ensure OpenSSL is installed: `sudo apt-get install libssl-dev libcrypto++-dev`
2. Check g++ version: `g++ --version` (must support C++17)
3. Clear old builds: `rm -rf compiled/*.o compiled/*_test compiled/ofs_*`

### Connection Issues

If CLI client cannot connect to server:
1. Verify server is running: `ps aux | grep ofs_server`
2. Check port availability: `netstat -tuln | grep 8080`
3. Verify default.uconf port setting matches client connection

### Test Failures

If tests fail during build:
1. Check log files in `logs/` directory
2. Run individual tests for details
3. Ensure data/test.omni exists (created by fs_format_test)

## Next Steps

- **GUI Client**: Implement Raylib-based graphical interface
- **Performance**: Optimize file operations and network throughput
- **Security**: Implement SSL/TLS for network communication
- **Persistence**: Add journaling for crash recovery
