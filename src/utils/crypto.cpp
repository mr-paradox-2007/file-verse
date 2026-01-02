#include "crypto.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>
#include <algorithm>
#include <ctime>

int Crypto::encryption_shift = 7;
bool Crypto::initialized = false;

void Crypto::init() {
    srand(time(nullptr));
    initialized = true;
}

std::string Crypto::simple_hash(const std::string& data) {
    unsigned long hash = 5381;
    
    for (char c : data) {
        hash = ((hash << 5) + hash) + (unsigned char)c;
    }
    
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

std::string Crypto::hash_password(const std::string& password) {
    std::string salt = generate_random(8);
    std::string hash = simple_hash(password + salt);
    return salt + ":" + hash;
}

bool Crypto::verify_password(const std::string& password, const std::string& stored_hash) {
    size_t colon_pos = stored_hash.find(':');
    if (colon_pos == std::string::npos) {
        return false;
    }
    
    std::string stored_salt = stored_hash.substr(0, colon_pos);
    std::string stored_hash_value = stored_hash.substr(colon_pos + 1);
    
    std::string computed_hash = simple_hash(password + stored_salt);
    
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

std::string Crypto::encode_content(const std::string& data) {
    std::string encoded = data;
    
    for (size_t i = 0; i < encoded.length(); i++) {
        unsigned char byte = static_cast<unsigned char>(encoded[i]);
        byte = (byte + encryption_shift) % 256;
        encoded[i] = static_cast<char>(byte);
    }
    
    return encoded;
}

std::string Crypto::decode_content(const std::string& data) {
    std::string decoded = data;
    
    for (size_t i = 0; i < decoded.length(); i++) {
        unsigned char byte = static_cast<unsigned char>(decoded[i]);
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
