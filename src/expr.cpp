// expr.cpp
#include "expr.h"
#include <cmath>
#include <cctype>
#include <vector>
#include <string>

struct Token { std::string t; };

static std::vector<Token> lex(const std::string& s){
  std::vector<Token> out; std::string buf;
  auto flush=[&]{ if(!buf.empty()){ out.push_back({buf}); buf.clear(); } };
  for(size_t i=0;i<s.size();++i){
    char c=s[i];
    if (std::isspace(c)) { flush(); continue; }
    if (std::string("()&|<>=!").find(c)!=std::string::npos) {
      flush();
      // handle two-char operators
      if (i+1<s.size() && ((c=='&'&&s[i+1]=='&')||(c=='|'&&s[i+1]=='|')||(c=='='&&s[i+1]=='=')||(c=='!'&&s[i+1]=='=')||(c=='<'&&s[i+1]=='=')||(c=='>'&&s[i+1]=='='))) {
        out.push_back({s.substr(i,2)}); ++i;
      } else {
        out.push_back({std::string(1,c)});
      }
    } else {
      buf.push_back(c);
    }
  }
  flush();
  return out;
}

// Minimal recursive descent for (expr) || expr && comparisons
// For brevity, we implement a simple precedence: parens > comparisons > && > ||
static double to_number(const std::string& s){ try { return std::stod(s); } catch (...) { return NAN; } }
static bool is_number(const std::string& s){ std::string tmp=s; return !std::isnan(to_number(tmp)); }

static std::optional<bool> cmp(const std::string& op, double a, double b){
  if (std::isnan(a) || std::isnan(b)) return std::nullopt;
  if (op=="<") return a<b;
  if (op=="<=") return a<=b;
  if (op==">") return a>b;
  if (op==">=") return a>=b;
  if (op=="==") return a==b;
  if (op=="!=") return a!=b;
  return std::nullopt;
}

static std::optional<bool> parse_or(const std::vector<Token>& toks, size_t& i, const EvalContext& ctx);

static std::optional<bool> parse_comparison(const std::vector<Token>& toks, size_t& i, const EvalContext& ctx){
  if (i>=toks.size()) return std::nullopt;
  std::string a = toks[i++].t;
  double av = is_number(a) ? to_number(a) : (ctx.vars->count(a)? ctx.vars->at(a) : NAN);
  if (i>=toks.size()) return std::nullopt;
  std::string op = toks[i++].t;
  if (i>=toks.size()) return std::nullopt;
  std::string b = toks[i++].t;
  double bv = is_number(b) ? to_number(b) : (ctx.vars->count(b)? ctx.vars->at(b) : NAN);
  return cmp(op,av,bv);
}

static std::optional<bool> parse_atom(const std::vector<Token>& toks, size_t& i, const EvalContext& ctx){
  if (i<toks.size() && toks[i].t=="(") {
    ++i;
    auto r = parse_or(toks,i,ctx);
    if (!r || i>=toks.size() || toks[i].t!=")") return std::nullopt;
    ++i; return r;
  }
  return parse_comparison(toks,i,ctx);
}

static std::optional<bool> parse_and(const std::vector<Token>& toks, size_t& i, const EvalContext& ctx){
  auto left = parse_atom(toks,i,ctx); if (!left) return std::nullopt;
  while (i<toks.size() && toks[i].t=="&&") { ++i; auto right = parse_atom(toks,i,ctx); if (!right) return std::nullopt; *left = (*left && *right); }
  return left;
}

static std::optional<bool> parse_or(const std::vector<Token>& toks, size_t& i, const EvalContext& ctx){
  auto left = parse_and(toks,i,ctx); if (!left) return std::nullopt;
  while (i<toks.size() && toks[i].t=="||") { ++i; auto right = parse_and(toks,i,ctx); if (!right) return std::nullopt; *left = (*left || *right); }
  return left;
}

std::optional<bool> eval_bool_expr(const std::string& expr, const EvalContext& ctx, std::string& error){
  auto toks = lex(expr); size_t i=0;
  auto r = parse_or(toks,i,ctx);
  if (!r || i!=toks.size()) { error = "invalid expression: "+expr; return std::nullopt; }
  return r;
}
