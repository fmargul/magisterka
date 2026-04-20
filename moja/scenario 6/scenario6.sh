#!/bin/bash

initRun=3000 #Set initRun to a (pseudo)random number (normally set to 1)
reps=3 #Number of repetitions (independent simulations)
switches=(5 10 15 20 25 30 35 40 45 50 55 60 65 70 75 80 85 90 95 100 105 110 115 120 125)

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

    for switch in "${switches[@]}" 
    do
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=32 --emlsrTransitionDelay=32'"
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=128 --emlsrTransitionDelay=32'"
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=0 --emlsrTransitionDelay=32'"
	done

    for switch in "${switches[@]}" 
    do
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=32 --emlsrTransitionDelay=256'"
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=128 --emlsrTransitionDelay=256'"
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=0 --emlsrTransitionDelay=256'"
	done
    for switch in "${switches[@]}" 
    do
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch'"
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=128'"
        run_simulation "./ns3 run 'scratch/mag6 --RngRun=$run --distance=1 --nStations=1 --csvPath=switch.csv --dataRate=107 --channelSwitchDelay=$switch --emlsrPaddingDelay=0'"
    done
done

# Poczekaj na wszystkie pozostałe procesy
wait
echo "All simulations completed."

