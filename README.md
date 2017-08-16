# Yet another mutex shootout

A quick benchmark that measures the performance of different mutex
implementations when synchronizing access to an LRU cache. The goal is
to answer 2 questions:

1. How much overhead does locking introduce when protecting a
semi-realistic data structure under no contention?

2. What kind of mutex performs best under heavy contention? (Fairness
is not a concern.)

The LRU cache is implemented using Boost.Bimap, using a UUID as a key
(hashed with MurmurHash3) to retrieve a 2KB buffer under a
course-grained lock.

Unless otherwise noted in third-party libraries (specifically Boost
and MurmurHash3), code for this benchmark is in the public domain.
