#ifndef BASE_LOGGER_TYPE_H
#define BASE_LOGGER_TYPE_H

#include <vector>

#include "run_command_type.hpp"

using namespace std;

class base_logger_type {


public:

  static void setup_logs();

  virtual void write_log(vector<run_command_type>& commands)=0;

};

#endif
