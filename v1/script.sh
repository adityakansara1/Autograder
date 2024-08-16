> average_response_time.txt
> average_throughput.txt
> average_timeout_rate.txt
> average_error_rate.txt
> average_request_sent_rate.txt
> average_active_threads.txt
> average_cpu_utilization.txt

if [ $# != 4 ]; then
    echo "Invalid Usage: loadtest.sh <numClients> <loopNum> <sleepTimeInSeconds> <timeout>"
fi

ip="127.0.0.1"
port="5000"
client_executable="./simple-client-cpp"
server_executable="./simple-server-cpp"

./$server_executable $port > /dev/null &
echo $?
server_pid=$!

for ((j=1; j<=$1;))
do
    mkdir client$j
    client=()
    pid=()

    watch -n 1 "ps -eLf | grep simple-server-cpp | grep -v grep | wc -l >> ./client$j/activethreads.txt" > /dev/null &
    watch_pid=$!
    vmstat 1 > ./client$j/vmstat.txt &
    vmstat_pid=$!
    
    for ((i=1; i<=$j; i++))
    do
        this=$(date +%s%N)
        client+=($this)
        ./$client_executable $ip:$port test.c $2 $3 $4 > ./client$j/client$this.txt &
        pid+=($!)
    done

    for ((i=0; i<$j; i++))
    do
        wait ${pid[$i]}
    done

    $(kill $watch_pid)
    $(kill $vmstat_pid)

    total_correct_response_rate=0
    total_total_time=0
    total_average_response_time=0
    total_timeout_rate=0
    total_error_rate=0

    for i in "${client[@]}";
    do 
        average_response_time_of_one_client=$(cat ./client$j/client$i.txt | cut -d ' ' -f 1)
        total_average_response_time=$(echo "$total_average_response_time + $average_response_time_of_one_client" | bc -l)
        
        correct_response_rate_of_one_client=$(cat ./client$j/client$i.txt | cut -d ' ' -f 2)
        total_correct_response_rate=$(echo "$total_correct_response_rate + $correct_response_rate_of_one_client" | bc -l)
        
        total_time_of_one_client=$(cat ./client$j/client$i.txt | cut -d ' ' -f 3)
        total_total_time=$(echo "$total_total_time + $total_time_of_one_client" | bc -l)
        
        timeout_rate_of_one_client=$(cat ./client$j/client$i.txt | cut -d ' ' -f 4)
        total_timeout_rate=$(echo "$total_timeout_rate + $timeout_rate_of_one_client" | bc -l)
        
        error_rate_of_one_client=$(cat ./client$j/client$i.txt | cut -d ' ' -f 5)
        total_error_rate=$(echo "$total_error_rate + $error_rate_of_one_client" | bc -l)
    done

    average_response_time_of_all_clients=$(echo "$total_average_response_time / $j" | bc -l)
    average_correct_response_rate_of_all_clients=$(echo "$total_correct_response_rate / $j" | bc -l)
    average_time_of_all_clients=$(echo "$total_total_time / $j" | bc -l)
    average_timeout_rate_of_all_clients=$(echo "$total_timeout_rate / $j" | bc -l)
    average_error_rate_of_all_clients=$(echo "$total_error_rate / $j" | bc -l)

    # echo $j $average_response_time_of_all_clients >> average_response_time.txt
    # echo $j $average_correct_response_rate_of_all_clients >> average_throughput.txt #goodput
    # echo $j $average_timeout_rate_of_all_clients >> average_timeout_rate.txt #timeout rate
    # echo $j $average_error_rate_of_all_clients >> average_error_rate.txt #error rate
    # request_sent_rate=$(echo "$average_correct_response_rate_of_all_clients+$average_timeout_rate_of_all_clients+$average_error_rate_of_all_clients" | bc -l)
    # echo $j $request_sent_rate >> average_request_sent_rate.txt #request sent rate

    echo $j $total_average_response_time >> average_response_time.txt
    echo $j $total_correct_response_rate >> average_throughput.txt #goodput
    echo $j $total_timeout_rate >> average_timeout_rate.txt #timeout rate
    echo $j $total_error_rate >> average_error_rate.txt #error rate
    request_sent_rate=$(echo "$total_correct_response_rate+$total_timeout_rate+$total_error_rate" | bc -l)
    echo $j $request_sent_rate >> average_request_sent_rate.txt #request sent rate


    average_active_threads=$(cat ./client$j/activethreads.txt | awk '{sum+=$1} END {print sum/NR}')
    echo $j $average_active_threads >> average_active_threads.txt 
   
    average_cpu_utilization=$(cat ./client$j/vmstat.txt | awk '{sum+=$15} END {print (((NR-2)*100)-sum)/(NR-2)}')
    echo $j $average_cpu_utilization >> average_cpu_utilization.txt

    rm -rf ./client$j

    j=$((j+1))
done

$(kill $server_pid)


# Plotting of vmstat.txt using 13th column

cat average_response_time.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg Res Time" -X "Number of Clients" -Y "Avg Res Time (in s)" -r 0.25> ./plot_response_time.png
cat average_throughput.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Throughput" -X "Number of Clients" -Y "Throughput (in res/sec)" -r 0.25> ./plot_throughput.png
cat average_timeout_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg Timeout Rate" -X "Number of Clients" -Y "Avg Timeout Rate" -r 0.25> ./plot_timeout_rate.png
cat average_error_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg Error Rate" -X "Number of Clients" -Y "Avg Error Rate" -r 0.25> ./plot_error_rate.png
cat average_request_sent_rate.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg Request Sent Rate" -X "Number of Clients" -Y "Avg Request Sent Rate" -r 0.25> ./plot_request_sent_rate.png
cat average_active_threads.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg Active Threads" -X "Number of Clients" -Y "Avg Active Threads" -r 0.25> ./plot_active_threads.png
cat average_cpu_utilization.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg CPU Utilization" -X "Number of Clients" -Y "Avg CPU Utilization" -r 0.25> ./plot_cpu_utilization.png

# rm -rf average_response_time.txt average_throughput.txt average_timeout_rate.txt average_error_rate.txt average_request_sent_rate.txt average_active_threads.txt average_cpu_utilization.txt