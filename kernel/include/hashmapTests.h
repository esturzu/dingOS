#ifndef HASHMAP_TESTS_H
#define HASHMAP_TESTS_H

#include "event_loop.h"
#include "hashmap.h"
#include "printf.h"
#include "stdint.h"
#include "testFramework.h"

void hashmapTests() {
  initTests("Hashmap Tests");

  // Test 1: Basic hashmap usage
  // Tests insertion and lookup
  HashMap<int, int>* map = new HashMap<int, int>();
  dPrintf("FINISHED MAKING HASHMAP!\n");
  schedule_event([=] {
    dPrintf("HERE 1\n");
    map->insert_or_assign(1, 2);
    dPrintf("HERE 2\n");
    map->insert_or_assign(3, 4);
    dPrintf("HERE 3\n");

    schedule_event([=] {
      dPrintf("HERE INNER\n");
      int value;
      map->find(3, value);
      testsResult("Basic hashmap usage", value == 4);
    });

    dPrintf("HERE 4\n");
  });

  // Test 2: Resizing
}

#endif  // HASHMAP_TESTS_H
