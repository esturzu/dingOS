#ifndef HASHMAP_TESTS_H
#define HASHMAP_TESTS_H

#include "atomics.h"
#include "event_loop.h"
#include "hashmap.h"
#include "printf.h"
#include "stdint.h"
#include "testFramework.h"

void hashmapTests() {
  initTests("HashMap Tests");
  static int constexpr N = 1e5;

  // Test 1: Basic HashMap usage
  // Tests many concurrent lookup and insertion
  debug_printf("Test 1: Basic HashMap usage\n");
  HashMap<int, int>* map = new HashMap<int, int>();
  Atomic<int> basic_usage_count(0);

  schedule_event([=, &basic_usage_count] {
    for (int j = 0; j < N; ++j) {
      schedule_event([=, &basic_usage_count] {
        // Test concurrent lookup/insertion
        map->insert_or_assign(1, 2);
        map->insert_or_assign(3, 4);

        int value1, value3;
        map->find(1, value1);
        map->find(3, value3);

        if (value1 == 2 && value3 == 4) {
          basic_usage_count.add_fetch(1);
        }
      });
    }

    for (int j = 0; j < N; ++j) {
      // Test concurrent lookup/insertion
      map->insert_or_assign(1, 2);
      map->insert_or_assign(3, 4);

      int value1, value3;
      map->find(1, value1);
      map->find(3, value3);

      if (value1 == 2 && value3 == 4) {
        basic_usage_count.add_fetch(1);
      }
    }
  });

  // Spin until all events are done
  while (basic_usage_count.load() < 2 * N);

  // Test 2: HashMap resizing
  debug_printf("Test 2: HashMap resizing\n");
  Atomic<int> resizing_count(0);

  // Insert/erase concurrently
  for (int i = 0; i < N; ++i) {
    schedule_event([=, &resizing_count] {
      map->insert_or_assign(i, i + 1);
      resizing_count.add_fetch(1);
    });
  }
  for (int i = 0; i < N; ++i) {
    schedule_event([=, &resizing_count] {
      map->erase(i);
      resizing_count.add_fetch(1);
    });
  }

  // Insert/erase sequentially
  for (int i = 0; i < N; ++i) {
    map->insert_or_assign(i, i + 1);
  }
  for (int i = 0; i < N; ++i) {
    map->erase(i);
  }

  // Insert/erase the same value
  for (int i = 0; i < N; ++i) {
    schedule_event([=, &resizing_count] {
      map->insert_or_assign(1, i + 1);
      resizing_count.add_fetch(1);
    });
  }
  for (int i = 0; i < N; ++i) {
    schedule_event([=, &resizing_count] {
      map->erase(1);
      resizing_count.add_fetch(1);
    });
  }

  // Erase again
  for (int i = 0; i < N; ++i) {
    schedule_event([=, &resizing_count] {
      map->erase(i);
      resizing_count.add_fetch(1);
    });
  }

  // Spin until all events are done
  while (resizing_count.load() < 5 * N);

  // Test 3: HashMap insert and erase
  // Tests that all inserts/erases are working properly
  debug_printf("Test 3: HashMap inserts/erases\n");
  Atomic<int> insert_erase_count(0);

  for (int i = 0; i < N; ++i) {
    schedule_event([=, &insert_erase_count] {
      map->insert_or_assign(i, i + 1);

      schedule_event([=, &insert_erase_count] {
        bool const erased = map->erase(i);
        if (erased) {
          // It should get here
          insert_erase_count.add_fetch(1);
        }
      });
    });
  }

  // Number of successful erase should be N
  while (insert_erase_count.load() < N);

  // All tests are completed
  debug_printf("HashMap Tests Completed\n");
}

#endif  // HASHMAP_TESTS_H
