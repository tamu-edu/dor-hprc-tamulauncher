#include "iostream"
#include <chrono>

#include "time.h"

#include "run_command_type.hpp"

using namespace std;


void 
run_command_type::fix_return_code() {
  // check if there was a signal.
  // special case for a compounded statement when the last 
  // statement (the actual command) gets a signal.
  // In that case the most significant bit is to 1
  // and the right byte holds the signal number 
  // instead of the left byte
  // for example SIGUSR (signal 12):
  //  actual:  10001100 00000000 
  //  correct: 00000000 00001100
  
  const int most_significant = 32768;
  
  int isset = return_code & most_significant;
  if (isset > 0) {
    // this means it's a SIGNAL
    // so set most significant bit to 0 and shift the number 8 positions right
    return_code = (return_code & (most_significant-1)) >> 8;
  }
}


run_command_type::run_command_type(string s,int index) {
  command_index=index;
  command = s;
}

string& 
run_command_type::get_command_string() { return command;}

int 
run_command_type::get_index() {return command_index;}

double 
run_command_type::get_runtime() {return run_time;}

void
run_command_type::set_runtime(double t) { run_time=t;}

int 
run_command_type::get_return_code() {return return_code;}

void
run_command_type::set_return_code(int c) {return_code=c;}

void 
run_command_type::execute(int task_id) {
  // tokenize                                                                                                                                            
  string buf("");

  string task_id_string = std::to_string(task_id);
  string command_index_string=std::to_string(command_index);
  buf.append("export TAMULAUNCHER_TASK_ID=");
  buf.append(task_id_string);
  buf.append("; export TAMULAUNCHER_COMMAND_ID=");
  buf.append(command_index_string);
  buf.append("; ");
  buf.append(command);
  
  time_t tstart = time(NULL);
  this->return_code = system(buf.c_str());
  time_t tend = time(NULL);
  this->run_time  = ((double) (tend - tstart));
  
  // in compound statement the return code is a bit mangled
  // when the last statement was signaled, so just fix if needed
  fix_return_code();
}

