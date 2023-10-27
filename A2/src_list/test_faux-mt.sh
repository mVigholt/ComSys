#!/usr/bin/env bash

# Exit immediately if any command below fails.
set -e

make fauxgrep-mt
make fauxgrep

tests=5

needle="int"

for i in $(seq 2 $tests) 
do
  cp ../test_files/fhistogram/50000records.tsv ../test_files/fhistogram/50000records_${i}.tsv
done

echo "--------------------------------------------------"
echo "Copying test files, please wait 3 sec.."

sleep 3

exitcode=0

# Testing fhistogram wit job_queue.c
echo "--------------------------------------------------"
echo "Testing 'fauxgrep.c' for refference"
echo "--------------------------------------------------"
echo "time ./fauxgrep ../"
time ./fauxgrep $needle ../ >/dev/null
echo ""

# Testing fhistogram-mt wit job_queue.c
echo "--------------------------------------------------"
echo "Testing 'fauxgrep-mt.c' with up to $tests threads"
echo "--------------------------------------------------"

for i in $(seq 1 $tests) 
do
  echo "Running with $i thread(s).."
  echo "time ./fauxgrep-mt -n ${i} ../"
  time ./fauxgrep-mt -n ${i} $needle ../ >/dev/null
  echo ""
done

for i in $(seq 2 $tests) 
do
  rm ../test_files/fhistogram/50000records_${i}.tsv
done

echo "Tests passed :)"
exit $exitcode
