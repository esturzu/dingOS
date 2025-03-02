#ifndef COROUTINE_H
#define COROUTINE_H

#include <coroutine>

#include "printf.h"
#include "stdint.h"

#define await \
  co_await EventAwaiter {}

template <typename T>
struct Task {
  struct promise_type {
    T value_;

    ~promise_type() { debug_printf("Destroying promise type\n"); }

    Task get_return_object() {
      return {.h_ = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}

    std::suspend_always yield_value(T value) {
      value_ = value;
      return {};
    }

    void return_value(T value) { value_ = value; }

    T get() { return value_; }
  };
  std::coroutine_handle<promise_type> h_;

  T get() {
    if (!h_.done()) h_.resume();
    return h_.promise().get();
  }

  T wait() {
    while (!h_.done()) {
      await;
    }
    return h_.promise().get();
  }

  void destroy() { h_.destroy(); }
};

template <>
struct Task<void> {
  struct promise_type {
    ~promise_type() { debug_printf("Destroying promise type"); }

    Task get_return_object() {
      return {.h_ = std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {}
    std::suspend_always yield_value(unsigned value) { return {}; }
  };
  std::coroutine_handle<promise_type> h_;

  void wait() {
    while (!h_.done()) {
      h_.resume();
    }
    return;
  }
  void destroy() { h_.destroy(); }
};

struct EventAwaiter {
  bool scheduled_ = false;
  bool await_ready() noexcept { return false; }
  bool await_suspend(std::coroutine_handle<> handle) noexcept {
    debug_printf("Suspending event\n");
    if (!scheduled_) {
      scheduled_ = true;

      schedule_event([handle, this]() {
        scheduled_ = false;
        debug_printf("Resuming event\n");
        handle.resume();
      });
    }

    return true;
  }
  void await_resume() noexcept {};
};

Task<uint32_t> counter(uint32_t max) {
  for (uint8_t i = 0; i < max; ++i) {
    co_yield i;
  }
}

Task<void> test_counter() {
  printf("Testing counter coroutine\n");
  auto h = counter(10);
  for (int i = 0; i < 15; i++) {
    uint32_t value = h.get();
    printf("Counter: %u\n", value);
  }
  h.destroy();
}

Task<void> printer() {
  for (uint8_t i = 0; i < 10; i++) {
    printf("Printer: %u\n", i);
    await;
  }
}

Task<void> test_printer() {
  printf("Testing counter coroutine\n");
  printer().wait();
}

Task<uint32_t> fib(uint32_t index) {
  uint32_t last = 1;
  uint32_t curr = 1;
  for (int i = 0; i < index; i++) {
    curr += last;
    last = curr - last;
    await;
  }

  co_return curr;
}
void test_fib() {
  printf("Testing fib coroutine\n");
  uint32_t val = fib(10).wait();
  printf("Fib: %d\n", val);
}

void test_coroutine() {
  printf("test_coroutines\n");

  test_counter();
  test_printer();
  test_fib();
}

#endif