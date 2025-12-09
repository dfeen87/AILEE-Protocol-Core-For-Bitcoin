// config_loader.h
#pragma once
#include "config_types.h"
#include <optional>
#include <string>

struct ConfigError {
  std::string message;
  std::string path;     // JSON/YAML path for clarity
};

struct ConfigResult {
  std::optional<Config> cfg;
  std::vector<ConfigError> errors;
};

enum class ConfigFormat { YAML, JSON, TOML };

ConfigResult load_config(const std::string& file, ConfigFormat fmt);
bool validate(const Config& cfg, std::vector<ConfigError>& errors);
