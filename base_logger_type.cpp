
#include "base_logger_type.hpp"

void base_logger_type::setup_logs() {
  system("mkdir -p .tamulauncher ; touch .tamulauncher/log_dummy");
  system("grep SIGNAL   .tamulauncher/log* | cut -d\" \" -f1 | cut -d\":\" -f2 > .tamulauncher.signaled");
  system("grep EXITCODE .tamulauncher/log* | cut -d\" \" -f1 | cut -d\":\" -f2 > .tamulauncher.processed");
  system("grep EXITCODE .tamulauncher/log* | grep -v \"EXITCODE=0\" | cut -d\" \" -f1 | cut -d\":\" -f2 > .tamulauncher.non_zero");
}

