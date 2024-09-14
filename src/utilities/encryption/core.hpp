#ifndef __CORE__H_INCLUDED__
#define __CORE__H_INCLUDED__
#include <string>
#include <vector>

namespace helper
{
    void initialize_openssl();
    void uninitialize_openssl();

    std::string aes_encrypt(
        const std::string& plaintext, 
        const std::string& key);

    std::string aes_decrypt(
        const std::string& ciphertext, 
        const std::string& key);

    std::string sha256(const std::string& input);
    std::string md5(const std::string& input);

    bool generate_key_pairs(std::string& pubkey, std::string& prikey);
    std::string RSAEncrypt(const std::string& data, const std::string& public_key);
    std::string RSADecrypt(const std::string& data, const std::string& private_key);

    
} // namespace helper


#endif // __CORE__H_INCLUDED__