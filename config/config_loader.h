// config_loader.h
#pragma once

#include "config_types.h"
#include <optional>
#include <string>
#include <vector>

enum class ConfigFormat {
  YAML,
  JSON,
  TOML,
};

struct ConfigError {
  std::string message;
  std::string path;
};

struct ConfigResult {
  std::optional<Config> cfg;
  std::vector<ConfigError> errors;
  std::string raw_text;
};

ConfigResult load_config(const std::string& file, ConfigFormat fmt);
bool validate(const Config& cfg, std::vector<ConfigError>& errors);
