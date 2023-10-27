#!/usr/bin/env bash

# Exit immediately if any command below fails.
set -e

make fibs

echo "Running the tests.."
exitcode=0

# Testing list job queue
echo "Testing 'job_queue.c with fibs.c"
for f in test_files/fibs/*.input
do
  echo "--------------------"
  echo "testing on file input: ${f}"
  for i in 1 2 3 4 5
  do
    echo "Computing Fibonacci numbers with $i thread(s).."
    echo "time ./fibs -n ${i} > /dev/null < ${f}"
    time ./fibs -n ${i} > /dev/null < ${f}
    echo ""
  done
done

echo "Tests passed :)"
exit $exitcode