#ifndef EVENTTESTS_H
#define EVENTTESTS_H

#include "event_loop.h"
#include "printf.h"
#include "atomics.h"
#include "testFramework.h"


void eventLoopTests(){
  initTests("Event Loop Tests");

  // Test 1: Basic event scheduling
  int total = 0;
  Atomic<int> A_total = Atomic<int>(&total);
  for (int i = 0; i < 10; i++){
    schedule_event([&] {
      A_total.add_fetch(1);
    });
  }

  while(A_total.load() < 10){
    // Debug::printf("Waiting for events to finish...\n");
  }
  
  testsResult("Basic event scheduling", 10 == A_total.load());
}

#endif