#!/bin/bash

initRun=407459 #Set initRun to a (pseudo)random number (normally set to 1)
reps=3 #Number of repetitions (independent simulations)
distances=(1 3 5 7 9 11 13 15 17 19 21 23 25 27 29 31) 

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

	for distance in "${distances[@]}" 
	do
		run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=$distance --nStations=1 --frequency=2.4 --csvPath=distanceF.csv --scenario=A'"
		run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=$distance --nStations=1 --frequency=2.4 --frequency2=5 --csvPath=distanceF.csv --emlsrLinks=0,1 --scenario=B'"
		run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=$distance --nStations=1 --frequency=5 --frequency2=2.4 --csvPath=distanceF.csv  --emlsrLinks=0,1 --scenario=C'"
		run_simulation "./ns3 run 'scratch/mag --RngRun=$run --distance=$distance --nStations=1 --frequency=2.4 --frequency2=5 --frequency3=6 --csvPath=distanceF.csv  --emlsrLinks=0,1,2 --scenario=D'"
	done
done




# Poczekaj na wszystkie pozostałe procesy
wait
echo "All simulations completed."

