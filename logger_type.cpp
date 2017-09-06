#include <vector>
#include <string>

#include <sys/wait.h>

#include "logger_type.hpp"

void logger_type::open(int id) {
  log_index=id;
  string id_string = to_string(id);
  string name = ".tamulauncher/log."+id_string;
  log_file.open(name,std::fstream::app);
}


void logger_type::close() {
  log_file.close();
}


void logger_type::write_log(vector<run_command_type>& commands) {
  for (vector<run_command_type>::iterator it=commands.begin();it!=commands.end();++it) {
    run_command_type& next_command = *it;
    int ret_signal = next_command.get_return_code();
    string exitstring;
    int retcode = 0;
    if (WIFSIGNALED(ret_signal)) {
      exitstring=":: SIGNAL=";
      retcode=WTERMSIG(ret_signal); 
    } else if (WIFEXITED(ret_signal)) {
      exitstring=":: EXITCODE=";
      retcode=WEXITSTATUS(ret_signal);
    } else {
      // not a signal not a regular exit
      // no good way to handle it, should not happen
      exitstring=":: UNKNOWN=";
      retcode = -1;
    }

    log_file << next_command.get_index() << " :: " << next_command.get_command_string() <<
      " :: time spent: " << next_command.get_runtime() << exitstring << retcode << "   " << ret_signal << "\n";
    log_file.flush();
  }
}


