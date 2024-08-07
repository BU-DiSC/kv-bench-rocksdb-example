// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include <math.h>
#include <limits.h>
#include <atomic>
#include <mutex>
//#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iomanip>
#include "rocksdb/iostats_context.h"
#include "rocksdb/perf_context.h"
#include "args.hxx"
#include "aux_time.h"
#include "emu_environment.h"
#include "workload_stats.h"
#include "emu_util.h"


using namespace rocksdb;

QueryTracker global_query_tracker;
DB* db = nullptr;

int runExperiments(EmuEnv* _env);    // API
int parse_arguments2(int argc, char *argv[], EmuEnv* _env);

void merge_tput_vectors(std::vector<double>* origin_tput, std::vector<double>* new_tput) {
  if (origin_tput == nullptr || new_tput == nullptr) return;
  if (origin_tput->size() == 0) {
    *origin_tput = *new_tput;
  }
  for (int i = 0; i < origin_tput->size() && i < new_tput->size(); i++) {
    origin_tput->at(i) += new_tput->at(i);
  }
}

int main(int argc, char *argv[]) {
  // check emu_environment.h for the contents of EmuEnv and also the definitions of the singleton experimental environment 
  EmuEnv* _env = EmuEnv::getInstance();
  // parse the command line arguments
  if (parse_arguments2(argc, argv, _env)) {
    exit(1);
  }
    
  my_clock start_time, end_time;
  std::cout << "Starting experiments ..."<<std::endl;
  if (my_clock_get_time(&start_time) == -1) {
    std::cerr << "Failed to get experiment start time" << std::endl;
  }
  int s = runExperiments(_env); 
  if (my_clock_get_time(&end_time) == -1) {
    std::cerr << "Failed to get experiment end time" << std::endl;
  }
  double experiment_exec_time = getclock_diff_ns(start_time, end_time);

  std::cout << std::endl << std::fixed << std::setprecision(2) 
            << "===== End of all experiments in "
            << experiment_exec_time/1000000 << "ms !! ===== "<< std::endl;
  
  // show average results for the number of experiment runs
  printEmulationOutput(_env, &global_query_tracker, _env->experiment_runs);

  return 0;
}

// Run rocksdb experiments for experiment_runs
// 1.Initiate experiments environment and rocksDB options
// 2.Preload workload into memory
// 3.Run workload and collect stas for each run
int runExperiments(EmuEnv* _env) {
 
  Options options;
  WriteOptions write_options;
  ReadOptions read_options;
  BlockBasedTableOptions table_options;
  FlushOptions flush_options;
 
  WorkloadDescriptor query_wd(_env->query_wpath);
 
  //WorkloadDescriptor wd(workloadPath);
  // init RocksDB configurations and experiment settings
  configOptions(_env, &options, &table_options, &write_options, &read_options, &flush_options);
   EnvOptions env_options (options);
  // parsing workload
  loadWorkload(&query_wd);
      
  std::vector<double> throughput_collector;
  std::vector<double> temp_collector;
  
  // Starting experiments
  assert(_env->experiment_runs >= 1);
  for (int i = 0; i < _env->experiment_runs; ++i) {
    // Reopen DB
    if (_env->destroy) {
      Status destroy_status = DestroyDB(_env->path, options);
      if (!destroy_status.ok()) std::cout << destroy_status.ToString() << std::endl;
    }
    Status s;
    QueryTracker *query_track = new QueryTracker();
    s = DB::Open(options, _env->path, &db);
    if (!s.ok()) std::cerr << s.ToString() << std::endl;
    assert(s.ok());
    
    get_iostats_context()->Reset(); 
    if (_env->throughput_collect_interval == 0) { 
      runWorkload(db, _env, &options, &table_options, &write_options, &read_options, &flush_options, &env_options, &query_wd, query_track);
    } else {
      temp_collector.clear();
      runWorkload(db, _env, &options, &table_options, &write_options, &read_options, &flush_options, &env_options, &query_wd, query_track, &temp_collector);
      merge_tput_vectors(&throughput_collector, &temp_collector);
    }

    // Collect stats after per run
    populateQueryTracker(query_track, db, table_options, _env);
    if (_env->verbosity > 1) {
      std::string state;
      db->GetProperty("rocksdb.cfstats-no-file-histogram", &state);
      std::cout << state << std::endl;
    }
  
    dumpStats(&global_query_tracker, query_track);    // dump stat of each run into acmulative stat
    if (query_track->point_lookups_completed + query_track->zero_point_lookups_completed > 0) {
      std::cout << std::fixed << std::setprecision(6) << "point query latency: " <<  static_cast<double>(query_track->point_lookups_cost +
      query_track->zero_point_lookups_cost)/(query_track->point_lookups_completed + query_track->zero_point_lookups_completed)/1000000 << " (ms/query)" << std::endl;
    }
    if (query_track->range_lookups_completed > 0) {
      std::cout << std::fixed << std::setprecision(6) << "range query latency: " <<  static_cast<double>(query_track->range_lookups_cost)/query_track->range_lookups_completed/1000000 << " (ms/query)" << std::endl;
      std::cout << std::fixed << std::setprecision(6) << "bytes read in total: " <<  static_cast<double>(query_track->bytes_read)/1024/1024 << " MB" << std::endl;
    }
    if (query_track->total_completed > 0) {
      std::cout << std::fixed << std::setprecision(6) << "avg operation latency: " <<  static_cast<double>(query_track->workload_exec_time)/query_track->total_completed/1000000 << " (ms/ops)" << std::endl;
    }
    delete query_track;
    ReopenDB(db, options, flush_options);
    CloseDB(db, flush_options);
   
    std::cout << "End of experiment run: " << i+1 << std::endl;
    std::cout << std::endl;
  }
  if (_env->throughput_collect_interval > 0) {
    for (int i = 0; i < throughput_collector.size(); i++) {
      throughput_collector[i] /= _env->experiment_runs;
    }
    write_collected_throughput({throughput_collector}, {"throughput"}, _env->throughput_path, _env->throughput_collect_interval);
  }

  return 0;
}



int parse_arguments2(int argc, char *argv[], EmuEnv* _env) {
  args::ArgumentParser parser("RocksDB_parser.", "");

  args::Group group1(parser, "This group is all exclusive:", args::Group::Validators::DontCare);
  args::Group group4(parser, "Optional switches and parameters:", args::Group::Validators::DontCare);
  args::ValueFlag<int> size_ratio_cmd(group1, "T", "The size ratio of two adjacent levels  [def: 2]", {'T', "size_ratio"});
  args::ValueFlag<int> buffer_size_in_pages_cmd(group1, "P", "The number of pages that can fit into a buffer [def: 128]", {'P', "buffer_size_in_pages"});
  args::ValueFlag<int> entries_per_page_cmd(group1, "B", "The number of entries that fit into a page [def: 128]", {'B', "entries_per_page"});
  args::ValueFlag<int> entry_size_cmd(group1, "E", "The size of a key-value pair inserted into DB [def: 128 B]", {'E', "entry_size"});
  args::ValueFlag<long> buffer_size_cmd(group1, "M", "The size of a buffer that is configured manually [def: 2 MB]", {'M', "memory_size"});


  args::Flag no_block_cache_cmd(group1, "block_cache", "Disable block cache", {"dis_blk_cache", "disable_block_cache"});
  args::ValueFlag<int> block_cache_capacity_cmd(group1, "block_cache_capacity", "The capacity (kilobytes) of block cache [def: 8 MB]", {"BCC", "block_cache_capacity"});
  args::ValueFlag<int> experiment_runs_cmd(group1, "experiment_runs", "The number of experiments repeated each time [def: 1]", {'R', "run"});
  args::Flag destroy_cmd(group4, "destroy_db", "Destroy and recreate the database", {"dd", "destroy_db"});
  args::Flag direct_reads_cmd(group4, "use_direct_reads", "Use direct reads", {"dr", "use_direct_reads"});
  args::Flag direct_writes_cmd(group4, "use_direct_writes", "Use direct writes", {"dw", "use_direct_writes"});
  args::ValueFlag<int> verbosity_cmd(group4, "verbosity", "The verbosity level of execution [0,1,2; def: 0]", {'V', "verbosity"});

  args::ValueFlag<std::string> path_cmd(group4, "path", "path for writing the DB and all the metadata files", {'p', "path"});
  args::ValueFlag<std::string> query_wpath_cmd(group4, "wpath", "path for query workload files", {"qwp", "query-wpath"});

  args::ValueFlag<uint32_t> collect_throughput_interval_cmd(group4, "collect_throughput_interval", "The interval of collecting the overal throughput", {"clct-tputi", "collect-throughput_interval"});
  args::ValueFlag<std::string> throughput_path_cmd(group4, "throughput_path", "path for dumping the collected throughputs when executing the workload", {"tput-op", "throughput-output-path"});

  try {
      parser.ParseCLI(argc, argv);
  }
  catch (args::Help&) {
      std::cout << parser;
      exit(0);
      // return 0;
  }
  catch (args::ParseError& e) {
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      return 1;
  }
  catch (args::ValidationError& e) {
      std::cerr << e.what() << std::endl;
      std::cerr << parser;
      return 1;
  }

  _env->size_ratio = size_ratio_cmd ? args::get(size_ratio_cmd) : _env->size_ratio;
  _env->buffer_size_in_pages = buffer_size_in_pages_cmd ? args::get(buffer_size_in_pages_cmd) : _env->buffer_size_in_pages;
  _env->entries_per_page = entries_per_page_cmd ? args::get(entries_per_page_cmd) : _env->entries_per_page;
  _env->entry_size = entry_size_cmd ? args::get(entry_size_cmd) : _env->entry_size;
  _env->buffer_size = buffer_size_cmd ? args::get(buffer_size_cmd) : _env->buffer_size_in_pages * _env->entries_per_page * _env->entry_size;
  _env->verbosity = verbosity_cmd ? args::get(verbosity_cmd) : _env->verbosity;
  _env->no_block_cache = no_block_cache_cmd ? true : false;
  _env->block_cache_capacity = block_cache_capacity_cmd ? args::get(block_cache_capacity_cmd) : _env->block_cache_capacity;

  _env->experiment_runs = experiment_runs_cmd ? args::get(experiment_runs_cmd) : _env->experiment_runs;
  _env->destroy = destroy_cmd ? true : _env->destroy;
  _env->use_direct_reads = direct_reads_cmd ? true : _env->use_direct_reads;
  _env->use_direct_io_for_flush_and_compaction = direct_writes_cmd ? true : _env->use_direct_io_for_flush_and_compaction;
  _env->path = path_cmd ? args::get(path_cmd) : _env->path;
  _env->query_wpath = query_wpath_cmd ? args::get(query_wpath_cmd) : _env->query_wpath;

  _env->throughput_collect_interval = collect_throughput_interval_cmd ? args::get(collect_throughput_interval_cmd) : _env->throughput_collect_interval;
  _env->throughput_path = throughput_path_cmd ? args::get(throughput_path_cmd) : _env->throughput_path;

  return 0;
}




