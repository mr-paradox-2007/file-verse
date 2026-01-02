// ===== STATE MANAGEMENT =====
let currentUser = null;
let currentPath = "/";
let currentSessionId = null;
let allFiles = [];

// ===== API BASE =====
const API_BASE = "http://localhost:9000";

// ===== INITIALIZATION =====
document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    checkAuth();
});

function setupEventListeners() {
    // Login form
    document.getElementById('loginForm').addEventListener('submit', (e) => {
        e.preventDefault();
        login();
    });

    // Signup form
    document.getElementById('signupForm').addEventListener('submit', (e) => {
        e.preventDefault();
        signup();
    });

    // Create file form
    document.getElementById('createFileForm').addEventListener('submit', (e) => {
        e.preventDefault();
        createFile();
    });

    // Create directory form
    document.getElementById('createDirForm').addEventListener('submit', (e) => {
        e.preventDefault();
        createDirectory();
    });

    // Edit file form
    document.getElementById('editFileForm').addEventListener('submit', (e) => {
        e.preventDefault();
        saveEditedFile();
    });

    // Search
    document.getElementById('searchBox').addEventListener('input', (e) => {
        filterFiles(e.target.value);
    });
}

// ===== AUTHENTICATION =====
function checkAuth() {
    const savedUser = localStorage.getItem('currentUser');
    const savedSession = localStorage.getItem('sessionId');
    
    if (savedUser && savedSession) {
        currentUser = savedUser;
        currentSessionId = savedSession;
        showDashboard();
        loadFiles();
    } else {
        showLoginScreen();
    }
}

async function login() {
    const username = document.getElementById('loginUsername').value;
    const password = document.getElementById('loginPassword').value;

    showMessage('loginError', '', 'error', false);
    showMessage('loginSuccess', '', 'success', false);

    try {
        const response = await fetch(`${API_BASE}/user/login`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ username, password })
        });

        const data = await response.json();

        if (data.success) {
            currentUser = username;
            currentSessionId = data.session_id || Math.random().toString(36).substr(2, 9);
            
            localStorage.setItem('currentUser', username);
            localStorage.setItem('sessionId', currentSessionId);

            showMessage('loginSuccess', '✓ Login successful!', 'success', true);
            
            setTimeout(() => {
                showDashboard();
                loadFiles();
            }, 500);
        } else {
            showMessage('loginError', '✗ ' + (data.error || 'Login failed'), 'error', true);
        }
    } catch (error) {
        console.error('Login error:', error);
        showMessage('loginError', '✗ Connection error', 'error', true);
    }
}

async function signup() {
    const username = document.getElementById('signupUsername').value;
    const password = document.getElementById('signupPassword').value;
    const confirm = document.getElementById('signupConfirm').value;

    showMessage('signupError', '', 'error', false);
    showMessage('signupSuccess', '', 'success', false);

    if (password !== confirm) {
        showMessage('signupError', '✗ Passwords do not match', 'error', true);
        return;
    }

    try {
        const response = await fetch(`${API_BASE}/user/signup`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ username, password })
        });

        const data = await response.json();

        if (data.success) {
            showMessage('signupSuccess', '✓ Account created! Please login.', 'success', true);
            document.getElementById('signupForm').reset();
            
            setTimeout(() => {
                toggleSignup();
            }, 1500);
        } else {
            showMessage('signupError', '✗ ' + (data.error || 'Signup failed'), 'error', true);
        }
    } catch (error) {
        console.error('Signup error:', error);
        showMessage('signupError', '✗ Connection error', 'error', true);
    }
}

function logout() {
    currentUser = null;
    currentSessionId = null;
    currentPath = "/";
    allFiles = [];
    
    localStorage.removeItem('currentUser');
    localStorage.removeItem('sessionId');
    
    showLoginScreen();
    document.getElementById('loginForm').reset();
}

// ===== UI NAVIGATION =====
function toggleSignup() {
    document.getElementById('loginScreen').classList.toggle('show');
    document.getElementById('signupScreen').classList.toggle('show');
    document.getElementById('signupForm').reset();
}

function showLoginScreen() {
    document.getElementById('loginScreen').style.display = 'flex';
    document.getElementById('signupScreen').style.display = 'none';
    document.getElementById('dashboard').style.display = 'none';
}

function showDashboard() {
    document.getElementById('loginScreen').style.display = 'none';
    document.getElementById('signupScreen').style.display = 'none';
    document.getElementById('dashboard').style.display = 'flex';
    
    // Update user info
    document.getElementById('userInfo').textContent = `Logged in as: ${currentUser}`;
}

function navigateTo(path) {
    currentPath = path;
    updateBreadcrumb();
    loadFiles();
}

function updateBreadcrumb() {
    const breadcrumb = document.getElementById('breadcrumb');
    breadcrumb.innerHTML = '<span class="breadcrumb-item active" onclick="navigateTo(\'/\')">Root</span>';
    
    if (currentPath === '/') return;
    
    let path = '';
    const parts = currentPath.split('/').filter(p => p);
    
    parts.forEach((part, index) => {
        path += '/' + part;
        const fullPath = path;
        breadcrumb.innerHTML += `<span class="breadcrumb-item" onclick="navigateTo('${fullPath}')">${part}</span>`;
    });
    
    document.getElementById('currentPath').textContent = currentPath;
}

// ===== FILE OPERATIONS =====
async function loadFiles() {
    const fileList = document.getElementById('fileList');
    fileList.innerHTML = '<div class="loading">Loading files...</div>';

    try {
        const response = await fetch(`${API_BASE}/file/list`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                path: currentPath,
                session_id: currentSessionId
            })
        });

        const data = await response.json();

        if (data.success && data.files) {
            allFiles = data.files;
            displayFiles(data.files);
            document.getElementById('fileCount').textContent = data.files.length;
        } else {
            fileList.innerHTML = '<div class="empty-state"><div class="empty-state-icon">📁</div>No files in this directory</div>';
        }
    } catch (error) {
        console.error('Load files error:', error);
        fileList.innerHTML = '<div class="error-message">Error loading files</div>';
    }
}

function displayFiles(files) {
    const fileList = document.getElementById('fileList');
    fileList.innerHTML = '';

    if (files.length === 0) {
        fileList.innerHTML = '<div class="empty-state"><div class="empty-state-icon">📁</div>No files in this directory</div>';
        return;
    }

    files.forEach(file => {
        const fileItem = document.createElement('div');
        fileItem.className = 'file-item';
        fileItem.onclick = () => openFile(file);

        const icon = file.type === 'directory' ? '📁' : '📄';
        const size = file.type === 'directory' ? '' : `${formatSize(file.size)}`;

        fileItem.innerHTML = `
            <div class="file-icon">${icon}</div>
            <div class="file-name">${file.name}</div>
            ${size ? `<div class="file-size">${size}</div>` : ''}
        `;

        fileList.appendChild(fileItem);
    });
}

function filterFiles(query) {
    const filtered = allFiles.filter(f => 
        f.name.toLowerCase().includes(query.toLowerCase())
    );
    displayFiles(filtered);
}

function formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
}

function openFile(file) {
    if (file.type === 'directory') {
        navigateTo(file.path);
    } else {
        viewFile(file);
    }
}

async function viewFile(file) {
    document.getElementById('previewTitle').textContent = file.name;
    
    try {
        const response = await fetch(`${API_BASE}/file/read`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                path: file.path,
                session_id: currentSessionId
            })
        });

        const data = await response.json();

        if (data.success) {
            const previewArea = document.getElementById('previewArea');
            previewArea.textContent = data.content || '(Empty file)';
            
            // Store current file for editing
            window.currentFile = file;
            
            document.getElementById('filePreview').style.display = 'block';
        }
    } catch (error) {
        console.error('View file error:', error);
        showToast('Error viewing file', 'error');
    }
}

function closePreview() {
    document.getElementById('filePreview').style.display = 'none';
    window.currentFile = null;
}

// ===== CREATE OPERATIONS =====
function showCreateFileModal() {
    document.getElementById('createFileModal').classList.add('show');
}

function showCreateDirModal() {
    document.getElementById('createDirModal').classList.add('show');
}

async function createFile() {
    const filename = document.getElementById('newFilename').value;
    const content = document.getElementById('fileContent').value;

    const filePath = currentPath === '/' ? '/' + filename : currentPath + '/' + filename;

    try {
        const response = await fetch(`${API_BASE}/file/create`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                path: filePath,
                content: content,
                session_id: currentSessionId
            })
        });

        const data = await response.json();

        if (data.success) {
            showToast('✓ File created successfully', 'success');
            closeModal('createFileModal');
            document.getElementById('createFileForm').reset();
            loadFiles();
        } else {
            showToast('✗ Error creating file', 'error');
        }
    } catch (error) {
        console.error('Create file error:', error);
        showToast('Connection error', 'error');
    }
}

async function createDirectory() {
    const dirname = document.getElementById('newDirname').value;
    const dirPath = currentPath === '/' ? '/' + dirname : currentPath + '/' + dirname;

    try {
        const response = await fetch(`${API_BASE}/directory/create`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                path: dirPath,
                session_id: currentSessionId
            })
        });

        const data = await response.json();

        if (data.success) {
            showToast('✓ Folder created successfully', 'success');
            closeModal('createDirModal');
            document.getElementById('createDirForm').reset();
            loadFiles();
        } else {
            showToast('✗ Error creating folder', 'error');
        }
    } catch (error) {
        console.error('Create directory error:', error);
        showToast('Connection error', 'error');
    }
}

// ===== EDIT OPERATIONS =====
function editFile() {
    if (!window.currentFile) return;
    
    const content = document.getElementById('previewArea').textContent;
    document.getElementById('editFileContent').value = content;
    
    closePreview();
    document.getElementById('editFileModal').classList.add('show');
}

async function saveEditedFile() {
    if (!window.currentFile) return;

    const newContent = document.getElementById('editFileContent').value;

    try {
        const response = await fetch(`${API_BASE}/file/edit`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                path: window.currentFile.path,
                content: newContent,
                session_id: currentSessionId
            })
        });

        const data = await response.json();

        if (data.success) {
            showToast('✓ File updated successfully', 'success');
            closeModal('editFileModal');
            loadFiles();
            window.currentFile = null;
        } else {
            showToast('✗ Error updating file', 'error');
        }
    } catch (error) {
        console.error('Edit file error:', error);
        showToast('Connection error', 'error');
    }
}

async function deleteCurrentFile() {
    if (!window.currentFile) return;

    if (!confirm('Are you sure you want to delete this file?')) return;

    try {
        const response = await fetch(`${API_BASE}/file/delete`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ 
                path: window.currentFile.path,
                session_id: currentSessionId
            })
        });

        const data = await response.json();

        if (data.success) {
            showToast('✓ File deleted successfully', 'success');
            closePreview();
            loadFiles();
        } else {
            showToast('✗ Error deleting file', 'error');
        }
    } catch (error) {
        console.error('Delete file error:', error);
        showToast('Connection error', 'error');
    }
}

// ===== MODALS =====
function closeModal(modalId) {
    document.getElementById(modalId).classList.remove('show');
}

document.querySelectorAll('.modal').forEach(modal => {
    modal.addEventListener('click', (e) => {
        if (e.target === modal) {
            modal.classList.remove('show');
        }
    });
});

// ===== UTILITY FUNCTIONS =====
function showMessage(elementId, message, type, show) {
    const element = document.getElementById(elementId);
    if (show) {
        element.textContent = message;
        element.style.display = 'block';
    } else {
        element.style.display = 'none';
    }
}

function showToast(message, type = 'info') {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.className = `toast show ${type}`;
    
    setTimeout(() => {
        toast.classList.remove('show');
    }, 3000);
}
