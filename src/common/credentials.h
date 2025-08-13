#pragma once
#ifndef GIT_SYNC_D_CREDENTIALS_H
#define GIT_SYNC_D_CREDENTIALS_H

#include <string>

namespace Credentials {

struct Credential {
    std::string name;
    std::string username;
    std::string encryptedSecret;
};

// Add a credential. The secret will be encrypted with the provided master key
// before being persisted on disk.
bool addCredential(const std::string &name,
                   const std::string &username,
                   const std::string &secret,
                   const std::string &masterKey);

// Retrieve and decrypt a credential identified by `name`. Returns true on
// success and populates `out` with the decrypted secret.
bool getCredential(const std::string &name,
                   const std::string &masterKey,
                   Credential &out);

// Remove a credential identified by `name`.
bool removeCredential(const std::string &name);

} // namespace Credentials

#endif // GIT_SYNC_D_CREDENTIALS_H
