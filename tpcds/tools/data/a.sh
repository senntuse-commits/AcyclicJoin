#!/usr/bin/bash
rm *.dat
cp ../temp/*.dat .
python3 a.py
ls -lh 
cd ../..
python3 run.py -c
python3 run.py -l
python3 run.py -t