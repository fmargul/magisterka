#!/bin/bash

initRun=407456 #Set initRun to a (pseudo)random number (normally set to 1)
reps=3 #Number of repetitions (independent simulations)

# Maksymalna liczba równoległych symulacji
max_parallel=4
current_jobs=0
MLOns=(0 1 2 3 4)
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

	for MLOn in "${MLOns[@]}" 
	do
		run_simulation "./ns3 run 'scratch/mag3 --RngRun=$run --distance=1 --nStations=1 --MLOn=$MLOn --frequency=2.4 --frequency2=5 --csvPath=MLOn1.csv --emlsrLinks=0,1 --scenario=B'"
		run_simulation "./ns3 run 'scratch/mag3 --RngRun=$run --distance=1 --nStations=1 --MLOn=$MLOn --frequency=5 --frequency2=2.4 --csvPath=MLOn1.csv  --emlsrLinks=0,1 --scenario=C --idSLD=1'"
		run_simulation "./ns3 run 'scratch/mag3 --RngRun=$run --distance=1 --nStations=1 --MLOn=$MLOn --frequency=2.4 --frequency2=5 --frequency3=6 --csvPath=MLOn1.csv  --emlsrLinks=0,1,2 --scenario=D'"
	
	    run_simulation "./ns3 run 'scratch/mag3 --RngRun=$run --distance=27 --nStations=1 --MLOn=$MLOn --frequency=2.4 --frequency2=5 --csvPath=MLOn27.csv --emlsrLinks=0,1 --scenario=B'"
		run_simulation "./ns3 run 'scratch/mag3 --RngRun=$run --distance=27 --nStations=1 --MLOn=$MLOn --frequency=5 --frequency2=2.4 --csvPath=MLOn27.csv --emlsrLinks=0,1 --scenario=C --idSLD=1'"
		run_simulation "./ns3 run 'scratch/mag3 --RngRun=$run --distance=27 --nStations=1 --MLOn=$MLOn --frequency=2.4 --frequency2=5 --frequency3=6 --csvPath=MLOn27.csv  --emlsrLinks=0,1,2 --scenario=D'"
	done
done

# Poczekaj na wszystkie pozostałe procesy
wait
echo "All simulations completed."

