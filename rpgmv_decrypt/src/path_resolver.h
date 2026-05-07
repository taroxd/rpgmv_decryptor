#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace rpgmv {

struct GamePaths {
    std::filesystem::path inputRoot;
    std::filesystem::path gameRoot;
    std::filesystem::path wwwRoot;
    std::filesystem::path systemJsonPath;
    std::filesystem::path imgRoot;
};

std::optional<GamePaths> ResolveGamePaths(
    const std::filesystem::path& inputRoot,
    std::string& error);

}  // namespace rpgmv
