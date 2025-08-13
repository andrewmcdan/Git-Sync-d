#include "credentials.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

#include <cryptopp/aes.h>
#include <cryptopp/base64.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/osrng.h>
#include <cryptopp/sha.h>

namespace Credentials {
namespace {
const char *kCredentialFile = "credentials.dat";

std::string hashKey(const std::string &masterKey) {
    CryptoPP::SHA256 hash;
    std::string digest;
    CryptoPP::StringSource(masterKey, true,
                           new CryptoPP::HashFilter(hash,
                                                     new CryptoPP::StringSink(digest)));
    return digest; // 32 bytes
}

std::string encrypt(const std::string &plain, const std::string &masterKey) {
    using namespace CryptoPP;
    AutoSeededRandomPool prng;
    std::string cipher;
    std::string key = hashKey(masterKey);
    byte iv[AES::BLOCKSIZE];
    prng.GenerateBlock(iv, sizeof(iv));
    cipher.assign(reinterpret_cast<char *>(iv), AES::BLOCKSIZE);
    CBC_Mode<AES>::Encryption enc;
    enc.SetKeyWithIV(reinterpret_cast<const byte *>(key.data()), key.size(), iv);
    StringSource ss(plain, true,
                    new StreamTransformationFilter(enc,
                                                   new StringSink(cipher),
                                                   BlockPaddingSchemeDef::PKCS_PADDING));
    return cipher;
}

std::string decrypt(const std::string &cipher, const std::string &masterKey) {
    using namespace CryptoPP;
    if (cipher.size() < AES::BLOCKSIZE)
        return {};
    std::string key = hashKey(masterKey);
    const byte *iv = reinterpret_cast<const byte *>(cipher.data());
    std::string enc = cipher.substr(AES::BLOCKSIZE);
    std::string plain;
    CBC_Mode<AES>::Decryption dec;
    dec.SetKeyWithIV(reinterpret_cast<const byte *>(key.data()), key.size(), iv);
    StringSource ss(enc, true,
                    new StreamTransformationFilter(dec,
                                                   new StringSink(plain),
                                                   BlockPaddingSchemeDef::PKCS_PADDING));
    return plain;
}

std::string b64encode(const std::string &input) {
    std::string output;
    CryptoPP::StringSource(input, true,
                           new CryptoPP::Base64Encoder(
                               new CryptoPP::StringSink(output), false));
    return output;
}

std::string b64decode(const std::string &input) {
    std::string output;
    CryptoPP::StringSource(input, true,
                           new CryptoPP::Base64Decoder(
                               new CryptoPP::StringSink(output)));
    return output;
}

std::vector<Credential> loadAll() {
    std::vector<Credential> creds;
    std::ifstream in(kCredentialFile);
    if (!in.is_open())
        return creds;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string name, user, enc;
        if (std::getline(iss, name, '|') && std::getline(iss, user, '|') &&
            std::getline(iss, enc)) {
            creds.push_back({name, user, enc});
        }
    }
    return creds;
}

bool saveAll(const std::vector<Credential> &creds) {
    std::ofstream out(kCredentialFile, std::ios::trunc);
    if (!out.is_open())
        return false;
    for (const auto &c : creds) {
        out << c.name << '|' << c.username << '|' << c.encryptedSecret << '\n';
    }
    return true;
}

} // namespace

bool addCredential(const std::string &name,
                   const std::string &username,
                   const std::string &secret,
                   const std::string &masterKey) {
    auto creds = loadAll();
    std::string enc = b64encode(encrypt(secret, masterKey));
    auto it = std::find_if(creds.begin(), creds.end(),
                           [&](const Credential &c) { return c.name == name; });
    if (it != creds.end()) {
        it->username = username;
        it->encryptedSecret = enc;
    } else {
        creds.push_back({name, username, enc});
    }
    return saveAll(creds);
}

bool getCredential(const std::string &name,
                   const std::string &masterKey,
                   Credential &out) {
    auto creds = loadAll();
    auto it = std::find_if(creds.begin(), creds.end(),
                           [&](const Credential &c) { return c.name == name; });
    if (it == creds.end())
        return false;
    out.name = it->name;
    out.username = it->username;
    out.encryptedSecret = decrypt(b64decode(it->encryptedSecret), masterKey);
    return true;
}

bool removeCredential(const std::string &name) {
    auto creds = loadAll();
    auto origSize = creds.size();
    creds.erase(std::remove_if(creds.begin(), creds.end(),
                               [&](const Credential &c) { return c.name == name; }),
                creds.end());
    if (creds.size() == origSize)
        return false;
    return saveAll(creds);
}

} // namespace Credentials
