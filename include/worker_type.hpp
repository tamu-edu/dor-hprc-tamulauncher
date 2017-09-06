#ifndef WORKER_TYPE_H
#define WORKER_TYPE_H

#include <vector>

#include "run_command_type.hpp"
#include "base_logger_type.hpp"
#include "base_master_worker_type.hpp"

using namespace std;

class worker_type : base_master_worker_type {

private:

  int task_id;
  base_logger_type& logger;
  vector<run_command_type> commands;
  //  MPI_Status status;
  
  bool receive_and_pack();
  void unpack_and_send();

public:


  worker_type(base_logger_type& lg, int logtype);
  void start();


};

#endif
