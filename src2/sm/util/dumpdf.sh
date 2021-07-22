#!/bin/sh

echo ./dumpci
./dumpci
echo

echo ./dumpci -j membase -f mydb-0-0
./dumpci -j membase -f mydb-0-0
echo

echo ./dumpci -j catalog -f mydb-0-0
./dumpci -j catalog -f mydb-0-0
echo

echo ./dumpci -j table-header -f mydb-0-0 -o 57224 
./dumpci -j table-header -f mydb-0-0 -o 57224 
echo

echo ./dumpci -j table-page-list -f mydb-0-0 -o 57224 
./dumpci -j table-page-list -f mydb-0-0 -o 57224 
echo

echo ./dumpci -j page-header -f mydb-0-0 -p 44 -l 150
./dumpci -j page-header -f mydb-0-0 -p 44 -l 150
echo

echo ./dumpci -j page-body -f mydb-0-0 -p 44 -l 60 -s 8 
./dumpci -j page-body -f mydb-0-0 -p 44 -l 60 -s 8 
echo

echo ./dumpci -j page-body -f mydb-0-0 -p 45 -l 60 -s 18 -d 0
./dumpci -j page-body -f mydb-0-0 -p 45 -l 60 -s 18 -d 0
echo
echo ./dumpci -j page-body -f mydb-0-0 -p 45 -l 60 -s 18 -d 1 
./dumpci -j page-body -f mydb-0-0 -p 45 -l 60 -s 18 -d 1 
echo
echo ./dumpci -j page-body -f mydb-0-0 -p 45 -l 60 -s 18 -d 2 
./dumpci -j page-body -f mydb-0-0 -p 45 -l 60 -s 18 -d 2 
echo
