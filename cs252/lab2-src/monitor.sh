#!/bin/bash

#Do not insert code here

#DO NOT REMOVE THE FOLLOWING TWO LINES
git add $0 >> .local.git.out
git commit -a -m "Lab 2 commit" >> .local.git.out


# cycles per second
hertz=$(getconf CLK_TCK)

function check_arguments {


	#If number of arguments is less than 5, exit. For part 2, the number of arguments should be greater than 7
	if [ "$1" -lt 7 ]; then
		echo "USAGE: "
		echo "$0 {process id} -cpu {cpu usage percentage} -mem {maximum memory in kB} {maximum reports} {time interval}"
		exit
	fi

	CPU_THRESHOLD=$4

	MEMORY_THRESHOLD=$6

	#Extract the memory threshold (part 2 of the script)


}

function init
{

	#Remove reports directory
	rm -fr ./reports_dir
	mkdir ./reports_dir

	REPORTS_DIR=./reports_dir

	PID=$1 #This is the pid we are going to monitor

	TIME_INTERVAL=${@:$#} #Time interval is the last argument

	MAXIMUM_REPORTS=${@:$#-1:1} #Maximum reports is the argument before the last

}

#This function calculates the CPU usage percentage given the clock ticks in the last $TIME_INTERVAL seconds
function jiffies_to_percentage {
	
	#Get the function arguments (oldstime, oldutime, newstime, newutime)


	#Calculate the elpased ticks between newstime and oldstime (diff_stime), and newutime and oldutime (diff_stime)
	diff_utime=$(($3-$1))
	diff_stime=$(($4-$2))

	#You will use the following command to calculate the CPU usage percentage. $TIME_INTERVAL is the user-provided time_interval
	#Note how we are using the "bc" command to perform floating point division

	echo "100 * ($diff_stime + $diff_utime) / $hertz / $TIME_INTERVAL" | bc -l
}


#This function takes as arguments the cpu usage and the memory usage that were last computed
function generate_report {

	
	#if ./reports_dir has more than $MAXIMUM_REPORTS reports, then, delete the oldest report to have room for the current one
	if [ $(ls reports_dir | wc -l) -ge $MAXIMUM_REPORTS ]; then
	    #echo "file exeeded, file number $(ls reports_dir | wc -l)" 
        rm reports_dir/$(ls reports_dir | head -1)
    fi

	#Name of the report file
	file_name="$(date +'%d.%m.%Y.%H.%M.%S')"

	#Extracting process name from /proc
	process_name=$(cat /proc/$PID/stat | awk '{print $2}')

	#You may uncomment the following lines to generate the report. Make sure the first argument to this function is the CPU usage
	#and the second argument is the memory usage

	echo "PROCESS ID: $PID" > ./reports_dir/$file_name
	echo "PROCESS NAME: $process_name" >> ./reports_dir/$file_name
	echo "CPU USAGE: $1 %" >> ./reports_dir/$file_name
	echo "MEMORY USAGE: $2 kB" >> ./reports_dir/$file_name
}

#Returns a percentage representing the CPU usage
function calculate_cpu_usage {

	#CPU usage is measured over a periode of time. We will use the user-provided interval_time value to calculate 
	#the CPU usage for the last interval_time seconds. For example, if interval_time is 5 seconds, then, CPU usage
	#is measured over the last 5 seconds


	#First, get the current utime and stime (oldutime and oldstime) from /proc/{pid}/stat
    
    #add my code here
    #egrep "utime" /proc/$1/stat
    oldutime=$(cat /proc/$PID/stat | awk '{print $14}')
    oldstime=$(cat /proc/$PID/stat | awk '{print $15}')

    #add my code end

	#Sleep for time_interval
	sleep $TIME_INTERVAL

	#Now, get the current utime and stime (newutime and newstime) /proc/{pid}/stat
    newutime=$(cat /proc/$PID/stat | awk '{print $14}')
    newstime=$(cat /proc/$PID/stat | awk '{print $15}')
	#The values we got so far are all in jiffier (not Hertz), we need to convert them to percentages, we will use the function
	#jiffies_to_percentage

	percentage=$(jiffies_to_percentage $oldutime $oldstime $newutime $newstime)


	#Return the usage percentage
	echo "$percentage" #return the CPU usage percentage
}

function calculate_mem_usage
{
	#Let us extract the VmRSS value from /proc/{pid}/status
    mem_usage=$(grep VmRSS /proc/$PID/status | awk '{print $2}')

	#Return the memory usage
	echo "$mem_usage"
}

function notify
{
	#We convert the float representating the CPU usage to an integer for convenience. We will compare $usage_int to $CPU_THRESHOLD
	cpu_usage_int=$(printf "%.f" $1)

	#Check if the process has exceeded the thresholds
	if [ $cpu_usage_int -gt $CPU_THRESHOLD ] ;  then
	    # send an email
	    /usr/bin/mailx -s "CPU Usage Exceeds Report" $USER < reports_dir/$(ls reports_dir | tail -1)
	 fi

	#Check if process exceeded its CPU or MEM thresholds. If that is the case, send an email to $USER containing the last report
    if [ $mem_usage -gt $MEMORY_THRESHOLD ] ;  then
    	# send an email
    	/usr/bin/mailx -s "Memory Usage Exceeds Report" $USER < reports_dir/$(ls reports_dir | tail -1)
    fi

}


check_arguments $# $@

init $1 $@

echo "CPU THRESHOLD: $CPU_THRESHOLD"
#code add by myself
echo "MEMORY THRESHOLD: $MEMORY_THRESHOLD"
#code add by myself end
echo "MAXIMUM REPORTS: $MAXIMUM_REPORTS"
echo "Time interval: $TIME_INTERVAL" 

#The monitor runs forever
while [ -n "$(ls /proc/$PID)" ] #While this process is alive
do
	#part 1
	cpu_usage=$(calculate_cpu_usage)
	#echo $cpu_usage

	#part 2
	mem_usage=$(calculate_mem_usage)
	#echo $mem_usage

	generate_report $cpu_usage $mem_usage

	#Call the notify function to send an email to $USER if the thresholds were exceeded
	notify $cpu_usage $mem_usage

done