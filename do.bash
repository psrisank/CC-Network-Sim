echo "" > out.log
for i in $(seq 0 0.2 1);
do
    echo "----------------------------------------------------------------------------------------------------------------------------------------------------------------" >> out.log
    echo  "Sharing Ratio: $i" >> out.log
    for k in $(seq 0 0.1 1);
    do
        echo "Write ratio: $k" >> out.log
        cd ~/CC-Network-Sim/artifacts && python3 new_generator.py "$k" "$i" 128 output_trace.csv && cd .. && make -s >> out.log
    done
done