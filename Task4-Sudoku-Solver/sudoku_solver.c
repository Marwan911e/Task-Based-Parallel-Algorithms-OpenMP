#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <time.h>

#define N 9
#define UNASSIGNED 0

// Global variables for solution tracking
int solution_found = 0;
int solution_board[N][N];
omp_lock_t solution_lock;

// Function to print the Sudoku board
void print_board(int board[N][N]) {
    printf("\n");
    for (int i = 0; i < N; i++) {
        if (i % 3 == 0 && i != 0) {
            printf("------+-------+------\n");
        }
        for (int j = 0; j < N; j++) {
            if (j % 3 == 0 && j != 0) {
                printf("| ");
            }
            if (board[i][j] == 0) {
                printf(". ");
            } else {
                printf("%d ", board[i][j]);
            }
        }
        printf("\n");
    }
    printf("\n");
}

// Check if it's safe to place a number at board[row][col]
int is_safe(int board[N][N], int row, int col, int num) {
    // Check row
    for (int x = 0; x < N; x++) {
        if (board[row][x] == num) {
            return 0;
        }
    }
    
    // Check column
    for (int x = 0; x < N; x++) {
        if (board[x][col] == num) {
            return 0;
        }
    }
    
    // Check 3x3 box
    int start_row = row - row % 3;
    int start_col = col - col % 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i + start_row][j + start_col] == num) {
                return 0;
            }
        }
    }
    
    return 1;
}

// Find an unassigned location in the board
int find_unassigned(int board[N][N], int *row, int *col) {
    for (*row = 0; *row < N; (*row)++) {
        for (*col = 0; *col < N; (*col)++) {
            if (board[*row][*col] == UNASSIGNED) {
                return 1;
            }
        }
    }
    return 0;
}

// Copy board contents
void copy_board(int dest[N][N], int src[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            dest[i][j] = src[i][j];
        }
    }
}

// Pure sequential Sudoku solver (for comparison - no parallel checks)
int solve_sudoku_sequential_pure(int board[N][N]) {
    int row, col;
    
    // If no unassigned location, puzzle is solved
    if (!find_unassigned(board, &row, &col)) {
        return 1;
    }
    
    // Try digits 1 to 9
    for (int num = 1; num <= 9; num++) {
        if (is_safe(board, row, col, num)) {
            board[row][col] = num;
            
            if (solve_sudoku_sequential_pure(board)) {
                return 1;
            }
            
            // Backtrack
            board[row][col] = UNASSIGNED;
        }
    }
    
    return 0;
}

// Sequential solver with early termination (for use in parallel context)
int solve_sudoku_sequential(int board[N][N]) {
    int row, col;
    
    // Check if another thread found solution (when called from parallel context)
    #pragma omp flush(solution_found)
    if (solution_found) {
        return 0;
    }
    
    // If no unassigned location, puzzle is solved
    if (!find_unassigned(board, &row, &col)) {
        return 1;
    }
    
    // Try digits 1 to 9
    for (int num = 1; num <= 9; num++) {
        // Early termination if solution found by another thread
        #pragma omp flush(solution_found)
        if (solution_found) {
            return 0;
        }
        
        if (is_safe(board, row, col, num)) {
            board[row][col] = num;
            
            if (solve_sudoku_sequential(board)) {
                return 1;
            }
            
            // Backtrack
            board[row][col] = UNASSIGNED;
        }
    }
    
    return 0;
}

// Parallel Sudoku solver using OpenMP tasks
void solve_sudoku_parallel(int board[N][N], int depth) {
    // Check if solution already found
    int found;
    #pragma omp atomic read
    found = solution_found;
    if (found) {
        return;
    }
    
    int row, col;
    
    // If no unassigned location, we found a solution
    if (!find_unassigned(board, &row, &col)) {
        omp_set_lock(&solution_lock);
        if (!solution_found) {
            solution_found = 1;
            copy_board(solution_board, board);
        }
        omp_unset_lock(&solution_lock);
        return;
    }
    
    // Try each number from 1 to 9
    for (int num = 1; num <= 9; num++) {
        // Quick check without lock
        #pragma omp atomic read
        found = solution_found;
        if (found) {
            return;
        }
        
        if (is_safe(board, row, col, num)) {
            // Only parallelize at the very first level to minimize overhead
            if (depth == 0) {
                // Create a new board for this branch (only at top level)
                int new_board[N][N];
                copy_board(new_board, board);
                new_board[row][col] = num;
                
                #pragma omp task firstprivate(new_board, num)
                {
                    int local_found;
                    #pragma omp atomic read
                    local_found = solution_found;
                    if (!local_found && solve_sudoku_sequential(new_board)) {
                        omp_set_lock(&solution_lock);
                        if (!solution_found) {
                            solution_found = 1;
                            copy_board(solution_board, new_board);
                        }
                        omp_unset_lock(&solution_lock);
                    }
                }
            } else {
                // For all other depths, use sequential solving (more efficient)
                board[row][col] = num;
                
                if (solve_sudoku_sequential(board)) {
                    omp_set_lock(&solution_lock);
                    if (!solution_found) {
                        solution_found = 1;
                        copy_board(solution_board, board);
                    }
                    omp_unset_lock(&solution_lock);
                    return;
                }
                
                // Backtrack
                board[row][col] = UNASSIGNED;
            }
        }
    }
    
    // Wait for all tasks to complete only at top level
    if (depth == 0) {
        #pragma omp taskwait
    }
}

// Wrapper function to start parallel solving
int solve_sudoku_parallel_wrapper(int board[N][N]) {
    solution_found = 0;
    omp_init_lock(&solution_lock);
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            solve_sudoku_parallel(board, 0);
        }
    }
    
    omp_destroy_lock(&solution_lock);
    
    if (solution_found) {
        copy_board(board, solution_board);
        return 1;
    }
    
    return 0;
}

// Validate if the board configuration is valid
int is_valid_board(int board[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (board[i][j] != 0) {
                int num = board[i][j];
                board[i][j] = 0; // Temporarily remove to check
                if (!is_safe(board, i, j, num)) {
                    board[i][j] = num; // Restore
                    return 0;
                }
                board[i][j] = num; // Restore
            }
        }
    }
    return 1;
}

// Read Sudoku board from user input
void read_board(int board[N][N]) {
    printf("Enter the Sudoku puzzle (use 0 for empty cells):\n");
    printf("Enter 9 numbers per row, separated by spaces:\n\n");
    
    for (int i = 0; i < N; i++) {
        printf("Row %d: ", i + 1);
        for (int j = 0; j < N; j++) {
            if (scanf("%d", &board[i][j]) != 1) {
                printf("Invalid input! Please enter a number.\n");
                exit(1);
            }
            if (board[i][j] < 0 || board[i][j] > 9) {
                printf("Invalid input! Numbers must be 0-9.\n");
                exit(1);
            }
        }
    }
}

int main() {
    int board[N][N];
    int board_copy[N][N];
    double start_time, end_time;
    
    printf("========================================\n");
    printf("    Parallel Sudoku Solver (OpenMP)    \n");
    printf("========================================\n\n");
    
    printf("Choose input method:\n");
    printf("1. Enter puzzle manually\n");
    printf("2. Use sample puzzle (easy)\n");
    printf("3. Use hard puzzle\n");
    printf("Enter choice: ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        choice = 2; // Default to easy puzzle
    }
    
    if (choice == 1) {
        read_board(board);
    } else if (choice == 3) {
        // Hard Sudoku puzzle (fewer clues, more backtracking)
        int hard_sample[N][N] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 3, 0, 8, 5},
            {0, 0, 1, 0, 2, 0, 0, 0, 0},
            {0, 0, 0, 5, 0, 7, 0, 0, 0},
            {0, 0, 4, 0, 0, 0, 1, 0, 0},
            {0, 9, 0, 0, 0, 0, 0, 0, 0},
            {5, 0, 0, 0, 0, 0, 0, 7, 3},
            {0, 0, 2, 0, 1, 0, 0, 0, 0},
            {0, 0, 0, 0, 4, 0, 0, 0, 9}
        };
        copy_board(board, hard_sample);
        printf("\nUsing hard puzzle:\n");
    } else {
        // Sample Sudoku puzzle (easy-medium difficulty)
        int sample[N][N] = {
            {5, 3, 0, 0, 7, 0, 0, 0, 0},
            {6, 0, 0, 1, 9, 5, 0, 0, 0},
            {0, 9, 8, 0, 0, 0, 0, 6, 0},
            {8, 0, 0, 0, 6, 0, 0, 0, 3},
            {4, 0, 0, 8, 0, 3, 0, 0, 1},
            {7, 0, 0, 0, 2, 0, 0, 0, 6},
            {0, 6, 0, 0, 0, 0, 2, 8, 0},
            {0, 0, 0, 4, 1, 9, 0, 0, 5},
            {0, 0, 0, 0, 8, 0, 0, 7, 9}
        };
        copy_board(board, sample);
        printf("\nUsing sample puzzle:\n");
    }
    
    printf("\nInitial Sudoku Board:");
    print_board(board);
    
    // Validate initial board
    if (!is_valid_board(board)) {
        printf("Error: The initial board configuration is invalid!\n");
        return 1;
    }
    
    // Make a copy for sequential solving
    copy_board(board_copy, board);
    
    // Solve using parallel method
    printf("Solving using parallel method...\n");
    start_time = omp_get_wtime();
    int parallel_result = solve_sudoku_parallel_wrapper(board);
    end_time = omp_get_wtime();
    double parallel_time = end_time - start_time;
    
    if (parallel_result) {
        printf("\n----- Solution Found (Parallel) -----");
        print_board(board);
        printf("Time taken (parallel): %.6f seconds\n", parallel_time);
    } else {
        printf("\nNo solution exists for this puzzle.\n");
    }
    
    // Compare with sequential solving
    printf("\n----- Comparing with Sequential Solver -----\n");
    printf("Solving using sequential method...\n");
    
    start_time = omp_get_wtime();
    int sequential_result = solve_sudoku_sequential_pure(board_copy);
    end_time = omp_get_wtime();
    double sequential_time = end_time - start_time;
    
    if (sequential_result) {
        printf("Sequential solver completed successfully.\n");
        printf("Time taken (sequential): %.6f seconds\n", sequential_time);
        
        if (parallel_time > 0) {
            double speedup = sequential_time / parallel_time;
            printf("\nSpeedup: %.2fx", speedup);
            if (speedup > 1.0) {
                printf(" (Parallel is %.1f%% faster)\n", (speedup - 1.0) * 100);
            } else {
                printf(" (Sequential is %.1f%% faster)\n", (1.0/speedup - 1.0) * 100);
            }
        }
    } else {
        printf("Sequential solver: No solution exists.\n");
    }
    
    return 0;
}

