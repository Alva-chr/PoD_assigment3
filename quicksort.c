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

	int *input = NULL; //So that each process 
	int *output = NULL;
	int num_values;

	//Only the the process with rank 0 will read in the full input file
	if(rank == root){
		if (0 > (num_values = read_input(input_name, &input))) {
			perror("Couldn't read input");
			return 2;
		}
		if (NULL == (output = malloc(num_values*sizeof(int)))) {
			perror("Couldn't allocate memory for output");
			return 2;
		}
	}

	int elements_per_process = num_values/size;
	int *process_memory;

    //Setting up MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	MPI_Bcast(&elements_per_process, 1, MPI_INT, root, MPI_COMM_WORLD);

	//Allocating memory for each process
	if (NULL == (process_memory = malloc(elements_per_process* sizeof(int)))) {
		perror("Couldn't allocate memory for process memory");
		return 2;
	}

	distribute_from_root(input, elements_per_process, process_memory);

	MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    //lägg till kod för att lösa problemet här


    //Time taken for each process and using the slowest
    double my_execution_time = MPI_Wtime() - start;
	double max_execution_time = 0;


	MPI_Reduce(&my_execution_time, &max_execution_time, 1, MPI_DOUBLE, MPI_MAX, root, MPI_COMM_WORLD);


    MPI_Finalize(); 
}

//Taken from assignment 2
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

int check_and_print(int *elements, int n, char *file_name){
    bool sorted = true;

    FILE *fp = fopen(file_name, "w");
	if (fp == NULL) {
        printf("Error opening file.\n");
        return -2;
    }

    for (int i = 1; i < n; i++)
		fprintf(fp, "%d\t", data[i]);
        if (elements[i - 1] > elements[i])
			printf("Error: The list is not sorted.\n");
            sorted =  false;

    fclose(fp);

	return 0
}

int distribute_from_root(int *all_elements, int n, int **my_elements){
	MPI_Scatter(input, elements_per_process, MPI_INT, process_memory, elements_per_process, MPI_INT, 0, MPI_COMM_WORLD);
}

void gather_on_root(int *all_elements, int *my_elements, int local_n){

}

int global_sort(int **elements, int n, MPI_Comm, int pivot_strategy){

}

void merge_ascending(int *v1, int n1, int *v2, int n2, int *result){

}

int read_input(char *file_name, int **elements){

}

int sorted_ascending(int *elements, int n){

}

void swap(int *e1, int *e2){

}

int compare(const void *v1, const void *v2){

}

int get_larger_index(int *elements, int n, int val){

}

int get_median(int *elements, int n){

}

int select_pivot(int pivot_strategy, int *elements, int n, MPI_Comm communicator){

}

int select_pivot_median_root(int *elements, int n, MPI_Comm communicator){

}

int select_pivot_mean_median(int *elements, int n, MPI_Comm communicator){

}

int select_pivot_median_median(int *elements, int n, MPI_Comm communicator){

}

int select_pivot_smallest_root(int *elements, int n, MPI_Comm communicator){
	
}