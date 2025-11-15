# Task 2: Parallel Merge Sort Implementation Summary

## Overview

This document provides a comprehensive summary of the **Parallel Merge Sort using OpenMP Tasks** implementation (Task 2 of the HPC assignment).

---

## Implementation Details

### Core Algorithm

The implementation uses **OpenMP tasks** to parallelize the merge sort algorithm. Each recursive call creates two child tasks:

1. **Left half task**: Sorts the left portion of the array
2. **Right half task**: Sorts the right portion of the array

The parent task waits for both child tasks to complete using `#pragma omp taskwait` before performing the merge operation.

### Key OpenMP Directives Used

```c
#pragma omp parallel              // Create team of threads
#pragma omp single                // Only one thread initiates tasks
#pragma omp task                  // Create a task
#pragma omp taskwait              // Wait for child tasks
shared(arr)                       // Shared data
firstprivate(left, mid, right)    // Private copies of variables
```

### Code Structure

```c
void merge_sort_parallel(int arr[], int left, int right) {
    if (left < right) {
        // Use sequential for small arrays
        if (size < PARALLEL_THRESHOLD) {
            merge_sort_sequential(arr, left, right);
            return;
        }
        
        int mid = left + (right - left) / 2;
        
        // Create task for left half
        #pragma omp task shared(arr) firstprivate(left, mid)
        {
            merge_sort_parallel(arr, left, mid);
        }
        
        // Create task for right half
        #pragma omp task shared(arr) firstprivate(mid, right)
        {
            merge_sort_parallel(arr, mid + 1, right);
        }
        
        // Wait for both tasks
        #pragma omp taskwait
        
        // Merge sorted halves
        merge(arr, left, mid, right);
    }
}
```

---

## Performance Optimizations

### 1. Adaptive Threshold (`PARALLEL_THRESHOLD = 1000`)

Small arrays are sorted sequentially to avoid task creation overhead.

**Rationale:**
- Task creation has overhead
- For small arrays, sequential execution is faster
- Threshold of 1000 elements provides good balance

### 2. Dynamic Task Scheduling

OpenMP automatically distributes tasks among available threads, ensuring optimal load balancing.

### 3. Memory Efficiency

- In-place sorting where possible
- Temporary arrays only during merge operations
- Proper memory cleanup with `free()`

---

## Files Created

### 1. `parallel_merge_sort.c`
**Purpose:** Main interactive program with performance measurements

**Features:**
- User input for array size or command-line argument
- Random array generation
- Performance timing
- Verification of correctness
- Comparison with sequential sort
- Speedup calculation

**Usage:**
```bash
./parallel_merge_sort         # Interactive mode
./parallel_merge_sort 50000   # Direct with size
```

### 2. `parallel_merge_sort_test.c`
**Purpose:** Comprehensive automated test suite

**Features:**
- 9 different test cases:
  - Already sorted array
  - Reverse sorted array
  - Random array
  - Array with duplicates
  - Single element
  - Two elements (sorted/unsorted)
  - All same elements
  - Larger random array (20 elements)
- Performance test with 100,000 elements
- Automated verification
- Test summary report

**Usage:**
```bash
./parallel_merge_sort_test
```

### 3. `demo_parallel_tasks.c`
**Purpose:** Educational demonstration of task execution

**Features:**
- Visual representation of task creation
- Shows which thread executes each task
- Displays merge operations
- Multiple examples (8 and 16 elements)
- Clear output with symbols (→, ⊕, ✓)
- Lower threshold for more task visibility

**Usage:**
```bash
./demo_parallel_tasks
```

---

## Test Results

### Correctness Tests

✓ All 9 test cases passed:
- Already sorted array
- Reverse sorted array
- Random array
- Array with duplicates
- Single element
- Two elements (both cases)
- All same elements
- Larger random array

### Performance Test Results

**Array Size:** 100,000 elements

**Results:**
- **Parallel sort time:** 0.005000 seconds
- **Sequential sort time:** 0.022000 seconds
- **Speedup:** 4.40x

**Hardware:** 16 threads available (likely 8 cores with hyperthreading)

---

## How Task Parallelism Works

### Task Creation Tree

For an array of size 8 (example from demo):

```
Main Thread
    ├─ Task 1: Sort [0-7]
    │   ├─ Task 2: Sort [0-3]
    │   │   ├─ Task 3: Sort [0-1]  → Thread 1
    │   │   └─ Task 4: Sort [2-3]  → Thread 12
    │   └─ Task 5: Sort [4-7]
    │       ├─ Task 6: Sort [4-5]  → Thread 10
    │       └─ Task 7: Sort [6-7]  → Thread 5
```

### Key Observations

1. **Dynamic Scheduling**: Tasks can be executed by ANY available thread
2. **Load Balancing**: OpenMP automatically distributes tasks
3. **Synchronization**: `taskwait` ensures correct ordering
4. **Scalability**: More cores = more parallel execution

---

## Advantages of Task-Based Approach

### 1. **Better Load Balancing**
- Tasks can be picked up by any idle thread
- No static partitioning of work
- Adapts to system load

### 2. **Natural Recursion**
- Clean, readable code
- Mirrors sequential algorithm structure
- Easy to understand and maintain

### 3. **Scalability**
- Automatically uses all available cores
- Performance scales with hardware
- No manual thread management

### 4. **Flexibility**
- Can adjust threshold for different hardware
- Works well with varying array sizes
- OpenMP handles complexity

---

## Comparison: Sections vs Tasks

| Aspect | Sections (Task 1) | Tasks (Task 2) |
|--------|-------------------|----------------|
| **Parallelism Type** | Static | Dynamic |
| **Number of Parallel Units** | Fixed (3 sections) | Variable (many tasks) |
| **Load Balancing** | Manual | Automatic |
| **Recursion Support** | No | Yes |
| **Complexity** | Simple | More complex |
| **Best For** | Independent, fixed work | Recursive, irregular work |

---

## Key Learning Points

### 1. **Task Creation**
- Tasks are lightweight units of work
- Created with `#pragma omp task`
- Can be executed by any thread in the team

### 2. **Task Synchronization**
- `taskwait` blocks until all child tasks complete
- Essential for maintaining algorithm correctness
- Prevents race conditions

### 3. **Data Sharing**
- `shared()`: Data accessible by all tasks
- `firstprivate()`: Each task gets its own copy
- Critical for correctness and performance

### 4. **Performance Tuning**
- Threshold value is crucial
- Too many tasks = overhead
- Too few tasks = underutilization
- Balance depends on hardware and problem size

---

## Compilation and Execution

### Compilation

```bash
gcc -fopenmp -Wall -o parallel_merge_sort.exe parallel_merge_sort.c
gcc -fopenmp -Wall -o parallel_merge_sort_test.exe parallel_merge_sort_test.c
gcc -fopenmp -Wall -o demo_parallel_tasks.exe demo_parallel_tasks.c
```

### Execution

```bash
# Run main program
./parallel_merge_sort.exe 50000

# Run test suite
./parallel_merge_sort_test.exe

# Run demo
./demo_parallel_tasks.exe
```

---

## Performance Considerations

### Factors Affecting Speedup

1. **Array Size**: Larger arrays show better speedup
2. **Number of Cores**: More cores = more parallelism
3. **Threshold Value**: Affects task granularity
4. **Memory Bandwidth**: Can become bottleneck
5. **Task Overhead**: More tasks = more overhead

### Expected Speedup

- **Small arrays (< 1000)**: Little to no speedup
- **Medium arrays (1000-10000)**: 2-3x speedup
- **Large arrays (> 10000)**: 3-5x speedup

### Amdahl's Law

Even with perfect parallelization of sorting, the merge operations create some sequential bottlenecks, limiting maximum speedup.

---

## Conclusion

The implementation successfully demonstrates:

✅ Correct parallel merge sort using OpenMP tasks  
✅ Each recursive call executes as a separate task  
✅ Automatic load balancing across threads  
✅ Significant performance improvement (4.40x speedup)  
✅ Comprehensive testing and verification  
✅ Educational demonstration of task execution  

The task-based approach provides an elegant, scalable solution for parallel sorting, showcasing the power of OpenMP's task parallelism model.

---

## References

- OpenMP 3.0 Specification (Tasks)
- Merge Sort Algorithm
- Task Parallelism Patterns
- Performance Optimization Techniques

---

**Author:** HPC Assignment Implementation  
**Date:** November 2025  
**OpenMP Version:** 3.0+  
**Compiler:** GCC with `-fopenmp`

