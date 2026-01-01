Manual Testing Guide for OFS CLI Client

OVERVIEW
The OFS CLI Client provides a menu-driven interface to test all filesystem operations.
The client connects to the OFS Server and allows you to perform complete testing.

QUICK START

Terminal 1 - Start the Server:
  cd file-verse
  ./compiled/ofs_server

Terminal 2 - Start the Client:
  cd file-verse
  ./compiled/ofs_client

MENU STRUCTURE

MAIN MENU
  [1] Connection Management
  [2] Authentication
  [3] File Operations
  [4] Directory Operations
  [5] View System Status
  [6] Show Help
  [0] Exit


CONNECTION MANAGEMENT (Menu Option 1)
  [1] Connect to Server
      Enter host: localhost
      Enter port: 8080
      Result: [OK] Connected to server at localhost:8080
  
  [2] Disconnect from Server
      Result: [OK] Disconnected from server
  
  [3] Check Connection Status
      Shows current connection state


AUTHENTICATION (Menu Option 2)
  [1] Login
      Username: admin
      Password: admin123
      Result: [OK] Logged in as admin
  
  [2] Logout
      Result: [OK] Logged out successfully
  
  [3] Current User Status
      Shows logged in user or "Not logged in"


FILE OPERATIONS (Menu Option 3)
  
  [1] Create File
      Path: /myfile.txt
      Permissions: 0644 (default)
      Result: [OK] File created: /myfile.txt
  
  [2] Delete File
      Path: /myfile.txt
      Result: [OK] File deleted: /myfile.txt
  
  [3] Read File
      Path: /myfile.txt
      Result: [OK] File read: /myfile.txt
      Shows file contents
  
  [4] Write to File
      Path: /myfile.txt
      Content: Hello, OFS!
      Result: [OK] File written: /myfile.txt
  
  [5] Get File Info
      Path: /myfile.txt
      Shows: size, owner, permissions


DIRECTORY OPERATIONS (Menu Option 4)
  
  [1] Create Directory
      Path: /mydir
      Permissions: 0755 (default)
      Result: [OK] Directory created: /mydir
  
  [2] Delete Directory
      Path: /mydir
      Result: [OK] Directory deleted: /mydir
  
  [3] List Directory
      Path: /
      Shows directory contents


SYSTEM STATUS (Menu Option 5)
  Shows current connection and authentication status


HELP (Menu Option 6)
  Shows command help and examples


COMPLETE TEST WORKFLOW

1. Start Server:
   ./compiled/ofs_server

2. Start Client:
   ./compiled/ofs_client

3. Select [1] Connection Management
   Select [1] Connect to Server
   Server: localhost
   Port: 8080

4. Select [2] Authentication
   Select [1] Login
   Username: admin
   Password: admin123

5. Select [3] File Operations
   [1] Create File: /test.txt (perms: 0644)
   [4] Write to File: /test.txt (content: "Hello World")
   [3] Read File: /test.txt
   [5] Get File Info: /test.txt
   [2] Delete File: /test.txt

6. Select [4] Directory Operations
   [1] Create Directory: /data (perms: 0755)
   [3] List Directory: /
   [2] Delete Directory: /data

7. Select [5] View System Status
   Verify connection and user status

8. Select [2] Authentication
   Select [2] Logout

9. Select [1] Connection Management
   Select [2] Disconnect from Server

10. Select [0] Exit


ERROR HANDLING

Common Errors and Solutions:

"Error: Not connected to server"
  - Go to Connection Management
  - Select [1] Connect to Server
  - Verify server is running

"Error: Not logged in"
  - Go to Authentication
  - Select [1] Login
  - Use: admin / admin123

"Error: File not found"
  - Create the file first using File Operations [1]

"Error: Directory not found"
  - Create the directory first using Directory Operations [1]


DEFAULT CREDENTIALS
  Username: admin
  Password: admin123


LOGS
  Server logs: logs/ofs.log
  All operations are logged with timestamps


KEYBOARD SHORTCUTS
  Ctrl+C: Interrupt current operation or exit
  Enter: Submit menu selection
  Any invalid input: Shows "Invalid option" and returns to current menu


TROUBLESHOOTING

If client can't connect:
  - Check server is running: ps aux | grep ofs_server
  - Verify port 8080 is not blocked: netstat -tuln | grep 8080
  - Check firewall settings

If commands fail:
  - Check logs: cat logs/ofs.log
  - Verify you are logged in
  - Try simple operations first (create file, list dir)

If filesystem is corrupted:
  - Stop server: Ctrl+C
  - Remove filesystem: rm data/test.omni
  - Restart server: ./compiled/ofs_server
