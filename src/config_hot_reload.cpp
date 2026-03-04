// config_hot_reload.cpp
#include "config_hot_reload.h"
#include "crypto_utils.h"
#include <functional>
#include <string>
#include <chrono>
#include <thread>

extern ConfigResult load_config(const std::string&, ConfigFormat);

ConfigReloader::ConfigReloader(ReloadOptions opt, ApplyFn apply, LogFn log)
  : opt_(opt), apply_(apply), log_(log) {
  next_try_ = std::chrono::steady_clock::now();
}

void ConfigReloader::tick() {
  auto now = std::chrono::steady_clock::now();
  if (now < next_try_) return;

  // Read raw file
  ConfigResult res = load_config(opt_.file, opt_.fmt);
  if (!res.cfg) {
    failures_++;
    if (failures_ >= opt_.max_failures) {
      log_("config: circuit breaker TRIPPED after "+std::to_string(failures_)+" failures");
      next_try_ = now + std::chrono::hours(24); // circuit breaker tripped — call reset() to resume
      return;
    }
    backoff_ms_ = backoff_ms_ == 0 ? opt_.base_backoff_ms : std::min(backoff_ms_*2, 10000);
    log_("config: load failed ("+std::to_string(failures_)+"), backoff "+std::to_string(backoff_ms_)+"ms");
    next_try_ = now + std::chrono::milliseconds(backoff_ms_);
    return;
  }

  // Compute hash for content change detection using the parsed raw text.
  std::string new_hash = ailee::crypto::sha256_hex(res.raw_text);
  if (new_hash == last_hash_) { next_try_ = now + std::chrono::seconds(1); return; }

  // Apply atomically
  try {
    apply_(*res.cfg);
    last_hash_ = new_hash;
    failures_ = 0;
    backoff_ms_ = 0;
    log_("config: applied successfully");
  } catch (const std::exception& ex) {
    failures_++;
    log_(std::string("config: apply failed: ")+ex.what());
  }
  next_try_ = now + std::chrono::seconds(1);
}

void ConfigReloader::reset() {
  failures_ = 0;
  backoff_ms_ = 0;
  next_try_ = std::chrono::steady_clock::now();
  log_("config: circuit breaker reset — will retry on next tick");
}
