/**
 * File: pipeline-test.c
 * ---------------------
 * Exercises the pipeline function to verify 
 * basic functionality.
 */

#include "pipeline.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

static void printArgumentVector(char *argv[]) {
  if (argv == NULL || *argv == NULL) {
    printf("<empty>");
    return;
  }
  
  while (true) {
    printf("%s", *argv);
    argv++;
    if (*argv == NULL) return;
    printf(" ");
  }
}

static void summarizePipeline(char *argv1[], char *argv2[]) {
  printf("Pipeline: ");
  printArgumentVector(argv1);
  printf(" -> ");
  printArgumentVector(argv2);
  printf("\n");
}

static void launchPipedExecutables(char *argv1[], char *argv2[]) {
  summarizePipeline(argv1, argv2);
  pid_t pids[2];
  pipeline(argv1, argv2, pids);
  waitpid(pids[0], NULL, 0);
  waitpid(pids[1], NULL, 0);
}

static void simpleTest() {
  char *argv1[] = {"cat", "/usr/include/tar.h", NULL};
  char *argv2[] = {"wc", NULL};
  launchPipedExecutables(argv1, argv2);
}

static void testCase2() {
  // should take around 10 seconds
  char *argv1[] = {"sleep", "10", NULL};
  char *argv2[] = {"sleep", "10", NULL};
  launchPipedExecutables(argv1, argv2);
}

static void testCase3() {
  char *argv1[] = {"ls", "-al", NULL};
  char *argv2[] = {"head", "-n", "3", NULL};
  launchPipedExecutables(argv1, argv2);
}

int main(int argc, char *argv[]) {
  simpleTest();
  testCase2();
  testCase3();
  return 0;
}
