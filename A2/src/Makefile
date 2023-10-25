CC=gcc
CFLAGS=-g -Wall -Wextra -pedantic -std=gnu99 -pthread
EXAMPLES=fibs fauxgrep fauxgrep-mt fhistogram fhistogram-mt fibs_array

.PHONY: all test clean ../src.zip

all: $(TESTS) $(EXAMPLES)

job_queue.o: job_queue.c job_queue.h
	$(CC) -c job_queue.c $(CFLAGS)

%: %.c job_queue.o
	$(CC) -o $@ $^ $(CFLAGS)

#####
job_queue.o: job_queue_array.c job_queue_array.h
	$(CC) -c job_queue_array.c $(CFLAGS)

%: %.c job_queue_array.o
	$(CC) -o $@ $^ $(CFLAGS)
#####

test: $(TESTS)
	@set e; for test in $(TESTS); do echo ./$$test; ./$$test; done

clean:
	rm -rf $(TESTS) $(EXAMPLES) *.o core

../src.zip:
	make clean
	cd .. && zip src.zip -r src
