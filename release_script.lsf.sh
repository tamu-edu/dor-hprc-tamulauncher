

# this process will be called whenever a node or core can be released
# in terra's case it's only being used when a node becomes available
# expect three parameters:
#     1) the hostname
#     2) location of logdir
#     3) marker core_index=core -1=node
#  
#     4) concurrent commands per node 
#     5) cores per node

hostname=$1
logdir=$2
marker=$3

commands_per_node=$4
cores_per_node=$5

appname=`echo ${LSB_APPLICATION_NAME}`

if [ "${appname}" != "resizable" ]; then
   # in this case, tamulauncher not called with -app resizable, 
   # so will not be able to resize
   exit 0
fi


masterhost=${TAMULAUNCHERHOST}

let "cores_per_command=cores_per_node/commands_per_node"
#echo "release_script: hostname: ${hostname}, marker=${marker}, commands_per_node=${commands_per_node}, cores_per_node=${cores_per_node}, cores_per_command=${cores_per_command}"

# check if this script is called when a core or a node is released
if [ $marker -eq -1 ]; then
    # this means the last core on the node, if it's not the 
    # masternode release the final core
    if [ ${hostname} != ${masterhost} ]; then
	#echo "last core: bresize release ${cores_per_command}*${hostname} ${LSB_JOBID}"
	bresize release ${cores_per_command}*${hostname} ${LSB_JOBID}
    else
	# in this case, release ${cores_per_command}-1 cores
	let "leftover_cores=${cores_per_command}-1"
	if [ ${leftover_cores} -gt 0 ]; then
	    #echo "lastcore: masterhost=${masterhost}  , hostname=${hostname} :  bresize release ${leftover_cores}*${hostname} ${LSB_JOBID}"
	    bresize release ${leftover_cores}*${hostname} ${LSB_JOBID}
	fi
    fi
    
else
    # make it as simple as possible for now. No lock or anything
    # just release a single core.
    #echo "releasing single core: bresize release ${cores_per_command}*${hostname} ${LSB_JOBID}"
    bresize release ${cores_per_command}*${hostname} ${LSB_JOBID}
fi
	    


