#!/bin/bash
#SBATCH -A uppmax2026-1-92      # Project ID
#SBATCH -N 1                    # Number of nodes
#SBATCH --ntasks=32             # INCREASED: Max MPI tasks needed for the job
#SBATCH --ntasks-per-node=32    # INCREASED: MPI tasks per node
#SBATCH --cpus-per-task=1
#SBATCH -t 00:40:00             # Time limit (hh:mm:ss)
#SBATCH --overload-allowed           # Optional: Ensures mpirun can flexibly use subsets of the allocation

module load OpenMPI/5.0.8-GCC-14.3.0

make all

INPUT_FILES=("input125000000.txt" "input250000000.txt" "input500000000.txt" "input1000000000.txt" "input2000000000.txt")
# ADDED 16 and 32 to the loop array
PROCESSES=(1 2 4 8 16 32) 
STRATEGIES=(1 2 3)
INPUT_DIR="/proj/uppmax2026-1-92/A3/inputs"
LOG_FILE="benchmark_results.txt"

# Initialize/Clear the log file
echo "Benchmark Results - $(date)" > $LOG_FILE
echo "------------------------------------------------" >> $LOG_FILE

for file in "${INPUT_FILES[@]}"; do
    echo "Processing File: $file"
    echo "FILE: $file" >> $LOG_FILE
    
    for np in "${PROCESSES[@]}"; do
        echo "  Running with $np processes..."
        
        for strategy in "${STRATEGIES[@]}"; do
            echo "    Strategy: $strategy"
            echo "PARAMS: File=$file, Procs=$np, Strategy=$strategy" >> $LOG_FILE
            
            #I do this for the sake of averaging the results
            for i in {1..2}; do
                mpirun -n $np ./quicksort "$INPUT_DIR/$file" "temp_out.txt" $strategy >> $LOG_FILE 2>&1
            done
            echo "------------------------------------" >> $LOG_FILE
        done
    done
done

echo "Benchmarks complete. Results saved to $LOG_FILE"