#include "emu_environment.h"

// Specify your path of workload file here
std::string ingestion_workloadPath = "./workload.txt";
std::string query_workloadPath = "./workload.txt";
std::string kDBPath = "./db_working_home";
std::string throughputPath = "./throughputs.txt";


/*Set up the singleton object with the experiment wide options*/
EmuEnv* EmuEnv::instance = 0;


EmuEnv::EmuEnv() 
{
// Options set through command line 
  size_ratio = 2;
  buffer_size_in_pages = 128;
  entries_per_page = 128;
  entry_size = 128;                                                    // in Bytes 
  buffer_size = buffer_size_in_pages * entries_per_page * entry_size;  // M = P*B*E = 128 * 128 * 128 B = 2 MB 
  verbosity = 0;

  experiment_runs = 1;                // run
  destroy = true;                     // dd
  use_direct_reads = false;           // dr
  use_direct_io_for_flush_and_compaction = false; //dw

  // TableOptions
  no_block_cache = false;                                        // TBC
  block_cache_capacity = 8*1024;				// KB (default 8 MB)

  // Other DBOptions
  create_if_missing = true;


  path = kDBPath; 
  ingestion_wpath = ingestion_workloadPath; 
  query_wpath = query_workloadPath; 

  string experiment_name = ""; 
  string experiment_starting_time = "";

  show_progress=false;
  throughput_collect_interval = 0;
  throughput_path = throughputPath;
}

EmuEnv* EmuEnv::getInstance()
{
  if (instance == 0)
    instance = new EmuEnv();

  return instance;
}
