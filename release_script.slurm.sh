# this process will be called whenever a node or core can be released
# in terra's case it's only being used when a node becomes available
# expect two parameters:
#     1) the hostname
#     2) location of logdir
#     3) marker 0=core 1=node

# this process can run on multiple nodes concurrently, we need to be
# careful that two processes are not trying to release at the same time
# We use the following approach

# all processes have a a file named RUNNING.$hostname created before the run started
#
# 1) keep checking lock RELEASE.* until there are no locks
# 2) set a lock RELEASE.$hostname
# 3) rm file RUNNING.$hostname
# 4) wait for a few seconds (doesn't matter how long since the node is finished
# 5) list all the lock files, sort them 
# 6) if my lock file is first in sorted list
#      7) create a new NodeList 
#      8) wait for few seconds
#      9) clear lock
#      10) execute the scontrol update command 
#    else
#      11) all process will clear their lock

 
hostname=$1
logdir=$2
marker=$3


# check if this script is called when a core or a node is released
if [ $marker -gt -1 ]; then
    # on slurm can only release whole nodes so just exit
    exit 0
fi

# go to the log directory
cd ${logdir}


lockfile=RELEASELOCK


# step 1:
# wait until there is no release lock
alllocks=`ls | grep ${lockfile}`
while [ ! -z "${alllocks}" ]; 
do
    sleep 3
    alllocks=`ls  | grep ${lockfile}`
    #echo "${hostname}: waiting for lock"
done

# step 2:
# create a lock
mylock=${lockfile}.${hostname}
touch ${mylock}

# step 3:
# this node can be released, so remove the marker
rm RUNNING.${hostname}


# step 4:
# wait few more seconds, so every that arrived  
# around same time has a chance to create the lock
sleep 2

# step 5:
# get a sorted listing of all the locks
sortedlocks=`ls ${lockfile}.* | sort -u` 


# step 6:
# find out which release script (on which node) will do
# the actual updating of the node list. Easiest way to
# do this is to just sort the list and get the first
# element.
firstlock=`echo ${sortedlocks} | cut -d " " -f1`
#echo "sortedlocks=${sortedlocks}"
if [ ${firstlock} = ${mylock} ]; 
then
    # step 7:
    # create a list of nodes that are still active
    remaininghosts=`ls | grep RUNNING | tr '\n' ','  | sed 's/RUNNING.//g' | sed 's/.cluster//g' `
    #echo "${hostname}:remaininghosts=${remaininghosts}"

    #step 8:
    # need to wait for a few seconds, to make sure the other  
    # release scripts have enough time to release their locks
    sleep 7

    # step 9:
    # need to remove the lock before the scontrol command. Otheriwise 
    # the script will be killed before the lock can be removed.
    rm ${mylock}

    # step 10:
    # time to call the scontrol update command, this will mean that 
    # the current script will be kill immediately after calling it 
    if [ ! -z "${remaininghosts}" ]; then
	scontrol update JobId=$SLURM_JOB_ID NodeList=${remaininghosts} 
	#. slurm_job_${SLURM_JOB_ID}_resize.sh
	#rm -f slurm_job_${SLURM_JOB_ID}_resize.sh
    fi
else
    # step 11:
    # Every other release script will just release the lock
    rm ${mylock}
    #echo "${hostname}: releasing lock"
fi
