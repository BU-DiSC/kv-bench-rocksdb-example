#!/bin/bash

ZD_list=("0" "3")
Z_list=("0.0" "0.1" "0.2" "0.3" "0.4" "0.5" "0.6" "0.7" "0.8" "0.9" "1.0")
for ZD in ${ZD_list[@]}
do
	echo "ZD=${ZD}"
	for Z in ${Z_list[@]}
	do
		str="Z${Z}_ZD${ZD}"
		grep "point query latency:" kvbench_*output.txt | grep ${str} | awk -F':|-' '{print $3}' | awk -F' ' '{print $1}'
	done
done
