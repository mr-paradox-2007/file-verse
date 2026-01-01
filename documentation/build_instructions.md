# OFS - Quick Start Guide (Final)

## One-Command Test

Run this to verify everything works:

```bash
cd file-verse && bash build.sh && bash test_system.sh
```

## Step-by-Step

### 1. Build (30 seconds)

```bash
cd file-verse
bash build.sh
```

Expected: All files compile, no errors.

### 2. Start Server (Terminal 1)

```bash
./compiled/ofs_server
```

Expected: Server starts on port 8080.

### 3. Start Client (Terminal 2)

```bash
./compiled/ofs_client
```

Expected: Client prompt appears.

### 4. Test Commands

```
connect localhost 8080
login admin admin123
mkdir /test 0755
create /test/file.txt 0644
write /test/file.txt "Hello World"
read /test/file.txt
ls /test
logout
exit
```

Expected: All commands succeed.

## Troubleshooting

### Build fails
```bash
sudo apt-get install build-essential g++ libssl-dev libcrypto++-dev
```

### Server won't start
- Check port 8080 is free: `netstat -tuln | grep 8080`
- Check logs: `cat logs/ofs.log`

### Client can't connect
- Verify server is running: `ps aux | grep ofs_server`
- Check firewall settings

## Files Created

After running:
- `compiled/ofs_server` - Server executable
- `compiled/ofs_client` - Client executable
- `data/test.omni` - Filesystem file
- `logs/ofs.log` - System log

## Default Credentials

- Username: `admin`
- Password: `admin123`

## Success Indicators

Server is working if you see:
```
Server running on localhost:8080
```

Client is working if you see:
```
> connect localhost 8080
Connected to server at localhost:8080

> login admin admin123
Logged in as admin
```

## Ready for Submission

If all above works, the system is ready. All requirements are implemented:
- Filesystem operations
- User authentication
- Network server
- CLI client
- Logging system
- Configuration management
- Documentation

## Next Steps

For Raylib GUI development:
1. Keep server running as-is
2. Create new client in `../raylib_native/`
3. Use same network protocol (see cli_client.cpp)
4. Reuse connection logic

## Support

Check these if problems occur:
- `logs/ofs.log` - System log
- `documentation/user_guide.md` - Full commands
- `documentation/build_instructions.md` - Detailed build steps
- `README.md` - Project overview