#!/bin/bash

P=4096
B=8
T=4
E=1024
R=1
bpk=8
BCC=16384
DB_HOME="/scratchNVM1/zczhu/test_db_dir/db_working_home"

throughput_interval=10000
D_LIST=("100000")
#Z_list=("0.5")
#ZD_list=("0" "3")

mkdir -p "output/"
for D in ${D_LIST[@]}
do
	U=`echo "1000000-${D}" | bc`
	echo "./plain_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --iwp /scratchHDDa/zczhu/K-V-Workload-Generator/ingestion_workload.txt --qwp /scratchHDDa/zczhu/K-V-Workload-Generator/kvbench_D${D}_U${U}_ingestion_workload.txt -B ${B} -P 4096 -b ${bpk} --BCC ${BCC} -V 1 --dw --dr --clct-tputi ${throughput_interval} > output/kvbench_D${D}_U${U}_interleaved_output.txt"
	./plain_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --iwp /scratchHDDa/zczhu/K-V-Workload-Generator/ingestion_workload.txt --qwp /scratchHDDa/zczhu/K-V-Workload-Generator/kvbench_D${D}_U${U}_ingestion_workload.txt -B ${B} -P 4096 -b ${bpk} --BCC ${BCC} -R ${R} -V 1 --dw --dr --clct-tputi ${throughput_interval} > output/kvbench_D${D}_U${U}_interleaved_output.txt
	mv throughputs.txt output/kvbench_D${D}_U${U}_interleaved_throughputs.txt
	rm ${DB_HOME}/*
	cd "/scratchHDDa/zczhu/K-V-Workload-Generator/"
	grep "^U" kvbench_D${D}_U${U}_ingestion_workload.txt > kvbench_D${D}_U${U}_sequential_ingestion_workload.txt
	grep "^D" kvbench_D${D}_U${U}_ingestion_workload.txt >> kvbench_D${D}_U${U}_sequential_ingestion_workload.txt
	grep "^Q" kvbench_D${D}_U${U}_ingestion_workload.txt >> kvbench_D${D}_U${U}_sequential_ingestion_workload.txt
	cd -
	echo "./plain_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --iwp /scratchHDDa/zczhu/K-V-Workload-Generator/ingestion_workload.txt --qwp /scratchHDDa/zczhu/K-V-Workload-Generator/kvbench_D${D}_U${U}_sequential_ingestion_workload.txt -B ${B} -P 4096 -b ${bpk} --BCC ${BCC} -V 1 --dw --dr --clct-tputi ${throughput_interval} > output/kvbench_D${D}_U${U}_sequential_output.txt"
	./plain_benchmark -T ${T} -E ${E} --dd -p ${DB_HOME} --iwp /scratchHDDa/zczhu/K-V-Workload-Generator/ingestion_workload.txt --qwp /scratchHDDa/zczhu/K-V-Workload-Generator/kvbench_D${D}_U${U}_sequential_ingestion_workload.txt -B ${B} -P 4096 -b ${bpk} --BCC ${BCC} -R ${R} -V 1 --dw --dr --clct-tputi ${throughput_interval} > output/kvbench_D${D}_U${U}_sequential_output.txt
	mv throughputs.txt output/kvbench_D${D}_U${U}_sequential_throughputs.txt
	rm ${DB_HOME}/*
done
