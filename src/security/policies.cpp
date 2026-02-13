// policies.cpp
#include "policies.h"

PolicyRunner::PolicyRunner(const std::vector<PolicySpec>& specs, ActionFns fns, std::function<void(const std::string&)> log)
  : specs_(specs), fns_(fns), log_(log) {}

void PolicyRunner::step(const std::unordered_map<std::string,double>& vars) {
  auto now = std::chrono::steady_clock::now();
  for (const auto& p : specs_) {
    auto& st = states_[p.name];
    if (now < st.next_ok) continue;

    std::string err;
    auto r = eval_bool_expr(p.when, EvalContext{&vars}, err);
    if (!r.has_value()) { log_("policy '"+p.name+"' invalid: "+err); continue; }
    if (!*r) continue;

    // Execute actions with idempotency guards
    for (const auto& a : p.actions) {
      try {
        if (a.type == "adjust_propagation_delay") {
          int delta = std::stoi(a.args.at("delta_ms"));
          fns_.adjust_propagation_delay(delta);
        } else if (a.type == "switch_route") {
          fns_.switch_route(a.args.at("route"));
        } else {
          log_("policy '"+p.name+"' unknown action: "+a.type);
        }
      } catch (const std::out_of_range&) {
        log_("policy '"+p.name+"' missing required argument for action: "+a.type);
      } catch (const std::invalid_argument&) {
        log_("policy '"+p.name+"' invalid argument value for action: "+a.type);
      }
    }

    // Cooldown to avoid thrash
    st.next_ok = now + cooldown_;
    st.oscillations++;
    if (st.oscillations > oscillation_limit_) {
      log_("policy '"+p.name+"' circuit breaker tripped (oscillation)");
      st.next_ok = now + std::chrono::hours(1);
    }
  }
}
