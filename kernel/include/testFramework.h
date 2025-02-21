#ifndef TESTFRAMEWORK_H
#define TESTFRAMEWORK_H
#include "printf.h"

const char* testsName;
int count;

void initTests(const char* testSuiteName) {
  testsName = testSuiteName;
  count = 1;

  printf("Starting %s\n", testsName);
}

void testsResult(const char* testName, bool success) {
  if (success) {
    printf(" %s %d Passed: %s\n", testsName, count, testName);
  } else {
    printf(" ***%s %d Failed: %s\n", testsName, count, testName);
  }
  count++;
}

#endif