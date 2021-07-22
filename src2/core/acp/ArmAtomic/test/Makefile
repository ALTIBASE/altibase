ALL:
	gcc -I./ -o test_swap test_swap.c
	gcc -I./ -o ./test_driver16 ./test_driver16.c ./acpAtomic-ARM64_LINUX.c -lpthread
	gcc -I./ -o ./test_driver32 ./test_driver32.c ./acpAtomic-ARM64_LINUX.c -lpthread
	gcc -I./ -o ./test_driver64 ./test_driver64.c ./acpAtomic-ARM64_LINUX.c -lpthread
clean:
	rm -f test_driver16 test_driver32 test_driver64 test_swap
