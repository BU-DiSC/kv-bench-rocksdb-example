#!/bin/bash

RQ_THRESHOLD_LIST=("0.0" "0.2" "0.4" "0.6"  "0.8"  "1.0")
echo "RQ_THRESHOLD,latency(ms/query)" > range_query_latency.txt
echo "RA_THRESHOLD,bytes_read(GB)" > bytes_read.txt
for RQ_THRESHOLD in ${RQ_THRESHOLD_LIST[@]}
do
	str="RQ_THRESHOLD${RQ_THRESHOLD}_"
	grep "range query latency:" kvbench_*output.txt | grep ${str} | awk -F':|-' '{print $3}' | awk -v a="${RQ_THRESHOLD}" -F' ' 'BEGIN{x=0}{x+=$1}END{if(x>0) print a","x/NR}' >> range_query_latency.txt
	grep "bytes read in total:" kvbench_*output.txt | grep ${str} | awk -F':|-' '{print $3}' | awk -v a="${RQ_THRESHOLD}" -F' ' 'BEGIN{x=0}{x+=$1}END{if(x>0) print a","x/1024/NR}' >> bytes_read.txt
done
