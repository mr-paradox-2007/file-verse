#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <string>
#include <cstdint>

class Crypto {
public:
    static std::string hash_password(const std::string& password);
    
    static bool verify_password(const std::string& password, const std::string& stored_hash);
    
    static std::string simple_hash(const std::string& data);
    
    static std::string encode_content(const std::string& data);
    
    static std::string decode_content(const std::string& data);
    
    static std::string generate_random(size_t len);
    
    static void init();

public:
    static int encryption_shift;
    static bool initialized;
};

#endif
