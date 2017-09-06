#ifndef MASTER_TYPE_H
#define MASTER_TYPE_H

#include "mpi.h"
  
#include "commands_type.hpp"
#include "base_logger_type.hpp"
#include "base_master_worker_type.hpp"

using namespace std;

class master_type : base_master_worker_type {

private:
  
  base_logger_type& logger;
  commands_type& commands;
  int chunksize; 
  int num_tasks;

  int receive_and_log();
  bool pack_and_send(int destination);

public:

  master_type(commands_type& cmds, base_logger_type& l ,int csize, int logtype);
  void start();

};

#endif
