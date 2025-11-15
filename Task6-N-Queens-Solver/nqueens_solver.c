/*
 * Parallel N-Queens Solver using OpenMP Tasks
 * 
 * This program solves the N-Queens problem using parallel backtracking
 * with OpenMP tasks. Each recursive call creates tasks for exploring
 * possible queen placements.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <omp.h>

// Structure to hold the board state
typedef struct {
    int n;              // Board size
    int *board;         // Array to store column position of queen in each row
    int solutions;      // Number of valid solutions found
} NQueensState;

// Function to check if placing a queen at (row, col) is safe
bool is_safe(int *board, int row, int col) {
    // Check all previous rows
    for (int i = 0; i < row; i++) {
        int queen_col = board[i];
        
        // Check if queens are in the same column
        if (queen_col == col) {
            return false;
        }
        
        // Check diagonal attacks
        // Two queens are on the same diagonal if |row1 - row2| == |col1 - col2|
        if (abs(row - i) == abs(col - queen_col)) {
            return false;
        }
    }
    return true;
}

// Print a solution board
void print_solution(int *board, int n, int solution_num) {
    printf("\n--- Solution %d ---\n", solution_num);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (board[i] == j) {
                printf("Q ");
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
}

// Recursive function to solve N-Queens using OpenMP tasks
void solve_nqueens_parallel(int *board, int row, int n, int *solution_count, 
                            bool print_solutions, int max_print, int depth) {
    // Base case: all queens placed successfully
    if (row == n) {
        int sol_num;
        #pragma omp atomic capture
        {
            (*solution_count)++;
            sol_num = *solution_count;
        }
        
        if (print_solutions && sol_num <= max_print) {
            #pragma omp critical
            {
                if (sol_num <= max_print) {
                    print_solution(board, n, sol_num);
                }
            }
        }
        return;
    }
    
    // Try placing queen in each column of current row
    for (int col = 0; col < n; col++) {
        if (is_safe(board, row, col)) {
            // Create a task for exploring this placement
            // Only create tasks at shallow depths to avoid too much overhead
            if (depth < 2 && row < n - 2) {
                #pragma omp task firstprivate(col, row) shared(solution_count)
                {
                    // Create a new board copy for this task
                    int *task_board = (int *)malloc(n * sizeof(int));
                    memcpy(task_board, board, n * sizeof(int));
                    
                    task_board[row] = col;
                    
                    // Recursively solve for next row
                    solve_nqueens_parallel(task_board, row + 1, n, solution_count, 
                                          print_solutions, max_print, depth + 1);
                    
                    free(task_board);
                }
            } else {
                // Sequential execution for deeper levels
                board[row] = col;
                solve_nqueens_parallel(board, row + 1, n, solution_count, 
                                      print_solutions, max_print, depth + 1);
            }
        }
    }
    
    // Wait for all tasks at this level to complete before backtracking
    if (depth < 2 && row < n - 2) {
        #pragma omp taskwait
    }
}

// Wrapper function to initiate parallel solving
int solve_nqueens(int n, bool print_solutions, int max_print, int num_threads) {
    int solution_count = 0;
    int *board = (int *)malloc(n * sizeof(int));
    
    if (board == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return -1;
    }
    
    // Initialize board
    for (int i = 0; i < n; i++) {
        board[i] = -1;
    }
    
    printf("\n----- Starting Parallel N-Queens Solver -----\n");
    printf("Board size: %dx%d\n", n, n);
    printf("Using %d threads\n\n", num_threads);
    
    double start_time = omp_get_wtime();
    
    // Start parallel region with task creation
    #pragma omp parallel num_threads(num_threads)
    {
        #pragma omp single
        {
            solve_nqueens_parallel(board, 0, n, &solution_count, 
                                  print_solutions, max_print, 0);
        }
    }
    
    double end_time = omp_get_wtime();
    
    free(board);
    
    printf("\n----- Results -----\n");
    printf("Total valid solutions found: %d\n", solution_count);
    printf("Time taken: %.6f seconds\n", end_time - start_time);
    
    return solution_count;
}

// Sequential version for comparison
void solve_nqueens_sequential_helper(int *board, int row, int n, int *solution_count) {
    if (row == n) {
        (*solution_count)++;
        return;
    }
    
    for (int col = 0; col < n; col++) {
        if (is_safe(board, row, col)) {
            board[row] = col;
            solve_nqueens_sequential_helper(board, row + 1, n, solution_count);
            board[row] = -1; // Backtrack
        }
    }
}

int solve_nqueens_sequential(int n) {
    int solution_count = 0;
    int *board = (int *)malloc(n * sizeof(int));
    
    if (board == NULL) {
        fprintf(stderr, "Memory allocation failed!\n");
        return -1;
    }
    
    for (int i = 0; i < n; i++) {
        board[i] = -1;
    }
    
    double start_time = omp_get_wtime();
    solve_nqueens_sequential_helper(board, 0, n, &solution_count);
    double end_time = omp_get_wtime();
    
    free(board);
    
    printf("\n----- Sequential Comparison -----\n");
    printf("Sequential solutions: %d\n", solution_count);
    printf("Sequential time: %.6f seconds\n", end_time - start_time);
    
    return solution_count;
}

int main(int argc, char *argv[]) {
    int n;
    int num_threads;
    bool compare_sequential = true;
    
    printf("========================================\n");
    printf("   Parallel N-Queens Solver (OpenMP)   \n");
    printf("========================================\n\n");
    
    // Get board size from user
    if (argc > 1) {
        n = atoi(argv[1]);
    } else {
        printf("Enter the size of the chessboard (N): ");
        if (scanf("%d", &n) != 1) {
            fprintf(stderr, "Invalid input!\n");
            return 1;
        }
    }
    
    // Validate input
    if (n < 1) {
        fprintf(stderr, "Board size must be at least 1!\n");
        return 1;
    }
    
    if (n > 15) {
        printf("\nWarning: N > 15 may take a very long time!\n");
        printf("Continue? (y/n): ");
        char choice;
        if (scanf(" %c", &choice) != 1) {
            printf("Invalid input!\n");
            return 1;
        }
        if (choice != 'y' && choice != 'Y') {
            return 0;
        }
    }
    
    // Get number of threads
    if (argc > 2) {
        num_threads = atoi(argv[2]);
    } else {
        num_threads = omp_get_max_threads();
        printf("Using %d threads (default). Enter custom number or press Enter: ", num_threads);
        
        char input[100];
        // Clear any leftover input
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            if (input[0] != '\n') {
                int custom_threads = atoi(input);
                if (custom_threads > 0) {
                    num_threads = custom_threads;
                }
            }
        }
    }
    
    // Ask if user wants to print solutions (only for small N)
    bool print_solutions = false;
    int max_print = 0;
    
    if (n <= 8) {
        printf("Do you want to print solutions? (y/n): ");
        char choice;
        if (scanf(" %c", &choice) != 1) {
            printf("Invalid input!\n");
            return 1;
        }
        if (choice == 'y' || choice == 'Y') {
            print_solutions = true;
            printf("How many solutions to print? (0 for all): ");
            if (scanf("%d", &max_print) != 1) {
                printf("Invalid input!\n");
                return 1;
            }
            if (max_print == 0) {
                max_print = 1000000; // Large number
            }
        }
    }
    
    // Solve using parallel approach
    int parallel_solutions = solve_nqueens(n, print_solutions, max_print, num_threads);
    
    if (parallel_solutions < 0) {
        return 1;
    }
    
    // Compare with sequential version for small N
    if (n <= 12 && compare_sequential) {
        int sequential_solutions = solve_nqueens_sequential(n);
        
        if (sequential_solutions >= 0) {
            if (parallel_solutions == sequential_solutions) {
                printf("\n✓ Parallel and sequential solutions match!\n");
            } else {
                printf("\n✗ WARNING: Solutions don't match! Parallel: %d, Sequential: %d\n", 
                       parallel_solutions, sequential_solutions);
            }
        }
    }
    
    printf("\n========================================\n");
    printf("Program completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}

