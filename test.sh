#!/bin/bash

pkill ringmaster
pkill player
num_players=$1
num_hops=$2
rport=51015
mkdir -p output
echo "players: $num_players, hops: $num_hops"
make
rm output/* -f
./ringmaster $rport $num_players $num_hops >output/master.txt &
for i in $(seq 1 $num_players)
do
    ./player localhost $rport >output/player$i.txt &
done

wait

