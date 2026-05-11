#include "quicksort.h"
#include "pivot.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


int main(int argc, char **argv) {

	if (4 != argc) { //make sure the code is used correctly
		printf("Usage: quicksort input_file output_file pivot_strategy\n");
		return 1;
	}

    //collecting input data
	char *input_name = argv[1];
	char *output_name = argv[2];
	int pivot_strategy = atoi(argv[3]); // 

	const int root = 0; //set root process
	int rank, size;

    //Setting up MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //initialize input and output for each process
	int *input = NULL;
	int *output = NULL;
	int num_values;
    

	//Only the the process with rank 0 will read in the full input file
	if(rank == root){
		if (0 > (num_values = read_input(input_name, &input))) { //read input
			perror("Couldn't read input");
			return 2;
		}
		if (NULL == (output = malloc(num_values*sizeof(int)))) { //allocate output
			perror("Couldn't allocate memory for output");
			return 2;
		}
	}

	int *process_memory; //initialize the memory for each process

	MPI_Bcast(&num_values, 1, MPI_INT, root, MPI_COMM_WORLD); //Broadcast the length of the input to all processes from the root process

    //start timer
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    //get length of each process array using distribute_from_root
	int memory_length = distribute_from_root(input, num_values, &process_memory, MPI_COMM_WORLD);

    // Local sorting
	qsort(process_memory,memory_length, sizeof(int), compare);

    // Global Sort Algorithm to get new length
    memory_length = global_sort(&process_memory,memory_length,MPI_COMM_WORLD,pivot_strategy);

    // Assembling sorted lists using gather_on_root
    gather_on_root(output, process_memory, memory_length, MPI_COMM_WORLD);

    // Outputting results and checking success
    if (rank==0) check_and_print(output,num_values, output_name);

    //Time taken for each process and using the slowest
    double my_execution_time = MPI_Wtime() - start;
	double max_execution_time = 0;

    //Take the slowest execution time
	MPI_Reduce(&my_execution_time, &max_execution_time, 1, MPI_DOUBLE, MPI_MAX, root, MPI_COMM_WORLD);
    
    //print execution time
    if(rank==0){
        printf("%f", my_execution_time);
    }

    free(process_memory); //free the process memory

    MPI_Finalize(); //end MPI
}

//Taken from assignment 2
int read_input(char *file_name, int **elements) {
	FILE *file;
	if (NULL == (file = fopen(file_name, "r"))) { //no input file exists
		perror("Couldn't open input file");
		return -1;
	}
	int num_values; //initialize number of values to then get it from the input file
	if (EOF == fscanf(file, "%d", &num_values)) {
		perror("Couldn't read element count from input file");
		return -1;
	}
	if (NULL == (*elements = malloc(num_values * sizeof(int)))) { //the memory cannot be allocated
		perror("Couldn't allocate memory for input");
		return -1;
	}
	for (int i=0; i<num_values; i++) { //read elements from the input file
		if (EOF == fscanf(file, "%d", &((*elements)[i]))) {
			perror("Couldn't read elements from input file");
			return -1;
		}
	}
	if (0 != fclose(file)){ //make sure the input file is closed
		perror("Warning: couldn't close input file");
	}
	return num_values; //return number of values in the input file
}

int check_and_print(int *elements, int n, char *file_name){
    int i; //initialize i

    FILE *fp = fopen(file_name, "w"); //open output file
	if (fp == NULL) {
        printf("Error opening file.\n");
        return -2;
    }

    for (i = 0; i < n; i++)
		fprintf(fp, "%d\t", elements[i]);
        int sorted = sorted_ascending(elements, n); //check sorting with help of sorted_ascending which outputs 1 or 0 depending on if the list is sorted
        if (sorted == 0){
			printf("Error: The list is not sorted.\n"); 
        }
        else if (sorted == 1){
            printf("List is sorted.\n");
        }
        else{
            printf("Error: Issue checking the sorting of the list.\n");
        }

    fclose(fp); //close the file

	return 0;
}

int distribute_from_root(int *all_elements, int n, int **my_elements, MPI_Comm communicator){
    //initialize MPI parameters
    int rank, size;
	MPI_Comm_size(communicator, &size);
	MPI_Comm_rank(communicator, &rank);

    //Allocate memory
    int *length_list = malloc(size*sizeof(int)); //number of elements in each process
    int *displacement_list = malloc(size*sizeof(int)); //indexes
    int sum = 0;

    for (int loop_rank = 0; loop_rank<size; loop_rank++){ //decide starting and ending indexes for each process
        int first = loop_rank*n/size;
        int last;
        if (loop_rank!=size-1) {
            last = (loop_rank+1)*n/size;
        } else {
            last = n;
        }
        length_list[loop_rank] = last-first; 
        displacement_list[loop_rank] = sum;
        sum += last-first;
    }

    int my_length = length_list[rank];
	
    //allocate elements
    *my_elements = malloc(my_length*sizeof(int));

    //Distribution using scatterv since different lengths
    MPI_Scatterv(all_elements, length_list, displacement_list, MPI_INT, *my_elements, my_length, MPI_INT, 0, communicator);

    //free allocations
    free(length_list);
    free(displacement_list);
    
    return my_length; //return the length of this process list
}

void gather_on_root(int *all_elements, int *my_elements, int local_n, MPI_Comm communicator){
    //initialize MPI
    int rank, size;
	MPI_Comm_size(communicator, &size);
	MPI_Comm_rank(communicator, &rank);

    //initialize lists
    int *receive_list = NULL;
    int *displacement_list = NULL;
    
    if (rank==0){ //allocate for root process
        receive_list = malloc(size*sizeof(int));
        displacement_list = malloc(size*sizeof(int));
    }

    MPI_Gather(&local_n, 1, MPI_INT, receive_list, 1, MPI_INT, 0, communicator); //gather the lengths to the root process in thee allocated list

    if (rank == 0) {
        int sum = 0;
        for (int i = 0; i < size; i++) {
            displacement_list[i] = sum; //set indexes
            sum += receive_list[i];
        }
    }

    //Use gatherv to gather the lists to root since they have different lenghts
    MPI_Gatherv(my_elements, local_n, MPI_INT, all_elements, receive_list, displacement_list, MPI_INT, 0, communicator);

    if (rank==0){ //free allocated memory
        free(receive_list);
        free(displacement_list);
    }
}

int global_sort(int **elements, int n, MPI_Comm communicator, int pivot_strategy){
    //Add MPI standard commands to use send and recv
    int rank, size;
	MPI_Comm_size(communicator, &size);
	MPI_Comm_rank(communicator, &rank);
    MPI_Request req;
    MPI_Status status;

    // smallest case
    if (size==1){
        return n;
    }

    // get pivot element, this is the index after the pivot element
    int pivot = select_pivot(pivot_strategy, *elements, n, communicator);

    // allocate two lists in order to split up the arry
    int *v1 = malloc((pivot)*sizeof(int));
    int *v2 = malloc((n-pivot)*sizeof(int));

    // fill these arrays
    for (int i = 0; i<pivot;i++) {
        v1[i] = (*elements)[i];
    }
    for (int i = pivot;i<n;i++) {
        v2[i-pivot] = (*elements)[i];
    }

    // get length of v2
    int len2 = n-pivot;

    //initialize variables for sorting
    int *vGot;
    int *result;
    int lengot = 0;
    int resultLength;

    if (rank<size/2){
        //send v2
        //recieve v1

        //exchanging lengths of arrays
        MPI_Irecv(&lengot, 1, MPI_INT, size/2+rank, 10, communicator, &req);
        MPI_Send(&len2, 1, MPI_INT, size/2+rank, 10, communicator);
        MPI_Wait(&req, &status);

        //allocate the array the process is recieving
        vGot = malloc(lengot*sizeof(int));

        //exchange arrays
        MPI_Irecv(vGot, lengot, MPI_INT, size/2+rank, 20, communicator, &req);
        MPI_Send(v2, len2, MPI_INT, size/2+rank, 20, communicator);
        MPI_Wait(&req, &status);

        //Get length of the merged array
        resultLength = pivot+lengot;

        result = malloc(resultLength*sizeof(int)); //allocate the result

        //merge arrays using merge_ascending
        merge_ascending(v1, pivot, vGot, lengot, result);
    } else {
        //send v1
        //recieve v2

        //exchanging lengths of arrays
        MPI_Irecv(&lengot, 1, MPI_INT, rank-size/2, 10, communicator, &req);
        MPI_Send(&pivot, 1, MPI_INT, rank-size/2, 10, communicator); //pivot since length of v1 = pivot
        MPI_Wait(&req, &status);

        //allocate the array the process is recieving
        vGot = malloc(lengot*sizeof(int));

        //exchange arrays
        MPI_Irecv(vGot, lengot, MPI_INT, rank-size/2, 20, communicator, &req);
        MPI_Send(v1, pivot, MPI_INT, rank-size/2, 20, communicator);
        MPI_Wait(&req, &status);

        //Get length of the merged array
        resultLength = len2+lengot;

        result = malloc(resultLength*sizeof(int));

        //merge arrays using merge_ascending
        merge_ascending(vGot, lengot, v2, len2, result);
    }

    int color = rank / (size / 2); // Determine color based on row

    //Split the communicator into two groups
    MPI_Comm newcomm;
    MPI_Comm_split(communicator, color, rank, &newcomm);

    //call global_sort recursively
    int newLength = global_sort(&result,resultLength,newcomm,pivot_strategy);
    
    free(*elements);

    *elements = result;
    
    free(v1);
    free(v2);
    free(vGot);

    MPI_Comm_free(&newcomm);

    return newLength;
}

void merge_ascending(int *v1, int n1, int *v2, int n2, int *result){ //result is allocated before this function
    //initialize needed parameters
    int i = 0;
    int i1 = 0;
    int i2 = 0;

    while (i1 < n1 && i2 < n2){ //if both v1 and v2 have values to add to the new list result
        if (v1[i1] < v2[i2]){ //if v1 is smaller than v2 add v1 to the list and move a step in v1
            result[i] = v1[i1];
            i1++;
        }
        else{ //otherwise add v2 to the list and move a step in v2
            result[i] = v2[i2];
            i2++;
        }
        i++; //move a step in result
    }

    while (i1 < n1){ //if v2 is all added to the list add the rest of v1
        result[i] = v1[i1];
        i1++;
        i++;
    }

    while (i2 < n2){ //if v1 is all added to the list add the rest of v2
        result[i] = v2[i2];
        i2++;
        i++;
    }
}

int sorted_ascending(int *elements, int n){
    int i;
    int res = 1;
    for (i=0; i<n-1; i++){
        //printf("\n%d\n",elements[i]);
        if (elements[i+1] > elements[i]){
            ;
        }
        else{
            res = 0;
            //printf("ERROR: Element %d with index %d is not smaller than element %d with index %d", elements[i], i, elements[i+1], i+1);
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
    return (*(int *)v1 - *(int *)v2);
}

int get_larger_index(int *elements, int n, int val){
    int index = 0;
    int i;
    for (i = 0; i < n; i++){
        if (elements[i] > val){
            break;
        }
        index++;
    }
    return index;
}

int get_median(int *elements, int n){
    int median;
    if(n == 0){
        return 0;
    }
    if (n%2 == 0){ //if dividable by two
        int el1 = elements[n/2-1];
        int el2 = elements[n/2];
        median = (int)(((long long)el1 + (long long)el2)/2);
    }
    else{
        median = elements[n/2];
    }
    return median;
}

int select_pivot(int pivot_strategy, int *process_memory, int elements_per_process, MPI_Comm communicator){

    int target_number = 50, idx = elements_per_process;
    // Switch statement to find the right pivot strategy
    switch (pivot_strategy) {
    case 1:
        target_number = select_pivot_median_root(process_memory, elements_per_process, communicator);
        break;
    case 2:
        target_number = select_pivot_mean_median(process_memory, elements_per_process, communicator);
        break;
    case 3:
        target_number = select_pivot_median_median(process_memory, elements_per_process, communicator);
        break;
    }
    int rank;
	MPI_Comm_rank(communicator, &rank);
    printf("rank %d: target_number %d\n",rank,target_number);

    //linear search
    // we did actually try to implement binary search but the target_number may not be in
    // in all process so it failed and we did not know how to solve it
    for(int i = 0; i <elements_per_process;i++){
        if(process_memory[i] >= target_number){
            idx = i;
            break;
        }
    }

    return idx;
}

int select_pivot_median_root(int *elements, int n, MPI_Comm communicator){
    int median = get_median(elements,n);
    MPI_Bcast(&median, 1, MPI_INT, 0, communicator);
    return median;
}

int select_pivot_mean_median(int *elements, int n, MPI_Comm communicator){
    //Add MPI standard commands to use send and recv
    int rank, size;
	MPI_Comm_size(communicator, &size);
	MPI_Comm_rank(communicator, &rank);

    //get median for my process
    long long med = (long long)get_median(elements, n);

    //initialize sum and mean_median
    long long sum;
    int mean_median = med;

    //Use MPI_Reduce to sum all medians
    MPI_Reduce(&med, &sum, 1, MPI_LONG_LONG, MPI_SUM, 0, communicator);

    MPI_Bcast(&sum, 1, MPI_LONG_LONG, 0, communicator);

    mean_median = (int)(sum/(long long)size);

    return mean_median;
}

int select_pivot_median_median(int *elements, int n, MPI_Comm communicator){
    //Add MPI standard commands to use send and recv
    int rank, size;
    int new_median;
	MPI_Comm_size(communicator, &size);
	MPI_Comm_rank(communicator, &rank);

    int *collected_medians = malloc(size*sizeof(int));

    int process_median = get_median(elements, n);

    MPI_Gather(&process_median, 1, MPI_INT, collected_medians, 1, MPI_INT ,0, communicator);
    if(rank == 0){
        qsort(collected_medians, size, sizeof(int), compare);
        new_median=get_median(collected_medians, size);
    }

    MPI_Bcast(&new_median, 1, MPI_INT, 0, communicator);

    return new_median;
}
