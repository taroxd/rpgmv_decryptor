#include "path_resolver.h"

#include <system_error>

namespace rpgmv {

namespace {

bool IsValidWwwRoot(const std::filesystem::path& wwwRoot) {
    std::error_code ec;
    if (!std::filesystem::exists(wwwRoot, ec) || !std::filesystem::is_directory(wwwRoot, ec)) {
        return false;
    }

    const std::filesystem::path systemJsonPath = wwwRoot / "data" / "System.json";
    const std::filesystem::path imgRoot = wwwRoot / "img";

    if (!std::filesystem::exists(systemJsonPath, ec) || !std::filesystem::is_regular_file(systemJsonPath, ec)) {
        return false;
    }
    if (!std::filesystem::exists(imgRoot, ec) || !std::filesystem::is_directory(imgRoot, ec)) {
        return false;
    }

    return true;
}

GamePaths BuildGamePaths(const std::filesystem::path& inputRoot, const std::filesystem::path& gameRoot) {
    const std::filesystem::path wwwRoot = gameRoot / "www";
    GamePaths paths;
    paths.inputRoot = inputRoot;
    paths.gameRoot = gameRoot;
    paths.wwwRoot = wwwRoot;
    paths.systemJsonPath = wwwRoot / "data" / "System.json";
    paths.imgRoot = wwwRoot / "img";
    return paths;
}

}  // namespace

std::optional<GamePaths> ResolveGamePaths(const std::filesystem::path& inputRoot, std::string& error) {
    std::error_code ec;
    if (!std::filesystem::exists(inputRoot, ec) || !std::filesystem::is_directory(inputRoot, ec)) {
        error = "Input path does not exist or is not a directory: " + inputRoot.string();
        return std::nullopt;
    }

    const std::filesystem::path directWwwRoot = inputRoot / "www";
    if (IsValidWwwRoot(directWwwRoot)) {
        return BuildGamePaths(inputRoot, inputRoot);
    }

    for (const auto& entry : std::filesystem::directory_iterator(inputRoot, ec)) {
        if (ec) {
            break;
        }

        if (!entry.is_directory()) {
            continue;
        }

        const std::filesystem::path nestedRoot = entry.path();
        const std::filesystem::path nestedWwwRoot = nestedRoot / "www";
        if (IsValidWwwRoot(nestedWwwRoot)) {
            return BuildGamePaths(inputRoot, nestedRoot);
        }
    }

    error =
        "Could not locate game data. Expected either <input>/www/data/System.json and <input>/www/img, "
        "or <input>/*/www/data/System.json and <input>/*/www/img.";
    return std::nullopt;
}

}  // namespace rpgmv
