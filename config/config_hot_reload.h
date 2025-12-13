// config_hot_reload.h
#pragma once
#include "config_types.h"
#include "config_loader.h"
#include <functional>
#include <chrono>

struct ReloadOptions {
  std::string file;
  ConfigFormat fmt;
  int max_failures = 5;         // circuit breaker
  int base_backoff_ms = 250;    // exponential
};

class ConfigReloader {
public:
  using ApplyFn = std::function<void(const Config&)>;
  using LogFn   = std::function<void(const std::string&)>;

  ConfigReloader(ReloadOptions opt, ApplyFn apply, LogFn log);
  void tick(); // call periodically

private:
  ReloadOptions opt_;
  ApplyFn apply_;
  LogFn log_;
  std::string last_hash_; // file content hash for change detection
  int failures_ = 0;
  int backoff_ms_ = 0;
  std::chrono::steady_clock::time_point next_try_;
};
