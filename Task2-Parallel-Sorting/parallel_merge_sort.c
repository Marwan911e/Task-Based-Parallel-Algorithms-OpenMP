#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <time.h>

// Minimum size for parallel execution
// Below this threshold, we use sequential sort for efficiency
#define PARALLEL_THRESHOLD 5000
// Maximum task depth to prevent excessive task creation overhead
#define MAX_TASK_DEPTH 5

// Function to merge two sorted subarrays (highly optimized)
void merge(int arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    // Use stack allocation for small arrays to avoid malloc overhead
    int *L, *R;
    int stack_L[1024], stack_R[1024];
    
    if (n1 <= 1024 && n2 <= 1024) {
        L = stack_L;
        R = stack_R;
    } else {
        L = (int*)malloc(n1 * sizeof(int));
        R = (int*)malloc(n2 * sizeof(int));
    }
    
    // Copy data to temporary arrays (optimized with memcpy)
    memcpy(L, &arr[left], n1 * sizeof(int));
    memcpy(R, &arr[mid + 1], n2 * sizeof(int));
    
    // Merge the temporary arrays back into arr[left..right]
    int i = 0, j = 0, k = left;
    
    // Unrolled merge loop for better performance
    while (i < n1 && j < n2) {
        // Process 4 elements at a time when possible
        if (i + 3 < n1 && j + 3 < n2) {
            arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
            arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
            arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
            arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
        } else {
            arr[k++] = (L[i] <= R[j]) ? L[i++] : R[j++];
        }
    }
    
    // Copy remaining elements using memcpy for efficiency
    if (i < n1) {
        memcpy(&arr[k], &L[i], (n1 - i) * sizeof(int));
    }
    if (j < n2) {
        memcpy(&arr[k], &R[j], (n2 - j) * sizeof(int));
    }
    
    // Free only if heap allocated
    if (n1 > 1024 || n2 > 1024) {
        free(L);
        free(R);
    }
}

// Sequential merge sort (for small arrays or when task depth is too high)
void merge_sort_sequential(int arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;
        
        // Sort first and second halves
        merge_sort_sequential(arr, left, mid);
        merge_sort_sequential(arr, mid + 1, right);
        
        // Merge the sorted halves
        merge(arr, left, mid, right);
    }
}

// Parallel merge sort using OpenMP tasks with depth control (optimized)
void merge_sort_parallel_helper(int arr[], int left, int right, int depth) {
    if (left >= right) return;
    
    int size = right - left + 1;
    
    // Use sequential sort for small arrays or when task depth is too high
    if (size < PARALLEL_THRESHOLD || depth >= MAX_TASK_DEPTH) {
        merge_sort_sequential(arr, left, right);
        return;
    }
    
    int mid = left + (right - left) / 2;
    
    // Create untied tasks for better work stealing and load balancing
    #pragma omp task shared(arr) firstprivate(left, mid, depth) untied if(depth < MAX_TASK_DEPTH - 1)
    {
        merge_sort_parallel_helper(arr, left, mid, depth + 1);
    }
    
    #pragma omp task shared(arr) firstprivate(mid, right, depth) untied if(depth < MAX_TASK_DEPTH - 1)
    {
        merge_sort_parallel_helper(arr, mid + 1, right, depth + 1);
    }
    
    // Wait for both tasks to complete before merging
    #pragma omp taskwait
    
    // Merge the sorted halves
    merge(arr, left, mid, right);
}

// Parallel merge sort using OpenMP tasks
void merge_sort_parallel(int arr[], int left, int right) {
    merge_sort_parallel_helper(arr, left, right, 0);
}

// Wrapper function to initiate parallel merge sort with optimized task creation
void parallel_merge_sort(int arr[], int size) {
    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            printf("\n----- Starting Parallel Merge Sort -----\n");
            printf("Using %d threads\n\n", omp_get_num_threads());
            
            // Use untied tasks for better work stealing
            #pragma omp task untied
            merge_sort_parallel(arr, 0, size - 1);
        }
    }
}

// Function to print array
void print_array(int arr[], int size, int max_elements) {
    int print_count = (size < max_elements) ? size : max_elements;
    
    for (int i = 0; i < print_count; i++) {
        printf("%d ", arr[i]);
    }
    
    if (size > max_elements) {
        printf("... (%d more elements)", size - max_elements);
    }
    printf("\n");
}

// Function to verify if array is sorted
int is_sorted(int arr[], int size) {
    for (int i = 0; i < size - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

// Function to generate random array
void generate_random_array(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 10000;
    }
}

int main(int argc, char *argv[]) {
    int size;
    int *arr;
    double start_time, end_time;
    
    // Get array size from command line or use default
    if (argc > 1) {
        size = atoi(argv[1]);
        if (size <= 0) {
            printf("Error: Size must be positive\n");
            return 1;
        }
    } else {
        printf("Enter array size: ");
        scanf("%d", &size);
        
        if (size <= 0) {
            printf("Error: Size must be positive\n");
            return 1;
        }
    }
    
    // Allocate memory for array
    arr = (int*)malloc(size * sizeof(int));
    if (arr == NULL) {
        printf("Error: Memory allocation failed\n");
        return 1;
    }
    
    // Seed random number generator
    srand(time(NULL));
    
    // Generate random array
    printf("Generating random array of size %d...\n", size);
    generate_random_array(arr, size);
    
    printf("\nOriginal array (first 20 elements): ");
    print_array(arr, size, 20);
    
    // Perform parallel merge sort and measure time
    start_time = omp_get_wtime();
    parallel_merge_sort(arr, size);
    end_time = omp_get_wtime();
    
    double parallel_time = end_time - start_time;
    
    printf("\n----- Sorting Complete -----\n\n");
    printf("Sorted array (first 20 elements): ");
    print_array(arr, size, 20);
    
    // Verify if array is sorted
    if (is_sorted(arr, size)) {
        printf("\n✓ Array is correctly sorted!\n");
    } else {
        printf("\n✗ Error: Array is NOT sorted correctly!\n");
    }
    
    printf("\nTime taken: %.6f seconds\n", parallel_time);
    
    // Performance comparison with sequential sort
    printf("\n----- Comparing with Sequential Sort -----\n");
    
    // Generate another random array for comparison
    int *arr_seq = (int*)malloc(size * sizeof(int));
    memcpy(arr_seq, arr, size * sizeof(int));
    generate_random_array(arr_seq, size);
    
    start_time = omp_get_wtime();
    merge_sort_sequential(arr_seq, 0, size - 1);
    end_time = omp_get_wtime();
    
    double seq_time = end_time - start_time;
    printf("Sequential sort time: %.6f seconds\n", seq_time);
    
    // Calculate and display speedup
    if (seq_time > 0 && parallel_time > 0) {
        printf("Speedup: %.2fx\n", seq_time / parallel_time);
    }
    
    // Clean up
    free(arr);
    free(arr_seq);
    
    return 0;
}

