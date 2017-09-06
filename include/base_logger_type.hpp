#ifndef BASE_LOGGER_TYPE_H
#define BASE_LOGGER_TYPE_H

#include <vector>

#include "run_command_type.hpp"

using namespace std;

class base_logger_type {


public:

  static void setup_logs();

  virtual void open(int log_id)=0;

  virtual void write_log(vector<run_command_type>& commands)=0;

  virtual void close()=0;
};

#endif
