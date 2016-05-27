#ifndef WORKER_TYPE_H
#define WORKER_TYPE_H

#include <vector>

#include "run_command_type.hpp"


using namespace std;



class worker_type {

private:

  int task_id;
  vector<run_command_type> commands;
  MPI_Status status;
  
  bool receive_and_pack();
  void unpack_and_send();

public:


  worker_type();
  void start();


};

#endif
