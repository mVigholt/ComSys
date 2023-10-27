#!/usr/bin/env bash

# Exit immediately if any command below fails.
set -e

make fibs

echo "Running the tests.."
exitcode=0

for f in test_files/fibs/*.input
do
  for i in 1 2 3 4 5
  do
    echo "Computing Fibonacci numbers with $i thread(s).."
    echo "time ./fibs -n ${i} > /dev/null < ${f}"
    time ./fibs -n ${i} > /dev/null < ${f} 
    echo ""
  done
done

exit $exitcode
