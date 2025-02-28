#ifndef COROUTINE_H
#define COROUTINE_H

#include <coroutine>

#include "printf.h"
#include "stdint.h"

template <typename T = void>
struct Task {
  struct promise_type {
    T value_;
    Task get_return_object() { return {}; }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    std::suspend_always yield_value(unsigned value) {
      value_ = value;
      return {};
    }
  };
  std::coroutine_handle<promise_type> h_;
};

struct Awaiter {
  std::coroutine_handle<>* hp_;
  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) { *hp_ = h; }
  constexpr void await_resume() const noexcept {}
};

Task<uint32_t> counter() {
  for (unsigned i = 0;; ++i) {
    co_yield i;
  }
}

void test_coroutine() {
  printf("test_coroutine\n");
  auto h = counter().h_;
  auto& hPromise = h.promise();
  for (int i = 0; i < 15; ++i) {
    uint32_t value = hPromise.value_;
    printf("main1: %d\n", i);
  }
  h.destroy();
}

#endif