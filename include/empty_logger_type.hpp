#ifndef EMPTY_LOGGER_TYPE_H
#define EMPTY_LOGGER_TYPE_H


class empty_logger_type : public base_logger_type{


public:

  void write_log(vector<run_command_type>& commands) {
    // nothing to do
  }

};

#endif
