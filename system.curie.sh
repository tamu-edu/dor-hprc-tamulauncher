
# this file contains functions to retrieve information
# from batch run (e.g. number of procs) and defines some
# variables and paths

#max number of cores per node
maxppn=16
interactive=8

# define the mpi lancher. Still has to be mpiexec.hydra right now
# if use mpirun will complain cannot find mpiexec.hydra.
# NOTE 1: before had to use mpiexec.hydra because mpirun didn't
#         I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=0, newer impi versions do
# NOTE 2: there is an issue with mpiexec.hydra: it doesn't propagate
#         ulimits to remote nodes.
launcher=<MPIRUN>


# these flags are specific to mpich. When using OpenMPI would be -np and -nperhost
tasksflag=-np
taskspernodeflag=-ppn

#path to executable
exec=<TAMULAUNCHERBASE>/tamulauncher-src/tamulauncher-loadbalanced.x

#path to log file
tamulauncherlog=<TAMULAUNCHERBASE>/log/tamulauncher.log


function get_job_id()
{
        local myresult=$LSB_JOBID
        if [ -z "$myresult" ]; then
           # in this case it's run on login node, set jobid to 000
           myresult=000
        fi
        echo "$myresult"
}


function get_num_nodes()
{
        local nn=`echo ${LSB_MCPU_HOSTS} | wc -w`
        if [ $nn -eq 0 ]; then
           # in this case it's run on login node, set result to 2
           nn=2
        fi
           expr $nn / 2
}


function create_node_task_files()
{
        local temp1=${LSB_MCPU_HOSTS}
        if [ -z "${temp1}" ]; then
             temp1="$HOSTNAME $interactive"
        fi
        echo  ${temp1} | sed 's/ /\n/g' > $1
}


function write_mod_log()
{
        local jobid=${LSB_JOBID}
        local jobname=${LSB_JOBFILENAME}
        if [ -z "$jobid" ]; then
           jobid=None
           jobname=None
        fi
        modlog=/software/lmod/hprc/logs/`date +%Y%m%d`

        # log message format from: /sw/lmod/hprc/mods/hprc_sp_log_modules.lua
        # local msg   = string.format("%s %s %s from %s:%s - \(LSF: %s %s\) - %s", user, mode, t.modFullName, host, pwd, jobid, jobfile, t.fn)
        # cmd = "echo `date +%H%M%S` '" .. msg .. "' >> "..logfile..""
        for mod in <GCCCOREMODULE> <MPIMODULE> ; do
           echo "`date +%H%M%S` $USER using $mod from $HOSTNAME:$PWD - (LSF: ${jobid} ${jobname}) - /software/easybuild/modules/all/$mod" >> $modlog
        done
}


function set_cluster_specific_env()
{
   echo ""
   # nothing to do here
}

