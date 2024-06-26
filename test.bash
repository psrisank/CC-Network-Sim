for k in $(seq 0 0.1 1);
do
    echo "Write ratio: $k" >> out.log
    cd ~/CC-Network-Sim/artifacts && python3 new_generator.py "$k" 0.4 128 output_trace.csv && cd .. && make -s >> out.log
done