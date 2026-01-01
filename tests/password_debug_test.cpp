#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <random>
#include <cstring>

std::string generateSalt()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 16; i++)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << dis(gen);
    }

    return ss.str();
}

std::string hashPassword(const std::string& password, const std::string& salt)
{
    // Combine password and salt
    std::string combined = password + salt;

    // Compute SHA-256
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<unsigned char*>(const_cast<char*>(combined.c_str())),
           combined.length(),
           hash);

    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << std::hex << std::setfill('0') << std::setw(2) << (int)hash[i];
    }

    return ss.str();
}

bool verifyPassword(const std::string& password, const std::string& stored_hash)
{
    // Extract salt and hash from stored format: "salt:hash"
    size_t colon_pos = stored_hash.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0)
    {
        std::cout << "Invalid stored hash format" << std::endl;
        return false;
    }

    std::string salt = stored_hash.substr(0, colon_pos);
    std::string expected_hash = stored_hash.substr(colon_pos + 1);

    std::cout << "Extracted salt: " << salt << std::endl;
    std::cout << "Expected hash: " << expected_hash << std::endl;

    // Hash the provided password with the extracted salt
    std::string computed_hash = hashPassword(password, salt);
    std::cout << "Computed hash: " << computed_hash << std::endl;

    // Compare hashes
    return computed_hash == expected_hash;
}

int main()
{
    std::string password = "admin123";
    std::string salt = generateSalt();
    
    std::cout << "Password: " << password << std::endl;
    std::cout << "Generated salt: " << salt << " (length: " << salt.length() << ")" << std::endl;

    std::string hash = hashPassword(password, salt);
    std::cout << "Hash: " << hash << " (length: " << hash.length() << ")" << std::endl;

    std::string stored = salt + ":" + hash;
    std::cout << "\nStored format: " << stored << std::endl;
    std::cout << "Stored length: " << stored.length() << std::endl;

    std::cout << "\n--- Testing verification ---\n" << std::endl;

    bool result = verifyPassword(password, stored);
    std::cout << "\nVerification result: " << (result ? "SUCCESS" : "FAILED") << std::endl;

    return 0;
}
