// policies.h
#pragma once
#include "config_types.h"
#include "expr.h"
#include <unordered_map>
#include <chrono>
#include <functional>

struct ActionFns {
  std::function<void(int delta_ms)> adjust_propagation_delay;
  std::function<void(const std::string& route)> switch_route;
};

class PolicyRunner {
public:
  PolicyRunner(const std::vector<PolicySpec>& specs, ActionFns fns, std::function<void(const std::string&)> log);
  void step(const std::unordered_map<std::string,double>& vars);

private:
  struct State {
    std::chrono::steady_clock::time_point next_ok;
    int oscillations = 0;
  };
  std::vector<PolicySpec> specs_;
  std::unordered_map<std::string, State> states_;
  ActionFns fns_;
  std::function<void(const std::string&)> log_;
  const std::chrono::milliseconds cooldown_{500}; // rate limit
  const int oscillation_limit_ = 10;
};
