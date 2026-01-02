#!/usr/bin/env python3
import http.server
import socketserver
import os
from pathlib import Path

PORT = 8000
WEB_DIR = Path(__file__).parent / 'web'

os.chdir(str(WEB_DIR))

class MyHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        super().end_headers()
    
    def do_GET(self):
        if self.path == '/':
            self.path = '/index.html'
        return super().do_GET()

try:
    with socketserver.TCPServer(("", PORT), MyHTTPRequestHandler) as httpd:
        print(f"✓ Web UI on http://localhost:{PORT}")
        print(f"  Backend: http://localhost:9000")
        print("  Press Ctrl+C to stop\n")
        httpd.serve_forever()
except KeyboardInterrupt:
    print("\n✓ Server stopped")
