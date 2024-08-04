#pragma once

namespace b {

namespace utils {

template <typename F> struct Defer {
  Defer(F f) : f(f) {}

  Defer(const Defer &) = delete;
  Defer(Defer &&) = delete;

  Defer &operator=(const Defer &) = delete;
  Defer &operator=(Defer &&) = delete;

  ~Defer() { f(); }

  F f;
};

template <typename F> Defer<F> defer(F f) { return Defer<F>(f); }

} // namespace utils

} // namespace b