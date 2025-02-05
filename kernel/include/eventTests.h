#ifndef EVENT_TESTS_H
#define EVENT_TESTS_H

#include "event_loop.h"
#include "printf.h"


void eventLoopTests(){
  // Test 1: Basic event scheduling
  for (int i = 0; i < 10; i++){
    schedule_event([i] {
      Debug::printf("Event #%d!\n", i);
    });
  }
}

#endif