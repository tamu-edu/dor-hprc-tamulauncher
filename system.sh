
# this file contains functions to retrieve information
# from batch run (e.g. number of procs) and defines some 
# variables and paths

#max number of cores per node
maxppn=20


# define the mpi lancher. Still has to be mpiexec.hydra right now
# if use mpirun will complain cannot find mpiexec.hydra.
# NOTE 1: before had to use mpiexec.hydra because mpirun didn't
#         I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=0, newer impi versions do
# NOTE 2: there is an issue with mpiexec.hydra: it doesn't propagate
#         ulimits to remote nodes.
launcher=/sw/eb/software/impi/2017.3.196-iccifort-2017.4.196-GCC-6.4.0-2.28/bin64/mpiexec.hydra


#path to executable
exec=/software/hprc/tamulauncher/src-git/tamulauncher-loadbalanced.x

#path to log file
tamulauncherlog=/software/hprc/tamulauncher/log/tamulauncher.log

function get_num_procs() 
{
	local myresult=$LSB_DJOB_NUMPROC
	if [ -z "$myresult" ]; then
	   myresult=0
	fi	
	echo "$myresult"
}

function get_num_nodes() 
{
	local myresult
	if [ -n "$LSB_DJOB_NUMPROC" ]; then
           let "myresult=$LSB_DJOB_NUMPROC/$maxppn"
           if (($LSB_DJOB_NUMPROC % $maxppn > 0)); then
	      let "myresult=$myresult+1"
           fi
        else
	   myresult=0
	fi
	echo "$myresult"
}

function get_job_id() 
{
	local myresult=$LSB_JOBID
	echo "$myresult"
}	

