OFS - Quick Start Guide

GETTING STARTED IN 5 MINUTES

Step 1: Build the Project
  cd file-verse
  bash build.sh

Expected: All components compile successfully, no errors

Step 2: Open Terminal 1 - Start Server
  cd file-verse
  ./compiled/ofs_server

Expected output:
  ================================================================
             OFS Server v1.0
  ================================================================
  
  Configuration:
    Port: 8080
    Max Connections: 20
    Filesystem: data/test.omni
  
  Filesystem initialized successfully
  Creating default admin user...
  Admin user created: admin
  
  ================================================================
    Server running on localhost:8080
    Press Ctrl+C to shutdown
  ================================================================
  
  Default credentials:
    Username: admin
    Password: admin123

Step 3: Open Terminal 2 - Start Client
  cd file-verse
  ./compiled/ofs_client

Expected output:
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


BASIC WORKFLOW

1. Connect to Server
   Select: 1 (Connection Management)
   Select: 1 (Connect to Server)
   Host: localhost (or press Enter for default)
   Port: 8080 (or press Enter for default)
   Result: [OK] Connected to server at localhost:8080

2. Login
   Select: 0 (Back to Main Menu)
   Select: 2 (Authentication)
   Select: 1 (Login)
   Username: admin
   Password: admin123
   Result: [OK] Logged in as admin

3. Create a File
   Select: 0 (Back to Main Menu)
   Select: 3 (File Operations)
   Select: 1 (Create File)
   Path: /test.txt
   Permissions: 0644 (or press Enter for default)
   Result: [OK] File created: /test.txt

4. Write to File
   Select: 4 (Write to File)
   Path: /test.txt
   Content: Hello, OFS!
   Result: [OK] File written: /test.txt

5. Read File
   Select: 3 (Read File)
   Path: /test.txt
   Result: [OK] File read: /test.txt
   (Shows file contents)

6. Create Directory
   Select: 0 (Back to Main Menu)
   Select: 4 (Directory Operations)
   Select: 1 (Create Directory)
   Path: /documents
   Permissions: 0755 (or press Enter for default)
   Result: [OK] Directory created: /documents

7. List Directory
   Select: 3 (List Directory)
   Path: /
   (Shows root directory contents)

8. Check Status
   Select: 0 (Back to Main Menu)
   Select: 5 (View System Status)
   Result: Shows connection and user info

9. Logout
   Select: 0 (Back to Main Menu)
   Select: 2 (Authentication)
   Select: 2 (Logout)
   Result: [OK] Logged out successfully

10. Disconnect
    Select: 0 (Back to Main Menu)
    Select: 1 (Connection Management)
    Select: 2 (Disconnect from Server)
    Result: [OK] Disconnected from server

11. Exit
    Select: 0 (Back to Main Menu)
    Select: 0 (Exit)
    Result: Thank you for using OFS Client. Goodbye!


MENU NAVIGATION TIPS

- Use numbers 0-6 to select menu options
- Invalid input shows "Invalid option" and repeats menu
- [0] always goes back to previous menu
- All operations have clear prompts
- [OK] indicates success, [FAIL] indicates error
- Invalid operations show error messages


COMMON OPERATIONS

Create Test File:
  [1] [1] localhost 8080 [3] [1] [0] [2] [1] admin admin123
  [3] [1] /myfile.txt 0644
  [3] [4] /myfile.txt "Test content"
  [3] [3] /myfile.txt

List Root:
  [4] [3] /

Delete File:
  [3] [2] /myfile.txt

Get File Info:
  [3] [5] /myfile.txt

View Status:
  [5]


KEYBOARD SHORTCUTS

Ctrl+C          - Interrupt and stop (works in menus and at prompts)
Enter           - Submit menu selection or continue
Backspace       - Delete character in input fields


TROUBLESHOOTING

Cannot Connect:
  Problem: "Error: Cannot connect to server"
  Solution: Make sure server is running in Terminal 1

Login Failed:
  Problem: "Login failed" message
  Solution: Check credentials (username: admin, password: admin123)

File Not Found:
  Problem: "File not found" error
  Solution: Create file first with option [3][1]

Port Already in Use:
  Problem: Server won't start (bind error)
  Solution: Kill other process: sudo fuser -k 8080/tcp

Filesystem Corrupted:
  Problem: Errors when reading/writing
  Solution: 
    1. Stop server (Ctrl+C in Terminal 1)
    2. Remove filesystem: rm data/test.omni
    3. Restart server


TESTING THE SYSTEM

Quick Test:
  bash test_system.sh

This runs automated tests for:
  - Server startup
  - Client connection
  - Authentication
  - File operations
  - Directory operations

Manual Testing:
  Follow the Basic Workflow above

Comprehensive Testing:
  See documentation/manual_test_guide.md


FILE LOCATIONS

Server Executable:   ./compiled/ofs_server
Client Executable:   ./compiled/ofs_client
Server Config:       ./default.uconf
Filesystem Data:     ./data/test.omni
Server Logs:         ./logs/ofs.log
Documentation:       ./documentation/


AFTER TESTING

For Submission:
  - Run automated tests: bash test_system.sh
  - Check documentation is complete
  - Verify all operations work
  - Check logs are being written

For Raylib Development:
  - Server stays unchanged
  - Create new GUI client using Raylib
  - Reference cli_client.cpp for network protocol
  - Server works with both CLI and GUI clients


HELP AND DOCUMENTATION

Show Help in Client:
  Select option: 6 (Show Help)

User Guide:
  See documentation/user_guide.md

Architecture:
  See documentation/architecture_design.md

Testing Guide:
  See documentation/manual_test_guide.md

Project Overview:
  See README.md

Completion Status:
  See PROJECT_COMPLETION_SUMMARY.md


NEXT STEPS

1. Build:     bash build.sh
2. Run Server: ./compiled/ofs_server
3. Run Client: ./compiled/ofs_client
4. Test:      bash test_system.sh
5. Develop:   Create Raylib GUI client

Good luck!
