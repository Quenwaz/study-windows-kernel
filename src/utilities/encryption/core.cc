#include "core.hpp"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

namespace helper
{

void initialize_openssl()
{
    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

void uninitialize_openssl()
{
    // 清理OpenSSL
    EVP_cleanup();
    ERR_free_strings();

}

// Helper function to convert hex string to bytes
std::vector<unsigned char> hex_to_bytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char) strtol(byteString.c_str(), NULL, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

// Helper function to convert bytes to hex string
std::string bytes_to_hex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto& byte : data)
        ss << std::setw(2) << static_cast<int>(byte);
    return ss.str();
}

// MD5 hash
std::string md5(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);
    
    return bytes_to_hex(std::vector<unsigned char>(digest, digest + MD5_DIGEST_LENGTH));
}

// SHA256 hash
std::string sha256(const std::string& input) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);
    return bytes_to_hex(std::vector<unsigned char>(digest, digest + SHA256_DIGEST_LENGTH));
}

void generate_iv(const std::string& keystr,std::vector<unsigned char>&key, std::vector<unsigned char>& iv)
{
    std::string hashval = sha256(keystr);
    key.resize(EVP_MAX_KEY_LENGTH);  // 最大key长度，单位字节，AES-128: 6byte AES-192: 24byte AES-256: 32byte
    iv.resize(EVP_MAX_IV_LENGTH);
    std::transform(hashval.begin(), hashval.end(), key.begin(), [](char val){
        return val;
    });

    std::transform(hashval.rbegin(), hashval.rbegin() + EVP_MAX_IV_LENGTH, iv.begin(), [](char val){
        return val;
    });
}

// AES encryption
std::string aes_encrypt(const std::string& plaintext, const std::string& keystr) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::vector<unsigned char> key;
    std::vector<unsigned char> iv;
    generate_iv(keystr,key,iv);
    
    // 使用AES256 CBC加密算法， 还可以切换其他的。 如EVP_aes_256_ecb EVP_aes_128_cbc 等
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data());

    std::vector<unsigned char> ciphertext(plaintext.size() + AES_BLOCK_SIZE);
    int len = 0, ciphertext_len = 0;

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size());
    ciphertext_len = len;

    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(ciphertext_len);
    return bytes_to_hex(ciphertext);
}

// AES decryption
std::string aes_decrypt(const std::string& hextext, const std::string& keystr) {
    std::vector<unsigned char> ciphertext = hex_to_bytes(hextext);;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    std::vector<unsigned char> key;
    std::vector<unsigned char> iv;
    generate_iv(keystr,key,iv);
    EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key.data(), iv.data());

    std::vector<unsigned char> plaintext(ciphertext.size());
    int len = 0, plaintext_len = 0;

    EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext.data(), ciphertext.size());
    plaintext_len = len;

    EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len);
    plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    return std::string(plaintext.begin(), plaintext.begin() + plaintext_len);
}

bool generate_key_pairs(std::string& pubkey, std::string& prikey)
{
    RSA* rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    if (!rsa) {
        return false;
    }

    BIO* pri = BIO_new(BIO_s_mem());
    BIO* pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, rsa, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, rsa);

    int pri_len = BIO_pending(pri);
    int pub_len = BIO_pending(pub);

    prikey.resize(pri_len+1, 0);
    pubkey.resize(pub_len+1, 0);

    BIO_read(pri, prikey.data(), pri_len);
    BIO_read(pub, pubkey.data(), pub_len);

    BIO_free(pri);
    BIO_free(pub);
    RSA_free(rsa);
    return true;
}

std::string RSAEncrypt(const std::string& data, const std::string& public_key) {
    BIO* keybio = BIO_new_mem_buf(public_key.c_str(), -1);
    RSA* rsa = PEM_read_bio_RSAPublicKey(keybio, NULL, NULL, NULL);
    if (!rsa) {
        BIO_free(keybio);
        throw std::runtime_error("加载公钥失败");
    }

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> encrypted(rsa_size);
    int result = RSA_public_encrypt(data.length(), (unsigned char*)data.c_str(), encrypted.data(), rsa, RSA_PKCS1_PADDING);

    RSA_free(rsa);
    BIO_free(keybio);

    if (result == -1) {
        throw std::runtime_error("Encryption failed");
    }

    return  bytes_to_hex(encrypted);
}

std::string RSADecrypt(const std::string& data, const std::string& private_key) {
    BIO* keybio = BIO_new_mem_buf(private_key.c_str(), -1);
    RSA* rsa = PEM_read_bio_RSAPrivateKey(keybio, NULL, NULL, NULL);
    if (!rsa) {
        BIO_free(keybio);
        throw std::runtime_error("加载私钥失败");
    }

    // Convert hex string to binary
    std::vector<unsigned char> encrypted = hex_to_bytes(data);

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> decrypted(rsa_size);
    int result = RSA_private_decrypt(encrypted.size(), encrypted.data(), decrypted.data(), rsa, RSA_PKCS1_PADDING);

    RSA_free(rsa);
    BIO_free(keybio);

    if (result == -1) {
        throw std::runtime_error("Decryption failed");
    }

    return std::string(reinterpret_cast<char*>(decrypted.data()), result);
}
}