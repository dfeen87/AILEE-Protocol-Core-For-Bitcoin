// config_types.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

struct SignalSpec {
  std::string name;
  std::string source;
  size_t window_ms;
};

struct MetricSpec {
  std::string name;
  std::string type; // "correlation_average", "ewma", etc.
  std::vector<std::string> signals;
  size_t window_ms;
  size_t stride_ms;
};

struct PolicyAction {
  std::string type; // "adjust_propagation_delay", "switch_route"
  std::unordered_map<std::string, std::string> args; // delta_ms, route, etc.
};

struct PolicySpec {
  std::string name;
  std::string when; // expression string
  std::vector<PolicyAction> actions;
};

struct PipelineSpec {
  std::string name;
  bool enabled;
};

struct OutputSpec {
  std::string type; // "csv"
  std::string path;
  std::vector<std::string> fields;
};

struct Config {
  int version = 1;
  std::string mode; // "simulation" | "live"
  size_t step_ms = 50;
  size_t horizon_s = 600;

  std::vector<SignalSpec> signals;
  std::vector<MetricSpec> metrics;
  std::vector<PolicySpec> policies;
  std::vector<PipelineSpec> pipelines;
  std::vector<OutputSpec> outputs;
};
