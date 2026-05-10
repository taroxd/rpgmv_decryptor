#include "path_resolver.h"

#include <system_error>

namespace rpgmv {

namespace {

bool IsValidContentRoot(const std::filesystem::path& contentRoot) {
    std::error_code ec;
    if (!std::filesystem::exists(contentRoot, ec) || !std::filesystem::is_directory(contentRoot, ec)) {
        return false;
    }

    const std::filesystem::path systemJsonPath = contentRoot / "data" / "System.json";
    const std::filesystem::path imgRoot = contentRoot / "img";

    if (!std::filesystem::exists(systemJsonPath, ec) || !std::filesystem::is_regular_file(systemJsonPath, ec)) {
        return false;
    }
    if (!std::filesystem::exists(imgRoot, ec) || !std::filesystem::is_directory(imgRoot, ec)) {
        return false;
    }

    return true;
}

GamePaths BuildGamePaths(
    const std::filesystem::path& inputRoot,
    const std::filesystem::path& gameRoot,
    const std::filesystem::path& contentRoot) {
    GamePaths paths;
    paths.inputRoot = inputRoot;
    paths.gameRoot = gameRoot;
    // Kept as-is for backward compatibility with existing struct fields.
    paths.wwwRoot = contentRoot;
    paths.systemJsonPath = contentRoot / "data" / "System.json";
    paths.imgRoot = contentRoot / "img";
    return paths;
}

}  // namespace

std::optional<GamePaths> ResolveGamePaths(const std::filesystem::path& inputRoot, std::string& error) {
    std::error_code ec;
    if (!std::filesystem::exists(inputRoot, ec) || !std::filesystem::is_directory(inputRoot, ec)) {
        error = "Input path does not exist or is not a directory: " + inputRoot.string();
        return std::nullopt;
    }

    if (IsValidContentRoot(inputRoot)) {
        return BuildGamePaths(inputRoot, inputRoot, inputRoot);
    }

    std::error_code firstLevelEc;
    for (const auto& entry : std::filesystem::directory_iterator(inputRoot, firstLevelEc)) {
        if (firstLevelEc) {
            break;
        }

        if (!entry.is_directory()) {
            continue;
        }

        const std::filesystem::path nestedRoot = entry.path();
        if (IsValidContentRoot(nestedRoot)) {
            return BuildGamePaths(inputRoot, nestedRoot, nestedRoot);
        }

        std::error_code secondLevelEc;
        for (const auto& secondEntry : std::filesystem::directory_iterator(nestedRoot, secondLevelEc)) {
            if (secondLevelEc) {
                break;
            }
            if (!secondEntry.is_directory()) {
                continue;
            }

            const std::filesystem::path nestedNestedRoot = secondEntry.path();
            if (IsValidContentRoot(nestedNestedRoot)) {
                return BuildGamePaths(inputRoot, nestedRoot, nestedNestedRoot);
            }
        }
    }

    error =
        "Could not locate game data. Expected one of:\n"
        "  <input>/data/System.json and <input>/img\n"
        "  <input>/*/data/System.json and <input>/*/img\n"
        "  <input>/*/*/data/System.json and <input>/*/*/img";
    return std::nullopt;
}

}  // namespace rpgmv
