# Allocator - Custom Heap Memory Manager

This project implements a simplified dynamic memory allocator in C++ using a linked list structure to manage heap blocks.

## Features

- `initHeap(size)`: Initializes the heap with a single free block.
- `myMalloc(id, size)`: Allocates memory by splitting free blocks.
- `myFree(id, index)`: Frees memory and merges adjacent free blocks.
- Thread-safe using `pthread_mutex_t` for concurrency.

## Notes

The allocator simulates real-world memory management techniques like splitting and merging, helping understand how heap memory can be managed manually.
