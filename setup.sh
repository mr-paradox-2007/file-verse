#!/bin/bash

echo "╔════════════════════════════════════════╗"
echo "║  OFS - Complete Setup Script           ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

info() {
    echo -e "${BLUE}[*]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Check if we're in the right directory
if [ ! -f "build.sh" ]; then
    error "Please run this script from the working-file-verse directory"
fi

info "Checking dependencies..."

# Check for g++
if ! command -v g++ &> /dev/null; then
    error "g++ not found. Please install: sudo apt-get install build-essential"
fi
success "g++ found"

# Check for pthread support
if ! ldconfig -p | grep -q pthread; then
    warn "pthread might not be available (but should work anyway)"
else
    success "pthread found"
fi

# Create necessary directories
info "Creating directories..."
mkdir -p compiled logs data web include src/core src/utils src/network
success "Directories created"

# Make build script executable
info "Setting permissions..."
chmod +x build.sh
if [ -f "test_system.sh" ]; then
    chmod +x test_system.sh
fi
success "Permissions set"

# Build the project
info "Building project..."
if ./build.sh; then
    success "Build successful"
else
    error "Build failed. Check error messages above."
fi

# Check if web files exist
info "Checking web interface..."
if [ ! -f "web/index.html" ]; then
    warn "web/index.html not found - web UI may not work"
else
    success "Web UI files found"
fi

# Test if we can create the .omni file
info "Testing file system creation..."
if [ -f "data/system.omni" ]; then
    warn "data/system.omni already exists (keeping existing file)"
else
    success "Ready to create filesystem on first run"
fi

echo ""
echo "╔════════════════════════════════════════╗"
echo "║  Setup Complete!                       ║"
echo "╚════════════════════════════════════════╝"
echo ""
echo "Next steps:"
echo ""
echo "  1. Start the server:"
echo "     ${GREEN}./compiled/server${NC}"
echo ""
echo "  2. Open your browser:"
echo "     ${BLUE}http://localhost:9000${NC}"
echo ""
echo "  3. Login with:"
echo "     Username: ${YELLOW}admin${NC}"
echo "     Password: ${YELLOW}admin123${NC}"
echo ""
echo "Optional:"
echo "  - Run automated tests: ${GREEN}./test_system.sh${NC}"
echo "  - View logs: ${GREEN}tail -f logs/ofs.log${NC}"
echo "  - Read guide: ${GREEN}cat DEPLOYMENT_GUIDE.md${NC}"
echo ""