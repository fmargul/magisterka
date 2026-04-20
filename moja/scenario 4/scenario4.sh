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
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=1 --channelWidth1=20 --channelWidth2=20 --channelWidth3=20 --dataRate1=107 --dataRate2=107 --dataRate3=107'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=2 --channelWidth1=40 --channelWidth2=40 --channelWidth3=40  --dataRate1=203 --dataRate2=203 --dataRate3=203'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=3 --channelWidth1=20 --channelWidth2=40 --channelWidth3=80  --dataRate1=107 --dataRate2=203 --dataRate3=382'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=4 --channelWidth1=20 --channelWidth2=80 --channelWidth3=80  --dataRate1=107 --dataRate2=382 --dataRate3=382'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=5 --channelWidth1=20 --channelWidth2=80 --channelWidth3=160  --dataRate1=107 --dataRate2=382 --dataRate3=646'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=6 --channelWidth1=40 --channelWidth2=80 --channelWidth3=160 --dataRate1=203 --dataRate2=382 --dataRate3=646'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=7 --channelWidth1=40 --channelWidth2=160 --channelWidth3=160  --dataRate1=203 --dataRate2=646 --dataRate3=646'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --distance=1 --nStations=1 --csvPath=channelWidth.csv --scenario=8 --channelWidth1=40 --channelWidth2=160 --channelWidth3=320  --dataRate1=203 --dataRate2=646 --dataRate3=988'"

    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=1 --channelWidth1=20 --channelWidth2=20 --channelWidth3=20 --dataRate1=107 --dataRate2=107 --dataRate3=107'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=2 --channelWidth1=40 --channelWidth2=40 --channelWidth3=40   --dataRate1=203 --dataRate2=203 --dataRate3=203'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=3 --channelWidth1=20 --channelWidth2=40 --channelWidth3=80 --dataRate1=107 --dataRate2=203 --dataRate3=382'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=4 --channelWidth1=20 --channelWidth2=80 --channelWidth3=80 --dataRate1=107 --dataRate2=382 --dataRate3=382'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=5 --channelWidth1=20 --channelWidth2=80 --channelWidth3=160 --dataRate1=107 --dataRate2=382 --dataRate3=646'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=6 --channelWidth1=40 --channelWidth2=80 --channelWidth3=160 --dataRate1=203 --dataRate2=382 --dataRate3=646'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=7 --channelWidth1=40 --channelWidth2=160 --channelWidth3=160 --dataRate1=203 --dataRate2=646 --dataRate3=646'"
    run_simulation "./ns3 run 'scratch/mag4 --RngRun=$run --useRts=true --distance=1 --nStations=1 --csvPath=channelWidthrts.csv --scenario=8 --channelWidth1=40 --channelWidth2=320 --channelWidth3=320 --dataRate1=203 --dataRate2=646 --dataRate3=988'"

done




# Poczekaj na wszystkie pozostałe procesy
wait
echo "All simulations completed."

