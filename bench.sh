#! /usr/bin/env bash

mk_random () {
    size=$1
    cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $size | head -n 1
}

repeat=$1

echo '# Creating messages...'

for i in 10 100 500 1000 2000 5000 10000 50000 100000 200000 500000 1000000 1500000 2000000; do
	mk_random $i > message_$i;
done

echo '# Sending messages...'

echo -n 'Run # '
for m in `ls message_*`; do
	bytes=$(echo $m | cut -d'_' -f2)
	echo -n $bytes 'B '
done
echo

for i in `seq 1 $repeat`; do
	echo -n $i ' '
	for m in `ls message_*`; do
		./sender archipelago lz4 $m;
	done;
	echo
done
