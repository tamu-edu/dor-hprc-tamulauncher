#ifndef LOGGER_TYPE_H
#define LOGGER_TYPE_H

#include <fstream>
#include <vector>
#include <string>

#include "run_command_type.hpp"

class logger_type {

private:

  std::string log_dir;
  std::string log_hostname;
  std::ofstream log_file;
  std::ofstream signal_file;

  
public:

  logger_type(std::string& hostname, std::string& dir);  

  void open();

  void write_log(run_command_type& commands);

  void close();
};

#endif
