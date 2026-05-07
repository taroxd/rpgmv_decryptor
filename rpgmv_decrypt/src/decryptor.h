#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace rpgmv {

constexpr std::size_t kHeaderLength = 16;

bool ParseEncryptionKeyFromSystemJson(
    const std::filesystem::path& systemJsonPath,
    std::array<std::uint8_t, kHeaderLength>& outKey,
    std::string& error);

bool DecryptRpgmData(
    const std::vector<std::uint8_t>& encryptedData,
    const std::array<std::uint8_t, kHeaderLength>& keyBytes,
    std::vector<std::uint8_t>& decryptedData,
    std::string& error);

bool IsEncryptedExtension(const std::filesystem::path& filePath);

std::filesystem::path MapDecryptedRelativePath(const std::filesystem::path& relativeEncryptedPath);

}  // namespace rpgmv
