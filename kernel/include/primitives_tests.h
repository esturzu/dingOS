#include "semaphore.h"
#include "barrier.h"
#include "future.h"
#include "printf.h"
#include "testFramework.h"
#include "atomics.h"
#include "event_loop.h"

void primitives_tests() {
    initTests("Synchronization Primitive Tests");

    Future<int>* f = new Future<int>();

    Atomic<int>* total = new Atomic<int>(0);
    for (int i = 0; i < 10; i++) {
        schedule_event([&f, &total] { 
            int x = f->get(); 
            total->add_fetch(x);
        });
    }

    f->set(1);

    while (total->load() < 10) {
        // debug_printf("Waiting for events to finish...\n");
    }

    testsResult("Future Testing", 10 == total->load());
    delete f;

    f = new Future<int>();
    Barrier* b = new Barrier(11);

    for (int i = 0; i < 10; i++) {
        schedule_event([b, total] { 
            total->add_fetch(-1);
            b->sync();
        });
    }


    // for (int i = 0; i < 5; i++) {
    //     printf("here\n");
    //     schedule_event([b, &total] { 
    //         b->sync();
    //         total.add_fetch(-1);
    //     });
    // }

   
    while (total->load() != 0) {
        // debug_printf("Waiting for events to finish...\n");
    }

    b->sync();

    testsResult("Barrier Testing", true);
}
