#!/bin/bash


# this function will create a new commands file in case
# there is an tamulauncher-log directory from a run using
# tamulauncher-classic
function check_classic()
{
    local input=$1
    local new_log_dir=$2
    local input_tl=${input}.tamulauncher.input
   
    local logdir="tamulauncher-log"
    local todofile=${logdir}/todo
    local output=""

# check if there is a directory named tamulauncher.log
# This directory will contain all the processed command
# using tamulauncher-classis
    if [ -d "${logdir}" ]; then
	rm -f ${input_tl}
	rm -f ${todofile}
	for f in ${todofile}-*
	do
        # retrieve the matching done file
	    replace="done"
	    done="${f/todo/$replace}"
	
        linestodo=`wc -l ${f} | cut -d" " -f1`
	
	if [ -f ${done} ]; then
	    linesdone=`wc -l ${done} | cut -d" " -f1`
	else
	    echo "Warning: file ${done} not found"
	    linesdone=0
	fi
	
	let "linediff = $linestodo - $linesdone"
	if (($linediff > 0)); then
	    tail -n $linediff $f >> ${input_tl}
	fi
	done
	
	# move the old log directory inside the new one
        # so we don't loose if something goes wrong.
        mv ${logdir} ${new_log_dir}/
	output=${input_tl}
	
    fi
    
    echo ${output}
}
