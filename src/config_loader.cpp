// config_loader.cpp
#include "config_loader.h"
#include <fstream>
#include <sstream>

static std::string slurp(const std::string& file) {
  std::ifstream in(file, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss; ss << in.rdbuf();
  return ss.str();
}

// Stub parse functions; wire to your chosen lib (yaml-cpp, nlohmann/json, toml++)
static std::optional<Config> parse_yaml(const std::string& text);
static std::optional<Config> parse_json(const std::string& text);
static std::optional<Config> parse_toml(const std::string& text);

ConfigResult load_config(const std::string& file, ConfigFormat fmt) {
  ConfigResult r;
  auto text = slurp(file);
  if (text.empty()) {
    r.errors.push_back({"Config file not found or empty", file});
    return r;
  }
  std::optional<Config> parsed;
  switch (fmt) {
    case ConfigFormat::YAML: parsed = parse_yaml(text); break;
    case ConfigFormat::JSON: parsed = parse_json(text); break;
    case ConfigFormat::TOML: parsed = parse_toml(text); break;
  }
  if (!parsed) {
    r.errors.push_back({"Parse failed", file});
    return r;
  }
  std::vector<ConfigError> errs;
  if (!validate(*parsed, errs)) {
    r.errors = std::move(errs);
    return r;
  }
  r.cfg = std::move(parsed);
  return r;
}

bool validate(const Config& cfg, std::vector<ConfigError>& e) {
  auto add = [&](const std::string& m, const std::string& p) { e.push_back({m,p}); };
  if (cfg.mode != "simulation" && cfg.mode != "live") add("mode must be 'simulation' or 'live'", "mode");
  if (cfg.step_ms < 5 || cfg.step_ms > 1000) add("step_ms out of bounds [5..1000]", "step_ms");
  if (cfg.horizon_s < 10 || cfg.horizon_s > 86400) add("horizon_s out of bounds [10..86400]", "horizon_s");

  if (cfg.signals.empty()) add("at least one signal required", "signals");
  for (size_t i=0;i<cfg.signals.size();++i) {
    const auto& s = cfg.signals[i];
    if (s.name.empty()) add("signal.name required", "signals["+std::to_string(i)+"].name");
    if (s.source.empty()) add("signal.source required", "signals["+std::to_string(i)+"].source");
    if (s.window_ms < cfg.step_ms) add("signal.window_ms must be >= step_ms", "signals["+std::to_string(i)+"].window_ms");
  }

  for (size_t i=0;i<cfg.metrics.size();++i) {
    const auto& m = cfg.metrics[i];
    if (m.name.empty()) add("metric.name required", "metrics["+std::to_string(i)+"]");
    if (m.window_ms < cfg.step_ms) add("metric.window_ms >= step_ms", "metrics["+std::to_string(i)+"].window_ms");
    if (m.stride_ms < cfg.step_ms) add("metric.stride_ms >= step_ms", "metrics["+std::to_string(i)+"].stride_ms");
    if (m.signals.size() < 2) add("metric must reference >=2 signals", "metrics["+std::to_string(i)+"].signals");
  }

  for (size_t i=0;i<cfg.policies.size();++i) {
    const auto& p = cfg.policies[i];
    if (p.name.empty()) add("policy.name required", "policies["+std::to_string(i)+"]");
    if (p.when.empty()) add("policy.when expression required", "policies["+std::to_string(i)+"].when");
    if (p.actions.empty()) add("policy must have actions", "policies["+std::to_string(i)+"].actions");
  }

  for (size_t i=0;i<cfg.outputs.size();++i) {
    const auto& o = cfg.outputs[i];
    if (o.type != "csv") add("outputs.type currently supports 'csv' only", "outputs["+std::to_string(i)+"].type");
    if (o.path.empty()) add("outputs.path required", "outputs["+std::to_string(i)+"].path");
  }
  return e.empty();
}
