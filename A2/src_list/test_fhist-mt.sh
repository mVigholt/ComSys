#!/usr/bin/env bash

# Exit immediately if any command below fails.
set -e

make fhistogram-mt
make fhistogram

tests=5

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
echo "Testing 'fhistogram.c' for refference"
echo "--------------------------------------------------"
echo "time ./fhistogram ../"
time ./fhistogram ../
echo ""

# Testing fhistogram-mt wit job_queue.c
echo "--------------------------------------------------"
echo "Testing 'fhistogram-mt.c' with up to $tests threads"
echo "--------------------------------------------------"

for i in $(seq 1 $tests) 
do
  echo "Running with $i thread(s).."
  echo "time ./fhistogram-mt -n ${i} ../"
  time ./fhistogram-mt -n ${i} ../
  echo ""
done

for i in $(seq 2 $tests) 
do
  rm ../test_files/fhistogram/50000records_${i}.tsv
done

echo "Tests passed :)"
exit $exitcode
