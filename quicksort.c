#include "quicksort.h"
#include "pivot.h"
#include <string.h>
#include <stdio.h>


int main(int argc, char **argv) {

	if (4 != argc) {
		printf("Usage: quicksort input_file output_file pivot_strategy\n");
		return 1;
	}

    //collecting input data
	char *input_name = argv[1];
	char *output_name = argv[2];
	int pivot_strategy = atoi(argv[3]); // 

	const int root = 0;
	int rank, size;
	MPI_Status status;

    //Setting up MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    double start = MPI_Wtime();

    //lägg till kod för att lösa problemet här


    //Time taken for each process and using the slowest
    double my_execution_time = MPI_Wtime() - start;
	double max_execution_time = 0;


	MPI_Reduce(&my_execution_time, &max_execution_time, 1, MPI_DOUBLE, MPI_MAX, root, MPI_COMM_WORLD);


    MPI_Finalize(); 
}

int read_input(const char *file_name, double **values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "r"))) {
		perror("Couldn't open input file");
		return -1;
	}
	int num_values;
	if (EOF == fscanf(file, "%d", &num_values)) {
		perror("Couldn't read element count from input file");
		return -1;
	}
	if (NULL == (*values = malloc(num_values * sizeof(double)))) {
		perror("Couldn't allocate memory for input");
		return -1;
	}
	for (int i=0; i<num_values; i++) {
		if (EOF == fscanf(file, "%lf", &((*values)[i]))) {
			perror("Couldn't read elements from input file");
			return -1;
		}
	}
	if (0 != fclose(file)){
		perror("Warning: couldn't close input file");
	}
	return num_values;
}


int write_output(char *file_name, const double *output, int num_values) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "w"))) {
		perror("Couldn't open output file");
		return -1;
	}
	for (int i = 0; i < num_values; i++) {
		if (0 > fprintf(file, "%.4f ", output[i])) {
			perror("Couldn't write to output file");
		}
	}
	if (0 > fprintf(file, "\n")) {
		perror("Couldn't write to output file");
	}
	if (0 != fclose(file)) {
		perror("Warning: couldn't close output file");
	}
	return 0;
}