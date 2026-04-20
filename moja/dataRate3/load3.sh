#!/bin/bash

initRun=1 #Set initRun to a (pseudo)random number (normally set to 1)
reps=3 #Number of repetitions (independent simulations)
dataRates=(100 200 300 400 500 600 700 800 900 1000 1100 1200 1300 1400 1500)

# Maksymalna liczba równoległych symulacji
max_parallel=4
current_jobs=0

# Funkcja uruchamiająca symulację i śledząca liczbę aktywnych procesów
run_simulation() {
    local cmd="$1"
    echo "Running: $cmd"
    eval "$cmd &" # Uruchom w tle
    ((current_jobs++))
    if ((current_jobs >= max_parallel)); then
        wait -n # Poczekaj na zakończenie dowolnego procesu
        ((current_jobs--))
    fi
}

for ((run=$initRun;run<$initRun+$reps; run+=1))
do   
    for dataRate in "${dataRates[@]}" 
	do
        run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=0.5 --nStations=1 --frequency=6 --csvPath=load2.csv --dataRate=$dataRate --scenario=4'"
        run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=0.5 --nStations=1 --frequency=6 --csvPath=load2.csv --dataRate=$dataRate --scenario=5'"
        run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=0.5 --nStations=1 --frequency=6 --csvPath=load2.csv --dataRate=$dataRate --scenario=1'"
        run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=0.5 --nStations=1 --frequency=6 --csvPath=load2.csv --dataRate=$dataRate --scenario=2'"
        run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=0.5 --nStations=1 --frequency=6 --csvPath=load2.csv --dataRate=$dataRate --scenario=3'"
    done
done

# Poczekaj na wszystkie pozostałe procesy
wait
echo "All simulations completed."

