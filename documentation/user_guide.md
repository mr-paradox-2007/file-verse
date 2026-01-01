# OFS Project - User Guide

## Quick Start

### 1. Build the Project

```bash
cd file-verse
bash build.sh
```

Wait for the build to complete and all tests to pass.

### 2. Start the Server

Open a terminal and run:

```bash
cd file-verse
./compiled/ofs_server
```

You should see:
```
================================================================
           OFS Server v1.0                                      
================================================================

Configuration:
  Port: 8080
  Max Connections: 10
  Filesystem: data/test.omni

Initializing filesystem...
Filesystem initialized successfully

Starting network server...

================================================================
  Server running on localhost:8080
  Press Ctrl+C to shutdown
================================================================

Default credentials:
  Username: admin
  Password: admin123
```

### 3. Start the CLI Client

Open another terminal and run:

```bash
cd file-verse
./compiled/ofs_client
```

You will see the client prompt:
```
================================================================
           OFS - Command Line Client v1.0                 
================================================================
Type 'help' for commands, 'exit' to quit

>
```

## Basic Workflow

### Connecting to Server

```
> connect localhost 8080
Connected to server at localhost:8080
```

To connect to a different host:
```
> connect 192.168.1.100 8080
```

### Authentication

Default credentials are in `default.uconf`:
- Username: `admin`
- Password: `admin123`

Login:
```
> login admin admin123
Logged in as admin
```

Check status:
```
> status
Connected to: localhost:8080
Logged in as: admin
```

### Creating Files

```
> create /document.txt 0644
File created: /document.txt
```

Permissions:
- First digit: owner (6 = read+write)
- Second digit: group (4 = read)
- Third digit: others (4 = read)

### Writing Files

```
> write /document.txt "This is file content"
File written: /document.txt
```

Note: Data is sent after path, separated by space.

### Reading Files

```
> read /document.txt
File read: /document.txt
```

### Creating Directories

```
> mkdir /projects 0755
Directory created: /projects
```

### Listing Directories

```
> ls /
Directory contents of /:
  /document.txt
  /projects

> ls /projects
Directory contents of /projects:
```

### Getting File Information

```
> info /document.txt
Metadata for /document.txt:
size=0|owner=admin
```

### Deleting Files and Directories

Delete file:
```
> delete /document.txt
File deleted: /document.txt
```

Delete directory:
```
> rmdir /projects
Directory deleted: /projects
```

### Logging Out

```
> logout
Logged out successfully
```

### Disconnecting

```
> disconnect
Disconnected from server
```

### Exiting the Client

```
> exit
Goodbye!
```

## Command Reference

### Connection Commands

| Command    | Format                  | Example                  |
| ---------- | ----------------------- | ------------------------ |
| Connect    | `connect [host] [port]` | `connect localhost 8080` |
| Disconnect | `disconnect`            | `disconnect`             |
| Status     | `status`                | `status`                 |

### Authentication Commands

| Command | Format                        | Example                |
| ------- | ----------------------------- | ---------------------- |
| Login   | `login <username> <password>` | `login admin admin123` |
| Logout  | `logout`                      | `logout`               |

### File Commands

| Command | Format                        | Example                     |
| ------- | ----------------------------- | --------------------------- |
| Create  | `create <path> [permissions]` | `create /file.txt 0644`     |
| Delete  | `delete <path>`               | `delete /file.txt`          |
| Read    | `read <path>`                 | `read /file.txt`            |
| Write   | `write <path> <data>`         | `write /file.txt "content"` |
| Info    | `info <path>`                 | `info /file.txt`            |

### Directory Commands

| Command | Format                       | Example            |
| ------- | ---------------------------- | ------------------ |
| Create  | `mkdir <path> [permissions]` | `mkdir /docs 0755` |
| Delete  | `rmdir <path>`               | `rmdir /docs`      |
| List    | `ls <path>`                  | `ls /`             |

### Help

```
> help
```

Shows this command reference in the client.

## Configuration

Edit `default.uconf` to customize behavior:

```ini
[filesystem]
total_size=104857600      # Filesystem size (bytes)
block_size=4096           # Block size (bytes)
max_files=1024            # Max files/dirs
max_filename_length=255   # Max name length

[security]
max_users=100
admin_username=admin
admin_password=admin123
require_auth=true

[server]
port=8080                 # Server port
max_connections=10        # Max clients
queue_timeout=30          # Request timeout (seconds)
```

After changes, restart both server and client.

## Troubleshooting

### "Cannot connect to server"

**Cause**: Server not running or on different port

**Solution**:
1. Check server is running: `ps aux | grep ofs_server`
2. Verify port in default.uconf matches your connection
3. Restart server: `./compiled/ofs_server`

### "Not logged in"

**Cause**: Tried operation without logging in

**Solution**: Run `login admin admin123` first

### "File not found"

**Cause**: Trying to read/delete non-existent file

**Solution**: Check path with `ls /` first

### Commands not responding

**Cause**: Server crashed or connection lost

**Solution**:
1. Disconnect: `disconnect`
2. Reconnect: `connect localhost 8080`
3. Login again: `login admin admin123`

## Example Session

```bash
> help
================================================================
           OFS - CLI Client Help                            
================================================================

Commands:

  connect <host> [port]     - Connect to server
  disconnect                - Disconnect from server
  ...

> connect localhost 8080
Connected to server at localhost:8080

> login admin admin123
Logged in as admin

> status
Connected to: localhost:8080
Logged in as: admin

> mkdir /projects 0755
Directory created: /projects

> mkdir /projects/cppfs 0755
Directory created: /projects/cppfs

> create /projects/cppfs/README.txt 0644
File created: /projects/cppfs/README.txt

> write /projects/cppfs/README.txt "OFS - C++ Filesystem Implementation"
File written: /projects/cppfs/README.txt

> read /projects/cppfs/README.txt
File read: /projects/cppfs/README.txt

> ls /projects
Directory contents of /projects:
/projects
  cppfs

> ls /projects/cppfs
Directory contents of /projects/cppfs:
/projects/cppfs
  README.txt

> info /projects/cppfs/README.txt
Metadata for /projects/cppfs/README.txt:
size=0|owner=admin

> logout
Logged out successfully

> disconnect
Disconnected from server

> exit
Goodbye!
```

## Performance Tips

1. **Batch Operations**: Client-server latency is ~1-10ms per command
2. **Large Files**: Keep data under 4KB per write command
3. **Directory Operations**: Use `ls` to verify paths before operations
4. **Connection Reuse**: Keep one connection throughout session

## Limitations

- **No Persistence**: Data lost when server restarts (this is a known limitation in the current implementation)
- **No Multi-User**: Single user per session
- **No Concurrency**: Serial request processing
- **No Transactions**: No rollback support
- **No Encryption**: Plain text over network
- **No Compression**: All data sent as-is

## Support

For issues or questions:
1. Check logs in `logs/` directory
2. Run tests: `bash build.sh`
3. Review documentation in `documentation/` directory