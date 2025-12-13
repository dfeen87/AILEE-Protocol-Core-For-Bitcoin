// metrics.h
#pragma once
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <functional>

struct SignalData {
  std::string name;
  std::deque<double> window;
};

struct MetricContext {
  size_t stride_ms;
  size_t step_ms;
  std::unordered_map<std::string, SignalData*> signals; // name -> ptr
};

using MetricFn = std::function<double(const MetricContext&, const std::vector<std::string>&)>;

class MetricsRegistry {
public:
  static MetricsRegistry& instance();
  void register_metric(const std::string& name, MetricFn fn);
  MetricFn get(const std::string& name) const;

private:
  std::unordered_map<std::string, MetricFn> map_;
};
