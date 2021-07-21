#!/bin/bash

./test_swap
./test_driver16 32 2000 2> /dev/null
./test_driver32 32 1000000 2> /dev/null
./test_driver64 32 10000000 2> /dev/null
