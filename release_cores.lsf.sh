# There will be a single multi threaded process running on every node. Whenever a thread
# has nothing left to do it will add another line to file released.<HOSTNAME>
# this function will calculate the difference of the number of lines in this file and the
# number of cores that were already released before, and call the lsf release command for that
# particular node and resources
                                                                                                                                                                          

# total number of nodes

# retrieve the actual nodes 

nodelistfile="$1"
releasedfile="$2"
batch_jobid="$3"
tasks_per_node="$4"


sleeptime=2
masternode=`hostname`

numnodes=0;
count=0
while read next_line; do
    if [ $((count%2)) -eq 0 ]; then
	# the line will read in the next host
	core_names[$numnodes]=${next_line}
	# also touch the released file
	touch ${releasedfile}.${next_line}
   else
	# the line will read the number of tasks
	cores_per_node[$numnodes]=${next_line}
	released_per_node[$numnodes]=0
	cores_left[$numnodes]=${next_line}
	let "numnodes++"
    fi
    let "count++"
done < ${nodelistfile}

# to get correct numnodes need to substract one
let "numnodes--"


while :
do
    for counter in $(seq 0 $numnodes); do
	next_host=${core_names[$counter]};

	prev_released=${released_per_node[${counter}]}
        new_released=`grep released ${releasedfile}.${next_host} | wc -l | cut -d' ' -f1 `
        let "released_diff=${new_released}-${prev_released}"
        if [ ${released_diff} -gt 0 ]; then
	    # cores have been released, so have to resize now
	    # For multi threaded jobs we need to release the number of cores
	    # as there are threads per command

	    cores_per_command=1
	    if [ ${tasks_per_node} -gt 0 ]; then
		let "cores_per_command=${cores_per_node[${counter}]}/${tasks_per_node} "
	    fi
	    let "tcores=${released_diff}*${cores_per_command}"
	    cleft=${cores_left[${counter}]}
	    if [ ${tcores} -ge ${cleft} ]; then
		tcores=${cleft}
		if [ "${masternode}" = "${next_host}" ]; then
		    let "tcores--"
		fi
	    fi
            bresize release ${tcores}*${next_host} ${batch_jobid}
	    released_per_node[${counter}]=${new_released}
	    let "cores_left[${counter}]=${cleft}-tcores"
#	    echo
#	    echo "--- released_diff=${released_diff}"   
#	    echo "released_per_node: ${released_per_node[*]}"
#	    echo "cores_left: ${cores_left[*]}"
        fi
    done

    sleep ${sleeptime}
done


