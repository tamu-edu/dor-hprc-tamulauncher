#ifndef MASTER_TYPE_H
#define MASTER_TYPE_H

#include "mpi.h"
  
#include "commands_type.hpp"

using namespace std;

class master_type {

private:

  commands_type& commands;
  int chunksize; 
  int num_tasks;

  bool pack_and_send(int destination);

public:

  master_type(commands_type& cmds, int csize);
  void start();

};

#endif
