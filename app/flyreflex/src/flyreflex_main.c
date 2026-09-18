/* SPDX-License-Identifier: Apache-2.0 */

#include "flyreflex.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *program)
{
  printf("FlyReflex - connectome-traceable safety reflex layer\n"
         "Usage:\n"
         "  %s demo [safe|slow|danger|recovery|noise|all] [--csv]\n"
         "  %s bench [iterations]\n"
#ifdef __NuttX__
         "  %s ui\n"
#endif
         "\nAI command is a SIMULATED local FORWARD command.\n",
         program, program
#ifdef __NuttX__
         , program
#endif
         );
}

static int scenario_from_name(const char *name,
                              enum flyreflex_scenario_e *scenario)
{
  const char *const names[] = {"safe", "slow", "danger", "recovery", "noise"};
  size_t index;

  for (index = 0; index < sizeof(names) / sizeof(names[0]); index++)
    {
      if (strcmp(name, names[index]) == 0)
        {
          *scenario = (enum flyreflex_scenario_e)index;
          return 0;
        }
    }

  return -1;
}

static int run_demo(int argc, char **argv)
{
  enum flyreflex_scenario_e scenario;
  bool csv = false;
  const char *name = "all";
  int index;

  for (index = 2; index < argc; index++)
    {
      if (strcmp(argv[index], "--csv") == 0)
        {
          csv = true;
        }
      else
        {
          name = argv[index];
        }
    }

  if (strcmp(name, "all") == 0)
    {
      for (scenario = FLYREFLEX_SCENARIO_SAFE;
           scenario <= FLYREFLEX_SCENARIO_RECOVERY;
           scenario = (enum flyreflex_scenario_e)(scenario + 1))
        {
          if (flyreflex_run_scenario(scenario, csv, NULL, 0, NULL) != 0)
            {
              return 1;
            }
        }

      return 0;
    }

  if (scenario_from_name(name, &scenario) != 0)
    {
      fprintf(stderr, "Unknown scenario: %s\n", name);
      return 1;
    }

  return flyreflex_run_scenario(scenario, csv, NULL, 0, NULL) == 0 ? 0 : 1;
}

static int run_benchmark(int argc, char **argv)
{
  struct flyreflex_stats_s reflex;
  struct flyreflex_stats_s end_to_end;
  char *end = NULL;
  unsigned long parsed;
  size_t iterations = 1000;

  if (argc >= 3)
    {
      errno = 0;
      parsed = strtoul(argv[2], &end, 10);
      if (errno != 0 || end == argv[2] || *end != '\0' || parsed == 0 ||
          parsed > 1000000UL)
        {
          fprintf(stderr, "iterations must be in [1, 1000000]\n");
          return 1;
        }

      iterations = (size_t)parsed;
    }

  if (flyreflex_run_benchmark(iterations, &reflex, &end_to_end) != 0)
    {
      fprintf(stderr, "benchmark failed\n");
      return 1;
    }

  printf("FlyReflex benchmark (measured locally with CLOCK_MONOTONIC)\n"
         "environment=%s iterations=%zu units=ns\n",
#ifdef __NuttX__
         "openvela simulator/target",
#else
         "host reference build (not openvela)",
#endif
         iterations);
  printf("path,min,mean,median,p95,p99,max\n");
  printf("reflex_compute,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
         ",%" PRIu64 ",%" PRIu64 "\n",
         reflex.minimum_ns, reflex.mean_ns, reflex.median_ns, reflex.p95_ns,
         reflex.p99_ns, reflex.maximum_ns);
  printf("end_to_end,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64
         ",%" PRIu64 ",%" PRIu64 "\n",
         end_to_end.minimum_ns, end_to_end.mean_ns, end_to_end.median_ns,
         end_to_end.p95_ns, end_to_end.p99_ns, end_to_end.maximum_ns);
  return 0;
}

int main(int argc, char **argv)
{
  if (argc < 2)
    {
      print_usage(argv[0]);
      return 0;
    }

  if (strcmp(argv[1], "demo") == 0)
    {
      return run_demo(argc, argv);
    }

  if (strcmp(argv[1], "bench") == 0)
    {
      return run_benchmark(argc, argv);
    }

#ifdef __NuttX__
  if (strcmp(argv[1], "ui") == 0)
    {
      return flyreflex_ui_run();
    }
#endif

  print_usage(argv[0]);
  return 1;
}

