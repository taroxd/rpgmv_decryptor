#include "decryptor.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace rpgmv {

namespace {

constexpr std::array<std::uint8_t, kHeaderLength> kExpectedHeader = {
    0x52, 0x50, 0x47, 0x4D, 0x56, 0x00, 0x00, 0x00,
    0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};

std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

int HexNibble(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

bool ReadWholeTextFile(const std::filesystem::path& path, std::string& content, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        error = "Failed to open file: " + path.string();
        return false;
    }

    std::ostringstream stream;
    stream << input.rdbuf();
    if (!input.good() && !input.eof()) {
        error = "Failed while reading file: " + path.string();
        return false;
    }

    content = stream.str();
    if (content.size() >= 3 && static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        content.erase(0, 3);
    }
    return true;
}

}  // namespace

bool ParseEncryptionKeyFromSystemJson(
    const std::filesystem::path& systemJsonPath,
    std::array<std::uint8_t, kHeaderLength>& outKey,
    std::string& error) {
    std::string jsonText;
    if (!ReadWholeTextFile(systemJsonPath, jsonText, error)) {
        return false;
    }

    const std::regex keyRegex(R"json("encryptionKey"\s*:\s*"([^"]+)")json");
    std::smatch matches;
    if (!std::regex_search(jsonText, matches, keyRegex) || matches.size() < 2) {
        error = "Could not find \"encryptionKey\" in: " + systemJsonPath.string();
        return false;
    }

    const std::string rawKey = matches[1].str();
    if (rawKey.size() < (kHeaderLength * 2U)) {
        error = "encryptionKey is too short, expected at least 32 hex chars.";
        return false;
    }
    if ((rawKey.size() % 2U) != 0U) {
        error = "encryptionKey has odd length, expected an even-length hex string.";
        return false;
    }

    for (std::size_t i = 0; i < kHeaderLength; ++i) {
        const char high = rawKey[i * 2U];
        const char low = rawKey[(i * 2U) + 1U];
        const int highNibble = HexNibble(high);
        const int lowNibble = HexNibble(low);
        if (highNibble < 0 || lowNibble < 0) {
            error = "encryptionKey contains non-hex characters.";
            return false;
        }

        outKey[i] = static_cast<std::uint8_t>((highNibble << 4) | lowNibble);
    }

    return true;
}

bool DecryptRpgmData(
    const std::vector<std::uint8_t>& encryptedData,
    const std::array<std::uint8_t, kHeaderLength>& keyBytes,
    std::vector<std::uint8_t>& decryptedData,
    std::string& error) {
    if (encryptedData.size() < kHeaderLength) {
        error = "Encrypted file is smaller than RPGMV header length.";
        return false;
    }

    for (std::size_t i = 0; i < kHeaderLength; ++i) {
        if (encryptedData[i] != kExpectedHeader[i]) {
            error = "RPGMV header mismatch.";
            return false;
        }
    }

    decryptedData.assign(encryptedData.begin() + static_cast<std::ptrdiff_t>(kHeaderLength), encryptedData.end());
    const std::size_t bytesToXor = std::min<std::size_t>(kHeaderLength, decryptedData.size());
    for (std::size_t i = 0; i < bytesToXor; ++i) {
        decryptedData[i] = static_cast<std::uint8_t>(decryptedData[i] ^ keyBytes[i]);
    }

    return true;
}

bool IsEncryptedExtension(const std::filesystem::path& filePath) {
    std::string ext = ToLowerAscii(filePath.extension().string());
    return ext == ".rpgmvp" || ext == ".rpgmvo" || ext == ".rpgmvm";
}

std::filesystem::path MapDecryptedRelativePath(const std::filesystem::path& relativeEncryptedPath) {
    std::filesystem::path mappedPath = relativeEncryptedPath;
    const std::string ext = ToLowerAscii(mappedPath.extension().string());

    if (ext == ".rpgmvp") {
        mappedPath.replace_extension(".png");
    } else if (ext == ".rpgmvo") {
        mappedPath.replace_extension(".ogg");
    } else if (ext == ".rpgmvm") {
        mappedPath.replace_extension(".m4a");
    }

    return mappedPath;
}

}  // namespace rpgmv
