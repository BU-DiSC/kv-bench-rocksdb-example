#!/bin/bash

P=4096
B=8
T=4
E=1024
R=1
BCC=16384
DB_HOME="/scratchNVM1/zczhu/test_db_dir/db_working_home"

Z_list=("0.0" "0.1" "0.2" "0.3" "0.4" "0.5" "0.6" "0.7" "0.8" "0.9" "1.0")
ZD_list=("0" "3")
Z_list=("0.5")
ZD_list=("0" "3")

mkdir -p "output/"
for ZD in ${ZD_list[@]}
do
	for Z in ${Z_list[@]}
	do
		echo "./plain_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --iwp /scratchHDDa/zczhu/K-V-Workload-Generator/ingestion_workload.txt --qwp /scratchHDDa/zczhu/K-V-Workload-Generator/kvbench_Z${Z}_ZD${ZD}_query_workload.txt -B ${B} -P 4096 --BCC ${BCC} -V 1 --dw --dr > output/kvbench_Z${Z}_ZD${ZD}_output.txt"
		./plain_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --iwp /scratchHDDa/zczhu/K-V-Workload-Generator/ingestion_workload.txt --qwp /scratchHDDa/zczhu/K-V-Workload-Generator/kvbench_Z${Z}_ZD${ZD}_query_workload.txt -B ${B} -P 4096 --BCC ${BCC} -R ${R} -V 1 --dw --dr > output/kvbench_Z${Z}_ZD${ZD}_output.txt
		rm ${DB_HOME}/*
	done
done
cd output/
./grep_query_latency_by_Z.sh > kvbench_output.txt
cd -
