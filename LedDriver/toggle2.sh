#!/bin/bash
for i in 1 2 3 4 5;
do
	./leds set caps on;
	sleep 0.5;
	./leds set caps off;
	sleep 0.5;
done
