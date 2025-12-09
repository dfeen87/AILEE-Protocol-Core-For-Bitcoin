// expr.h
#pragma once
#include <string>
#include <unordered_map>
#include <optional>

struct EvalContext {
  const std::unordered_map<std::string, double>* vars;
};

std::optional<bool> eval_bool_expr(const std::string& expr, const EvalContext& ctx, std::string& error);
