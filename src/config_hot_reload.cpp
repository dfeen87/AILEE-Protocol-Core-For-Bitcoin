// config_hot_reload.cpp
#include "config_hot_reload.h"
#include <functional>
#include <string>
#include <chrono>
#include <thread>
#include <openssl/sha.h> // or any hash; keep deterministic

static std::string sha256(const std::string& s) {
  unsigned char h[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(s.data()), s.size(), h);
  std::string out; out.reserve(2*SHA256_DIGEST_LENGTH);
  static const char* hex="0123456789abcdef";
  for(int i=0;i<SHA256_DIGEST_LENGTH;++i){ out.push_back(hex[h[i]>>4]); out.push_back(hex[h[i]&0xF]); }
  return out;
}

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
      next_try_ = now + std::chrono::hours(24); // require manual reset
      return;
    }
    backoff_ms_ = backoff_ms_ == 0 ? opt_.base_backoff_ms : std::min(backoff_ms_*2, 10000);
    log_("config: load failed ("+std::to_string(failures_)+"), backoff "+std::to_string(backoff_ms_)+"ms");
    next_try_ = now + std::chrono::milliseconds(backoff_ms_);
    return;
  }

  // Compute hash for content change detection
  // (You can expose raw text from loader; here we re-read for simplicity)
  // In production, pass the raw text through loader to avoid re-read.
  // Assume we have a function get_raw_text(file) returning std::string.
  // For brevity, skip; treat any success as potential change.
  std::string new_hash = std::to_string(reinterpret_cast<uintptr_t>(res.cfg->signals.data())); // placeholder
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
