#!/bin/bash

pkill ringmaster
pkill player
num_players=$1
num_hops=$2
mkdir -p output
echo "players: $num_players, hops: $num_hops"
make
rm output/* -f
./ringmaster 6000 $num_players $num_hops >output/master.txt &
for i in $(seq 1 $num_players)
do
    ./player localhost 6000 >output/player$i.txt &
done

wait

