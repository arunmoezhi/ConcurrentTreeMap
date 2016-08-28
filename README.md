# ConcurrentTreeMap
This repository contains implementation of a concurrent tree map. This uses a lock-based internal Binary Search Tree.<br>
The algorithm is described in our paper **CASTLE: Fast Concurrent Internal Binary Search Tree using Edge-Based Locking** published in PPoPP'15<br>
The technical report is available in the papers directory<br>
How to compile?<br>
1. Change the Makefile appropriately and run make<br>
How to run?<br>
`$./bin/concurrentTreeMap.o numOfThreads read% insert% delete% durationInSeconds maximumKeySize initialSeed`<br>
Example: `$./bin/concurrentTreeMap.o 64 70 20 10 1 10000 0`<br>

Member Functions:<br>
* `ConcurrentTreeMap()` &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- constructs a concurrent tree map
* `V lookup(const K key)`&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- returns the value associated with they key if present
* `bool insert(K key, V value)` - inserts a (key, value) pair if key is absent
* `bool remove(K key)`&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- removes a (key, value) pair if key is present
* `bool isValidTree()`&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;- validates if this tree is a valid binary search tree

Optional Libraries:<br>
* GSL to create random numbers (faster than standard PRNG)
* Intel "Threading Building Blocks(TBB)" atomic library
* TBB atomic can be used by replacing std::atomic in Node.h
* Some memory allocator like jemalloc, tcmalloc, tbbmalloc, etc (these are much faster than std malloc)

Any questions?

contact - arunmoezhi at gmail dot com
