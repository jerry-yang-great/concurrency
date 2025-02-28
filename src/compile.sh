#!/bin/bash

g++ thread_pool_test.cpp -o thread_pool_test --std=c++17 -g -O2 -lpthread -I lock_free -I thread_pool
g++ lock_free_test.cpp -o lock_free_test --std=c++17 -g -O2 -lpthread -I lock_free -I thread_pool

