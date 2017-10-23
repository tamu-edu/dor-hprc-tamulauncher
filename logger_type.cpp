#include <vector>
#include <string>

#include <sys/wait.h>

#include "logger_type.hpp"



logger_type::logger_type(std::string& hostname){
  log_hostname=hostname;
}

void logger_type::open() {
  string name = ".tamulauncher-log/log." + log_hostname;
  log_file.open(name,std::fstream::app);
}


void logger_type::close() {
  log_file.close();
}


void logger_type::write_log(run_command_type& command) {
  int ret_signal = command.get_return_code();
  string exit_string;
  int ret_code = 0;
  if (WIFSIGNALED(ret_signal)) {
    exit_string=" signal code  ";
    ret_code=WTERMSIG(ret_signal); 
  } else if (WIFEXITED(ret_signal)) {
    exit_string=" exit code ";
    ret_code=WEXITSTATUS(ret_signal);
  } else {
    // not a signal not a regular exit
    // no good way to handle it, should not happen
    exit_string=" unknown ";
    ret_code = 0;
  }
  
  std::string output = std::to_string(command.get_index()) + " :" + exit_string + 
    std::to_string(ret_code) + ": elapsed time = " + std::to_string(command.get_runtime()) + 
    "s # " + command.get_command_string() + "\n";  

  // ready to print it. Need to put it in a critical block, to avoid other threads from 
  // writing to the same file at the same time.
#pragma omp critical
  {  
    log_file << output;
    log_file.flush();
  }
}


