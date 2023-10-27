#!/usr/bin/env bash

# Exit immediately if any command below fails.
set -e

make fhistogram-mt

echo "Running the tests.."
exitcode=0

# Testing fhistogram-mt wit job_queue.c
echo "Testing 'fhistogram-mt.c' with up to 5 threads"
for f in test_files/fhistogram/*.tsv
do
  echo "--------------------"
  echo "testing on file input: ${f}"
  for i in 1 2 3 4 5
  do
    echo "Running with $i thread(s).."
    echo "time ./fhistogram-mt -n ${i} ${f}"
    time ./fhistogram-mt -n ${i} ${f}
    echo ""
  done
done

echo "Tests passed :)"
exit $exitcode
