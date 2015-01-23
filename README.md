tamulauncher-src
================

src files for tamulauncher

important files:
run_many_instances.cpp --> performs actual execution of commands.
tamulauncher --> partitions input based on #procs and restart

Building:
run make install
will compile the c++ file and copy the files to ../bin

libraries and mpirun is hardcoded in the sources.
Currently everything is compiled using ictce/6.3.5


TODO:
- when there is no todo file in tamulauncher dir run_many_instances will run forever

- run_many_instances will return with value 21, tamulauncher will check for this value
  not guaranteed to be safe
