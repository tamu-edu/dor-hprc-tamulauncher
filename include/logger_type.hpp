#ifndef LOGGER_TYPE_H
#define LOGGER_TYPE_H

#include <fstream>
#include <vector>

#include "base_logger_type.hpp"

class logger_type : public base_logger_type{

private:

  const string base_name = ".tamulauncher.log";
 
  ofstream log_file;

  
public:

  logger_type(int id);

  void write_log(vector<run_command_type>& commands);

  void close();
};

#endif
