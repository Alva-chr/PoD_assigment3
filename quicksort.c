#include "quicksort.h"
#include "pivot.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


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

	distribute_from_root(input, elements_per_process, &process_memory);

    // Local sorting
	qsort(process_memory, elements_per_process, sizeof(int), compare);

    // Global Sort Algorithm
    global_sort(&process_memory,elements_per_process,MPI_COMM_WORLD,pivot_strategy);

    // Assembling sorted lists
    MPI_Gather(output,num_values,MPI_INT,process_memory,elements_per_process,MPI_INT,root,MPI_COMM_WORLD);

    // Outputting results and checking success
    check_and_print(output,num_values, output_name);

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

void gather_on_root(&output, &process_memory, elements_per_process){
    MPI_Gather(process_memory, elements_per_process, MPI_INT, output, elements_per_process, MPI_INT, 0, MPI_COMM_WORLD);
}

int global_sort(int **elements, int n, MPI_Comm, int pivot_strategy){
    //Add MPI standard commands to use send and recv
    int rank, size;
	MPI_Comm_size(MPI_Comm, &size);
	MPI_Comm_rank(MPI_Comm, &rank);
    MPI_Request req;
    MPI_Status status;

    if (size==1){
        return n;
    }

    int pivot = select_pivot(pivot_strategy,*elements,n,MPI_Comm);

    int *v1 = malloc((pivot)*sizeof(int));
    int *v2 = malloc((n-pivot)*sizeof(int));

    for (int i = 0; i<pivot;i++) {
        v1[i] = elements[i];
    }
    for (int i = pivot;i<n;i++) {
        v2[i-pivot] = elements[i];
    }
    int len2 = n-pivot;

    int *vGot;
    int *result;
    int lengot;
    int resultLength;

    if (rank<size/2){
        //send v2
        //recieve v1

        //exchanging lengths of arrays
        MPI_Irecv(&lengot, 1, MPI_INT, size/2+rank, 10, MPI_Comm, &req);
        MPI_Send(&len2, 1, MPI_INT, size/2+rank, 10, MPI_Comm);
        MPI_Wait(&req, &status);

        vGot = malloc(lengot*sizeof(int));

        //exchange arrays
        MPI_Irecv(&vGot, lengot, MPI_INT, size/2+rank, 20, MPI_Comm, &req);
        MPI_Send(&v2, len2, MPI_INT, size/2+rank, 20, MPI_Comm);
        MPI_Wait(&req, &status);

        resultLength = pivot+lengot;

        //merge arrays using merge_ascending
        result = malloc(resultLength*sizeof(int));

        merge_ascending(*v1, pivot, *vGot, lengot, *result);
    } else {
        //send v1
        //recieve v2

        //exchanging lengths of arrays
        MPI_Irecv(&lengot, 1, MPI_INT, rank-size/2, 10, MPI_Comm, &req);
        MPI_Send(&pivot, 1, MPI_INT, rank-size/2, 10, MPI_Comm); //pivot since length of v1 = pivot
        MPI_Wait(&req, &status);

        vGot = malloc(lengot*sizeof(int));

        //exchange arrays
        MPI_Irecv(&vGot, lengot, MPI_INT, rank-size/2, 20, MPI_Comm, &req);
        MPI_Send(&v1, pivot, MPI_INT, rank-size/2, 20, MPI_Comm);
        MPI_Wait(&req, &status);

        resultLength = len2+lengot;

        //merge arrays using merge_ascending
        result = malloc(resultLength*sizeof(int));

        merge_ascending(*vGot, lengot, *v2, len2, *result);
    }

    int color = rank / (size / 2); // Determine color based on row

    MPI_Comm newcomm;
    MPI_Comm_Split(MPI_Comm, color, rank, &newcomm);

    int newLength = global_sort(**result,resultLength,newcomm,pivot_strategy);
    
    swap(elements, result);
    
    free(v1);
    free(v2);
    free(result);
    free(vGot);

    return newLength
}

void merge_ascending(int *v1, int n1, int *v2, int n2, int *result){
    int n = n1 + n2;
    int i;
    int i1 = 0;
    int i2 = 0;
    for (i; i < n; i++){
        if (v1[i1] < v2[i2]){
            result[i] = v1[i1];
            i1++;
        }
        else if(v1[i1] == v2[i2]){
            result[i] = v1[i1];
            i1++;
        }
        else{
            result[i] = v2[i2];
            i2++;
        }
    }
}

int sorted_ascending(int *elements, int n){
    int i;
    int res = 1;
    for (i=0; i<n; i++){
        if (elements[i+1] > elements[i]){
            ;
        }
        else{
            res = 0;
            printf("ERROR: Element %d with index %d is not smaller than element %d with index %d", elements[i], i, elements[i+1], i+1);
        }
    }
    return res;
}

void swap(int *e1, int *e2){
    int temp = *e1;
    *e1 = *e2;
    *e2 = temp;
}

int compare(const void *v1, const void *v2){
    int res;
    if (*v1 == *v2){
        res = 0;
    }
    else if (*v1 > *v2){
        res = 1;
    }
    else{
        res = -1;
    }
    return res;
}

int get_larger_index(int *elements, int n, int val){
    int index = 0;
    int i;
    for (i = 0; i < n; i++){
        if (elements[i] > val){
            break
        }
        index++;
    }
    return index;
}

int get_median(int *elements, int n){
    int median;
    if (n%2 == 0){ //if dividable by two
        int el1 = elements[n/2-1];
        int el2 = elements[n/2];
        median = (el1 + el2)/2;
    }
    else{
        median = elements[n/2];
    }
    return median;
}

int select_pivot(int pivot_strategy, int *process_memory, int elements_per_process, MPI_Comm communicator){

    int target_number, idx = -1;
    // Switch statement to find the right pivot strategy
    switch (pivot_strategy) {
    case 1:
        target_number = select_pivot_median_root(process_memory, elements_per_process, communicator);
        break;
    case 2:
        target_number = select_pivot_mean_median(process_memory, elements_per_process, communicator);
        break;
    case 3:
        target_number = select_pivot_medain_median(process_memory, elements_per_process, communicator);
        break;
    }

    //binary research implmented as in geeks for geeks, see refrence in report and the link below
    //https://www.geeksforgeeks.org/c/c-program-for-binary-search-recursive-and-iterative/
    int left=0,right=elements_per_process, mid;

    while (left <= right){
        // calculating mid point
        int mid = left + (right - left) / 2;

        // Check if key is present at mid
        if (arr[mid] == key){
            idx = mid;
        }

        // If key greater than arr[mid], ignore left half
        if (arr[mid] < key){
            left = mid + 1;
        }

        // If key is smaller than or equal to arr[mid],
        // ignore right half
        else{
            right = mid - 1;
        }
    }
    return idx;
}

int select_pivot_median_root(int *elements, int n, MPI_Comm communicator){
    int median = get_median(elements,n);
    MPI_Bcast(&median, 1, MPI_INT, 0, MPI_Comm);
    return median;
}

int select_pivot_mean_median(int *elements, int n, MPI_Comm communicator){
    //Add MPI standard commands to use send and recv
    int rank, size;
	MPI_Comm_size(MPI_Comm, &size);
	MPI_Comm_rank(MPI_Comm, &rank);

    //get median for my process
    int med = get_median(*elements, n);

    //initialize sum and mean_median
    int sum;
    int mean_median;

    //Use MPI_Reduce to sum all medians
    MPI_Reduce(&med, &sum, 1, MPI_INT, MPI_SUM, 0, MPI_Comm);

    if(rank==0){
        mean_median = sum/size;
    }

    return mean_median;
}

int select_pivot_median_median(int *elements, int n, MPI_Comm communicator){
	MPI_Comm_size(MPI_Comm, &size);
	MPI_Comm_rank(MPI_Comm, &rank);

    int collected_medians = malloc(size*sizeof(int));

    int process_median = get_median(elements, n);

    MPI_Gather(process_median, 1, MPI_INT, collected_medians, 1, MPI_DOUBLE,0, communicator);
    qsort(collected_medians, size, size_of(int), compare);

    return get_median(collected_medians, size);
}