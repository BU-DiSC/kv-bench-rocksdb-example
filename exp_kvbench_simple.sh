#!/bin/bash

P=4096
B=8
T=4
E=1024
R=1
BCC=16384
DB_HOME="/scratchNVM1/zczhu/db_working_home"
S="1000"
Y="0.5"

RQ_THRESHOLD_LIST=("0.0" "0.2" "0.4" "0.6"  "0.8"  "1.0")

mkdir -p "output/"
for RQ_THRESHOLD in ${RQ_THRESHOLD_LIST[@]}
do
	echo "./simple_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --qwp ../K-V-Workload-Generator/kvbench_S${S}_RQ_THRESHOLD${RQ_THRESHOLD}_Y${Y}_mixed_workload.txt -B ${B} -P 4096 --BCC ${BCC} -V 1 --dw --dr > output/kvbench_S${S}_RQ_THRESHOLD${RQ_THRESHOLD}_Y${Y}_output.txt -R 3"
	./simple_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --qwp ../K-V-Workload-Generator/kvbench_S${S}_RQ_THRESHOLD${RQ_THRESHOLD}_Y${Y}_mixed_workload.txt -B ${B} -P 4096 --BCC ${BCC} -R ${R} -V 1 --dw --dr > output/kvbench_S${S}_RQ_THRESHOLD${RQ_THRESHOLD}_Y${Y}_output.txt -R 3
	rm ${DB_HOME}/*
done
cd output/
./grep_query_latency_by_Z.sh > kvbench_output.txt
cd -
