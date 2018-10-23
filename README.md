#  ConcurrentTreeMap
This repository contains implementation of a concurrent tree map. This uses a lock-based internal Binary Search Tree. The algorithm is described in our paper **CASTLE: Fast Concurrent Internal Binary Search Tree using Edge-Based Locking** published in PPoPP'15. The technical report is available in the papers directory.

## How to compile?
1. Change the Makefile appropriately and run make from your shell to build.

## How to run?

`$ ./bin/ConcurrentTreeMap.o numOfThreads read% insert% delete% durationInSeconds maximumKeySize initialSeed`

### Example: `$ ./bin/ConcurrentTreeMap.o 64 70 20 10 1 10000 0`

## Member Functions:
* `ConcurrentTreeMap()` - constructs a concurrent tree map
* `V lookup(const K key)` - returns the value associated with they key if present
* `bool insert(K key, V value)` - inserts a (key, value) pair if key is absent
* `bool remove(K key)` - removes a (key, value) pair if key is present
* `unsigned long size()` - returns the size of the tree
* `bool isValidTree()` - validates if this tree is a valid binary search tree

## Optional Libraries:
* GSL to create random numbers (faster than standard PRNG)
* Intel "Threading Building Blocks(TBB)" atomic library
* TBB atomic can be used by replacing std::atomic in `Node.h`
* Some memory allocator like jemalloc, tcmalloc, tbbmalloc, etc. These libraries can improve performance since they are much faster than std malloc.

## Limitations:
* Not a full blown templated library like `std::map` or `std::unordered_map`
* Supports only numeric types as keys

## To do:
* Add support for concurrent iterator which returns the elements in sorted order (thinking of a global lock for now)

## questions/comments/bugs?

contact - arunmoezhi at gmail dot com
