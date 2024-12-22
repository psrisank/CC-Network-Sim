traces=($(seq 10 1 10))

rm simulation_results.xlsx

for num in "${traces[@]}"; do
    bash run_trace.bash $num
done
