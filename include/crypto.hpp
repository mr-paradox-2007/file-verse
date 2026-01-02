#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <string>
#include <cstdint>

/**
 * Simple Cryptographic utilities for OFS
 * - Password hashing with salt
 * - File content encoding/decoding with configurable key
 * - Key generation and validation
 */
class Crypto {
public:
    /**
     * Hash password with random salt
     * Returns: "salt:hash" format
     */
    static std::string hash_password(const std::string& password);
    
    /**
     * Verify password against stored hash
     * stored_hash format: "salt:hash"
     */
    static bool verify_password(const std::string& password, const std::string& stored_hash);
    
    /**
     * Simple hash function (for passwords)
     * Returns hex-encoded hash
     */
    static std::string simple_hash(const std::string& data);
    
    /**
     * Encode file content using Caesar shift cipher
     * Uses ENCRYPTION_SHIFT value from config
     */
    static std::string encode_content(const std::string& data);
    
    /**
     * Decode file content (reverse of encode_content)
     */
    static std::string decode_content(const std::string& data);
    
    /**
     * Generate random string
     * len: number of characters to generate
     */
    static std::string generate_random(size_t len);
    
    /**
     * Initialize crypto system
     * Should be called once at startup
     */
    static void init();

public:
    static int encryption_shift;  // Configurable Caesar cipher shift (1-25)
    static bool initialized;
};

#endif
