# User Guide

## Table of Contents

1. System Requirements
2. Installation and Setup
3. Starting the Server
4. Using the Web Interface
5. File Operations
6. User Management
7. Configuration
8. Troubleshooting
9. Advanced Usage

## 1. System Requirements

### Hardware Requirements

- **CPU**: Any modern x86-64 processor
- **RAM**: Minimum 512MB, recommended 1GB
- **Disk Space**: Minimum 100MB for system files + desired filesystem size

### Software Requirements

- **Operating System**: Linux (Ubuntu 20.04+ or equivalent)
- **Compiler**: g++ with C++17 support
- **Libraries**:
  - pthread (threading support)
  - OpenSSL (for cryptographic functions)
- **Browser**: Modern web browser (Chrome, Firefox, Edge, Safari)

### Installing Dependencies

Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

## 2. Installation and Setup

### Step 1: Extract Project Files

```bash
unzip BSAI24056_phase1.zip
cd BSAI24056_phase1
```

### Step 2: Verify Directory Structure

```
BSAI24056_phase1/
├── src/
│   ├── core/
│   ├── network/
│   └── utils/
├── include/
├── web/
├── compiled/
├── data/
├── logs/
├── build.sh
├── default.uconf
└── README.md
```

### Step 3: Build the Project

```bash
chmod +x build.sh
./build.sh
```

Expected output:
```
Building OFS - Complete System
[1/4] Compiling storage engine...
[2/4] Compiling core modules...
[3/4] Compiling utilities...
[4/4] Linking server...
Build Complete!
```

### Step 4: Verify Build

```bash
ls compiled/
```

Should show: `server` and various `.o` files

## 3. Starting the Server

### Start Server

```bash
./compiled/server
```

### Expected Output

```
=====================================
  OFS Multi-User File System
=====================================
[*] Initializing storage...
[*] Creating new filesystem...
[*] Loading users...
[*] Creating socket...
[*] Binding to port 9000...
[SUCCESS] Server running on http://localhost:9000
[INFO] Default admin account: admin / admin123
[INFO] Press Ctrl+C to shutdown
```

### Stopping Server

Press `Ctrl+C` in the terminal to shutdown gracefully.

### Running in Background

```bash
nohup ./compiled/server > server.log 2>&1 &
```

To stop:
```bash
pkill -f "./compiled/server"
```

## 4. Using the Web Interface

### Accessing the Interface

1. Open web browser
2. Navigate to: `http://localhost:9000`
3. You should see the login screen

### Login Screen

![Login Interface]

**Default Admin Account**:
- Username: `admin`
- Password: `admin123`

**First-time Users**:
- Click "Create Account" button
- Fill in desired username and password
- Click "Create Account"
- Return to login screen and login

### Dashboard Overview

After login, you'll see:

1. **Navigation Bar** (top):
   - System title
   - Current username
   - Encryption status indicator
   - Logout button

2. **Sidebar** (left):
   - Breadcrumb navigation
   - Quick action buttons
   - System information

3. **Main Content** (center):
   - File browser
   - Search bar
   - Sort options
   - File list grid

### Navigation

**Breadcrumb Path**:
- Shows current location
- Click any part to navigate to that directory
- Root is always visible

**Current Path Display**:
- Shows full path in sidebar
- Updates as you navigate

## 5. File Operations

### Creating Files

1. Click "New File" button in sidebar
2. Enter filename (no spaces or special characters recommended)
3. Optionally enter file content
4. Click "Create"

**Restrictions**:
- Maximum filename length: 31 characters
- Cannot use: `/`, `\`, `..`, or start with `.`

### Reading Files

1. Click on file in file list
2. File preview panel appears
3. View file content

**Supported**:
- Text files
- Any content stored as text

### Editing Files

1. Open file (click on it)
2. Click "Edit" button in preview panel
3. Modify content in text area
4. Click "Save Changes"

**Note**: Edit replaces entire file content

### Deleting Files

1. Open file (click on it)
2. Click "Delete" button in preview panel
3. Confirm deletion
4. File is permanently removed

**Warning**: Deletion is permanent, no recovery

### Searching Files

1. Type filename in search box
2. Results filter automatically
3. Clear search box to show all files

**Search Characteristics**:
- Case-insensitive
- Partial match
- Searches current directory only

### Sorting Files

Use sort dropdown to order files:
- **By Name**: Alphabetical order
- **By Date**: Most recent first
- **By Type**: Directories first, then files

## 6. Directory Operations

### Creating Directories

1. Click "New Folder" button in sidebar
2. Enter directory name
3. Click "Create"

**Restrictions**:
- Maximum name length: 31 characters
- Cannot use special characters

### Navigating Directories

1. Click on directory in file list
2. View contents
3. Use breadcrumb to go back

### Deleting Directories

**Requirement**: Directory must be empty

1. Delete all files and subdirectories first
2. Navigate to parent directory
3. (Currently requires file operation)

## 7. User Management

### Creating New Account

**From Login Screen**:
1. Click "Create Account"
2. Enter username (3-31 characters)
3. Enter password (minimum 4 characters)
4. Confirm password
5. Click "Create Account"

### Account Requirements

**Username**:
- 3-31 characters
- Alphanumeric recommended
- Case-sensitive
- Must be unique

**Password**:
- Minimum 4 characters
- Case-sensitive
- Hashed before storage
- No recovery mechanism (remember it!)

### Session Management

**Automatic**:
- Session created on login
- Validated with each operation
- Stored in browser localStorage

**Manual Logout**:
- Click "Logout" button
- Session terminated
- Redirected to login screen

### Concurrent Users

**Restrictions**:
- Same username cannot login twice
- Each user has separate session
- File operations are sequential (FIFO)

**Behavior**:
- Multiple different users can login simultaneously
- Each operates independently
- All see same filesystem
- No file locking (last write wins)

## 8. Configuration

### Configuration File: default.uconf

Located in project root:

```ini
[filesystem]
total_size = 104857600        # 100MB
header_size = 512
block_size = 4096             # 4KB blocks
max_files = 1000
max_filename_length = 256

[security]
max_users = 50
admin_username = admin
admin_password = admin123
require_auth = true

[server]
port = 9000
max_connections = 20
queue_timeout = 30
```

### Changing Configuration

1. Stop server
2. Edit `default.uconf`
3. Delete `data/system.omni` (to recreate filesystem)
4. Restart server

**Warning**: Deleting .omni file loses all data

### Important Settings

**total_size**: Total filesystem capacity
- Default: 100MB
- Minimum: 10MB
- Maximum: Limited by disk space

**block_size**: Storage block size
- Default: 4KB
- Must be power of 2
- Affects performance and space efficiency

**max_users**: Maximum user accounts
- Default: 50
- Affects memory usage
- Can be increased if needed

**port**: Server listening port
- Default: 9000
- Change if port conflict
- Update browser URL accordingly

## 9. Troubleshooting

### Server Won't Start

**Error**: "Bind failed"
```bash
# Check if port is in use
sudo netstat -tulpn | grep 9000

# Kill existing process
sudo kill -9 <PID>

# Or change port in default.uconf
```

**Error**: "Failed to open filesystem"
```bash
# Check file permissions
ls -l data/system.omni

# Fix permissions
chmod 666 data/system.omni
```

### Cannot Login

**Symptom**: "Invalid username or password"
- Verify credentials
- Check if user exists
- Username and password are case-sensitive

**Symptom**: "User already logged in"
- User logged in from another browser/tab
- Logout from other session
- Or wait for session timeout

### Files Not Loading

**Symptom**: "Loading files..." never completes
- Check browser console for errors
- Verify server is running
- Check network connectivity
- Verify session is valid

### Browser Issues

**Clear Cache**:
1. Press Ctrl+Shift+Delete
2. Clear cache and cookies
3. Reload page

**Try Different Browser**:
- Chrome/Chromium
- Firefox
- Edge

## 10. Advanced Usage

### Viewing Logs

**Application Logs**:
```bash
tail -f logs/ofs.log
```

**Server Output**:
- Shown in terminal where server runs
- Can redirect to file:
```bash
./compiled/server > server_output.log 2>&1
```

### Checking System Status

**Free Space**:
- Shown in dashboard (future enhancement)
- Check .omni file size:
```bash
ls -lh data/system.omni
```

**Active Sessions**:
- Tracked internally
- No external viewer currently

### Performance Tuning

**Block Size**:
- Smaller blocks: Less waste, more overhead
- Larger blocks: More waste, less overhead
- Default 4KB is good balance

**Max Files**:
- Higher limit: More capacity, more memory
- Lower limit: Less memory, restricted capacity

### Backup and Restore

**Backup**:
```bash
cp data/system.omni data/system.omni.backup
```

**Restore**:
```bash
./compiled/server  # Stop server first (Ctrl+C)
cp data/system.omni.backup data/system.omni
./compiled/server  # Restart
```

### Multiple Instances

**Run on Different Ports**:
1. Copy project directory
2. Edit default.uconf (change port)
3. Edit web/script.js (change API_BASE port)
4. Run server

**Separate Filesystems**:
- Each instance has own .omni file
- No data sharing between instances

## 11. Command Reference

### Build Commands

```bash
# Clean build
rm -rf compiled/*.o compiled/server
./build.sh

# Check dependencies
gcc --version
g++ --version
```

### Server Commands

```bash
# Start server
./compiled/server

# Start in background
nohup ./compiled/server &

# Stop server
pkill -f "./compiled/server"

# Check if running
ps aux | grep server
```

### File System Commands

```bash
# Check size
du -h data/system.omni

# Check structure
file data/system.omni

# Backup
cp data/system.omni backups/system_$(date +%Y%m%d).omni
```

## 12. Security Considerations

### Password Security

- Passwords hashed with SHA-256
- Salt added to prevent rainbow table attacks
- Never stored in plain text

### Session Security

- Unique session ID per login
- Stored in browser localStorage
- Validated on each request

### Network Security

- HTTP only (not HTTPS)
- Suitable for localhost/trusted network
- For production: add HTTPS support

### File Encryption

- Simple byte substitution cipher
- Not cryptographically secure
- Sufficient for project requirements

## 13. Known Limitations

### Current Limitations

1. **No File Permissions**: All users can access all files
2. **No Directory Permissions**: All users can modify structure
3. **No File Locking**: Concurrent edits possible
4. **No Version History**: No undo capability
5. **Limited Search**: Current directory only
6. **No File Upload**: Text input only

### Planned Enhancements (Phase 2)

1. Version control (Delta Vault)
2. Change tracking
3. Rollback capability
4. History viewing

## 14. Getting Help

### Documentation

- README.md: Quick start guide
- design_choices.md: Technical details
- file_io_strategy.md: Implementation details

### Common Issues

**Port Already in Use**:
- Change port in default.uconf
- Or kill existing process

**Build Fails**:
- Install dependencies
- Check g++ version
- Verify file permissions

**UI Not Loading**:
- Check server is running
- Verify port number
- Check browser console

**Session Errors**:
- Clear browser cache
- Logout and login again
- Check server logs

### Support

For issues or questions:
1. Check logs: `logs/ofs.log`
2. Review documentation
3. Check troubleshooting section
4. Verify system requirements

## 15. Best Practices

### General Usage

1. **Regular Backups**: Backup .omni file regularly
2. **Logout When Done**: Free up sessions
3. **Use Descriptive Names**: Easy to identify files
4. **Organize with Directories**: Keep files structured
5. **Check Free Space**: Monitor system capacity

### Multi-User Environment

1. **Coordinate**: Communicate with other users
2. **Avoid Conflicts**: Don't edit same file simultaneously
3. **Be Considerate**: Don't monopolize system
4. **Report Issues**: Help improve system

### Performance

1. **Small Files**: Better performance than large files
2. **Fewer Levels**: Shallow directory structure preferred
3. **Regular Cleanup**: Delete unused files
4. **Monitor Logs**: Check for errors regularly

## 16. Quick Reference

### File Size Limits

- Max file size: Limited by free space
- Max filename: 31 characters
- Max path depth: No practical limit
- Total files: 8192 maximum

### Port Information

- Default: 9000
- Configurable: Edit default.uconf
- Check availability: `netstat -tulpn | grep 9000`

### File Locations

- Binary: `compiled/server`
- Config: `default.uconf`
- Data: `data/system.omni`
- Logs: `logs/ofs.log`
- Web UI: `web/index.html`

### Keyboard Shortcuts

Browser standard shortcuts:
- Ctrl+R: Refresh page
- Ctrl+Shift+R: Hard refresh
- F12: Developer tools

### URL Structure

- Main: `http://localhost:9000/`
- API: `http://localhost:9000/user/login` (etc.)
- Static: `http://localhost:9000/web/style.css`