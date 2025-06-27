#!/bin/bash

initRun=1 #Set initRun to a (pseudo)random number (normally set to 1)
reps=3 #Number of repetitions (independent simulations)

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
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=1'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=2'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=3'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=4'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=5'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=6'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=7'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --distance=1 --nStations=1 --csvPath=tos.csv --scenario=8'"

    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=1'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=2'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=3'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=4'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=5'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=6'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=7'"
    run_simulation "./ns3 run 'scratch/mag5 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=tosrts.csv --scenario=8'"

done




# Poczekaj na wszystkie pozostałe procesy
wait
echo "All simulations completed."

