#ifndef BASE_MASTER_WORKER_TYPE_H
#define BASE_MASTER_WORKER_TYPE_H


using namespace std;

class base_master_worker_type {

private:
  
  int log_type;

public:

  static const int WORKER_LOGS = 1;
  static const int MASTER_LOGS = 2;

  base_master_worker_type(int lt) {
    log_type=lt;
  }

  int get_log_type() { return log_type;}

  virtual void start() = 0;

};

#endif
