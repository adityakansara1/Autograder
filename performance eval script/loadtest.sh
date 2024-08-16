> out_avg_res.txt
> out_thr.txt

if [ $# != 3 ]; then
    echo "Invalid Usage: loadtest.sh <numClients> <loopNum> <sleepTimeInSeconds>"
fi

ip="127.0.0.1"
port="5000"

for ((j=1; j<=$1; j++))
do
pids=()
for ((i=1; i<=$j; i++))
do
    ./simple-client-cpp $ip:$port test.c $2 $3 > client$i.txt &
    pids+=($!)    
done

for ((i=0; i<$j; i++))
do
    wait $((pids[i]))
done

correct_res=0
total_time=0
for ((i=1; i<=$j; i++))
do 
    x=$(cat client$i.txt | cut -d' ' -f 2)
    correct_res=$(echo "$correct_res + $x" | bc -l)
    y=$(cat client$i.txt | cut -d' ' -f 3)
    total_time=$(echo "$total_time + $y" | bc -l)
done

thr=$(echo "$correct_res / $total_time" | bc -l)
echo $j $thr >> out_thr.txt

avg_res=0
for ((i=1; i<=$j; i++))
do 
    x=$(cat client$i.txt | cut -d' ' -f 1)
    avg_res=$(echo "$avg_res + $x" | bc -l)
done
avg_res=$(echo "$avg_res / $j" | bc -l)
echo $j $avg_res >> out_avg_res.txt

done

cat out_thr.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Throughput" -X "Number of Clients" -Y "Throughput (in res/sec)" -r 0.25> ./plot1.png
cat out_avg_res.txt | graph -T png --bitmap-size "1400x1400" -g 3 -L "Clients vs Avg Res Time" -X "Number of Clients" -Y "Avg Res Time (in s)" -r 0.25> ./plot2.png