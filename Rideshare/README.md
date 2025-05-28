# RideShare Synchronization Simulation

A multi-threaded C simulation where fans from Team A and Team B wait to board cars according to specific rules using semaphores and barriers.

## Rules

- A car can carry 4 fans:
  - Either 4 from the same team
  - Or 2 from each team
- One thread becomes the driver (captain), others are passengers.

## Features

- Synchronization with `pthread_barrier_t` and `sem_t`
- Mutual exclusion with `pthread_mutex_t`
- Command-line arguments to specify number of fans from each team

## Notes

This project models real-world synchronization problems using concurrency primitives.
