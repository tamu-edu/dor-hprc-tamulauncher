Tool to run a large number of serial or multi-threaded commands using a single job.

For information about using tamulauncher, check
https://hprc.tamu.edu/kb/Software/tamulauncher/

Implementation:

tamulauncher uses a hybrid approach; it will first split up all the commands into N chunks, where N is the number of nodes. Then, tamulauncher will start a single MPI task per node, and
within the node, every task will start T threads (specified using the tamulauncher --commands-per-node flag or, if batch scheduler provides tasks per node value, it will use that value.


Example, Slurmn (say, TC  threads for each command):
    
    --nodes=N, --tasks-per-node=T --cpus-per-task=TC
 
For other batch schedulers that don't have option to differentiate between cores and tasks, 
user will specify commands per node.


NOTE, on TAMU clusters, cpu limits are strongly enforced, so the traditional way "--nodes=N, --tasks-per-node=T" 
does not work, because number of threads will be limited to 1, essentially serializing intra node processing.

Solution:

   --nodes=N, --tasks-per-node=1 --cpus-per-task=TC*T
   tamulauncher --commands-per-node=T

Tamulauncher is modularized, where system specifics are defined in system files (e.g., system.aces.sh). This file specifies the number of cores per node and functions to retrieve submission parameters. If tamulauncher is installed on a new system, probably need to create a new systems file and update the Makefile

Important files:
run_many_instances.cpp --> performs actual execution of commands.
tamulauncher --> partitions input based on #procs and restart

Building for cluster <target>:
run make clean <target> install
Will compile the c++ file and copy the files to ../bin


