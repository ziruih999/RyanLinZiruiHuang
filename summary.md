#### When check memory leak with valgrind, I find the client which receive the '!' termination character will end up with 1,638 bytes in 4 blocks that are still reachable.
#### This is caused by pthread_cancel_init() and I do not know how to figure it out.
