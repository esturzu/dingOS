#ifndef EVENTTESTS_H
#define EVENTTESTS_H

#include "event_loop.h"
#include "printf.h"
#include "atomics.h"
#include "testFramework.h"


void eventLoopTests(){
  initTests("Event Loop Tests");

  // Test 1: Basic event scheduling
  Atomic<int> total = Atomic<int>(0);
  for (int i = 0; i < 10; i++){
    schedule_event([&] {
      total.add_fetch(1);
    });
  }

  while(total.load() < 10){
    // Debug::printf("Waiting for events to finish...\n");
  }
  
  testsResult("Basic event scheduling", 10 == total.load());
}

#endif