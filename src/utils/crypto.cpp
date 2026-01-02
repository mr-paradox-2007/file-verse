#include "crypto.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>
#include <algorithm>
#include <ctime>

int Crypto::encryption_shift = 7;  // Default shift for Caesar cipher (can be 1-25)
bool Crypto::initialized = false;

void Crypto::init() {
    // Initialize random seed
    srand(time(nullptr));
    initialized = true;
}

// Simple polynomial hash for passwords
std::string Crypto::simple_hash(const std::string& data) {
    unsigned long hash = 5381;
    
    for (char c : data) {
        hash = ((hash << 5) + hash) + (unsigned char)c;  // hash * 33 + c
    }
    
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

std::string Crypto::hash_password(const std::string& password) {
    // Generate random salt (8 random characters)
    std::string salt = generate_random(8);
    
    // Hash password + salt
    std::string hash = simple_hash(password + salt);
    
    // Return "salt:hash" format
    return salt + ":" + hash;
}

bool Crypto::verify_password(const std::string& password, const std::string& stored_hash) {
    // Extract salt and hash
    size_t colon_pos = stored_hash.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string stored_salt = stored_hash.substr(0, colon_pos);
    std::string stored_hash_value = stored_hash.substr(colon_pos + 1);
    
    // Compute hash of provided password with stored salt
    std::string computed_hash = simple_hash(password + stored_salt);
    
    // Constant-time comparison to prevent timing attacks
    bool match = true;
    if (stored_hash_value.length() != computed_hash.length()) {
        match = false;
    } else {
        for (size_t i = 0; i < stored_hash_value.length(); i++) {
            if (stored_hash_value[i] != computed_hash[i]) {
                match = false;
            }
        }
    }
    
    return match;
}

// Caesar cipher - shift each byte by encryption_shift
std::string Crypto::encode_content(const std::string& data) {
    std::string encoded = data;
    
    for (size_t i = 0; i < encoded.length(); i++) {
        unsigned char byte = static_cast<unsigned char>(encoded[i]);
        // Apply Caesar shift
        byte = (byte + encryption_shift) % 256;
        encoded[i] = static_cast<char>(byte);
    }
    
    return encoded;
}

// Reverse of Caesar cipher
std::string Crypto::decode_content(const std::string& data) {
    std::string decoded = data;
    
    for (size_t i = 0; i < decoded.length(); i++) {
        unsigned char byte = static_cast<unsigned char>(decoded[i]);
        // Reverse Caesar shift
        byte = (byte - encryption_shift + 256) % 256;
        decoded[i] = static_cast<char>(byte);
    }
    
    return decoded;
}

std::string Crypto::generate_random(size_t len) {
    const char* charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    
    for (size_t i = 0; i < len; i++) {
        result += charset[rand() % 62];
    }
    
    return result;
}
