#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <limits.h>
#include <string.h>

// Game constants
#define BOARD_SIZE 3
#define EMPTY 0
#define PLAYER_X 1
#define PLAYER_O 2
#define MAX_DEPTH 9

// Task depth threshold - controls when to stop creating tasks
#define TASK_DEPTH_THRESHOLD 3

// Global statistics for monitoring parallel execution
typedef struct {
    long nodes_explored;
    long branches_pruned;
    int max_depth_reached;
} SearchStats;

SearchStats stats = {0, 0, 0};

// Structure to represent the game board
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    int move_count;
} GameState;

// Structure for storing move with its score
typedef struct {
    int row;
    int col;
    int score;
} Move;

// Function prototypes
void init_board(GameState *state);
void print_board(GameState *state);
int check_winner(GameState *state);
int is_board_full(GameState *state);
void make_move(GameState *state, int row, int col, int player);
void undo_move(GameState *state, int row, int col);
int minimax_sequential(GameState *state, int depth, int is_maximizing, int alpha, int beta);
int minimax_parallel(GameState *state, int depth, int is_maximizing, int alpha, int beta, int task_depth);
Move find_best_move_parallel(GameState *state, int player);
void play_game_interactive(void);
void run_automated_test(void);

// Initialize the game board
void init_board(GameState *state) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            state->board[i][j] = EMPTY;
        }
    }
    state->move_count = 0;
}

// Print the game board
void print_board(GameState *state) {
    printf("\n");
    printf("     0   1   2\n");
    printf("   +---+---+---+\n");
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        printf(" %d ", i);
        for (int j = 0; j < BOARD_SIZE; j++) {
            char symbol;
            switch (state->board[i][j]) {
                case PLAYER_X: symbol = 'X'; break;
                case PLAYER_O: symbol = 'O'; break;
                default: symbol = ' '; break;
            }
            printf("| %c ", symbol);
        }
        printf("|\n");
        printf("   +---+---+---+\n");
    }
    printf("\n");
}

// Check if there's a winner
// Returns PLAYER_X, PLAYER_O, or 0 if no winner
int check_winner(GameState *state) {
    // Check rows
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (state->board[i][0] != EMPTY &&
            state->board[i][0] == state->board[i][1] &&
            state->board[i][1] == state->board[i][2]) {
            return state->board[i][0];
        }
    }
    
    // Check columns
    for (int j = 0; j < BOARD_SIZE; j++) {
        if (state->board[0][j] != EMPTY &&
            state->board[0][j] == state->board[1][j] &&
            state->board[1][j] == state->board[2][j]) {
            return state->board[0][j];
        }
    }
    
    // Check diagonals
    if (state->board[0][0] != EMPTY &&
        state->board[0][0] == state->board[1][1] &&
        state->board[1][1] == state->board[2][2]) {
        return state->board[0][0];
    }
    
    if (state->board[0][2] != EMPTY &&
        state->board[0][2] == state->board[1][1] &&
        state->board[1][1] == state->board[2][0]) {
        return state->board[0][2];
    }
    
    return 0; // No winner
}

// Check if the board is full
int is_board_full(GameState *state) {
    return state->move_count >= BOARD_SIZE * BOARD_SIZE;
}

// Make a move on the board
void make_move(GameState *state, int row, int col, int player) {
    if (state->board[row][col] == EMPTY) {
        state->board[row][col] = player;
        state->move_count++;
    }
}

// Undo a move
void undo_move(GameState *state, int row, int col) {
    if (state->board[row][col] != EMPTY) {
        state->board[row][col] = EMPTY;
        state->move_count--;
    }
}

// Evaluate the board position
// Returns positive score for PLAYER_X advantage, negative for PLAYER_O
int evaluate_position(GameState *state) {
    int winner = check_winner(state);
    
    if (winner == PLAYER_X) {
        return 10;
    } else if (winner == PLAYER_O) {
        return -10;
    }
    
    return 0; // Draw or game in progress
}

// Sequential minimax with alpha-beta pruning
int minimax_sequential(GameState *state, int depth, int is_maximizing, int alpha, int beta) {
    // Update statistics
    #pragma omp atomic
    stats.nodes_explored++;
    
    #pragma omp critical
    {
        if (depth > stats.max_depth_reached) {
            stats.max_depth_reached = depth;
        }
    }
    
    int winner = check_winner(state);
    
    // Terminal conditions
    if (winner != 0) {
        int score = evaluate_position(state);
        // Adjust score by depth to prefer faster wins
        return score - (is_maximizing ? depth : -depth);
    }
    
    if (is_board_full(state)) {
        return 0; // Draw
    }
    
    if (is_maximizing) {
        int max_eval = INT_MIN;
        
        // Try all possible moves
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (state->board[i][j] == EMPTY) {
                    make_move(state, i, j, PLAYER_X);
                    int eval = minimax_sequential(state, depth + 1, 0, alpha, beta);
                    undo_move(state, i, j);
                    
                    max_eval = (eval > max_eval) ? eval : max_eval;
                    alpha = (alpha > eval) ? alpha : eval;
                    
                    // Alpha-beta pruning
                    if (beta <= alpha) {
                        #pragma omp atomic
                        stats.branches_pruned++;
                        return max_eval;
                    }
                }
            }
        }
        return max_eval;
    } else {
        int min_eval = INT_MAX;
        
        // Try all possible moves
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (state->board[i][j] == EMPTY) {
                    make_move(state, i, j, PLAYER_O);
                    int eval = minimax_sequential(state, depth + 1, 1, alpha, beta);
                    undo_move(state, i, j);
                    
                    min_eval = (eval < min_eval) ? eval : min_eval;
                    beta = (beta < eval) ? beta : eval;
                    
                    // Alpha-beta pruning
                    if (beta <= alpha) {
                        #pragma omp atomic
                        stats.branches_pruned++;
                        return min_eval;
                    }
                }
            }
        }
        return min_eval;
    }
}

// Parallel minimax with alpha-beta pruning and task-based exploration
int minimax_parallel(GameState *state, int depth, int is_maximizing, int alpha, int beta, int task_depth) {
    // Update statistics
    #pragma omp atomic
    stats.nodes_explored++;
    
    #pragma omp critical
    {
        if (depth > stats.max_depth_reached) {
            stats.max_depth_reached = depth;
        }
    }
    
    int winner = check_winner(state);
    
    // Terminal conditions
    if (winner != 0) {
        int score = evaluate_position(state);
        return score - (is_maximizing ? depth : -depth);
    }
    
    if (is_board_full(state)) {
        return 0; // Draw
    }
    
    // Switch to sequential if task depth is too high
    if (task_depth >= TASK_DEPTH_THRESHOLD) {
        return minimax_sequential(state, depth, is_maximizing, alpha, beta);
    }
    
    if (is_maximizing) {
        int max_eval = INT_MIN;
        
        // Collect all possible moves
        Move moves[BOARD_SIZE * BOARD_SIZE];
        int move_count = 0;
        
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (state->board[i][j] == EMPTY) {
                    moves[move_count].row = i;
                    moves[move_count].col = j;
                    moves[move_count].score = INT_MIN;
                    move_count++;
                }
            }
        }
        
        // Create tasks for each possible move
        for (int m = 0; m < move_count; m++) {
            #pragma omp task shared(state, moves) firstprivate(m, depth, alpha, beta, task_depth)
            {
                // Create a local copy of the state for this task
                GameState local_state = *state;
                
                int row = moves[m].row;
                int col = moves[m].col;
                
                // Only print for first move (depth 0)
                if (depth == 0) {
                    printf("Thread %d: Exploring move (%d,%d) at depth %d\n", 
                           omp_get_thread_num(), row, col, depth);
                }
                
                make_move(&local_state, row, col, PLAYER_X);
                int eval = minimax_parallel(&local_state, depth + 1, 0, alpha, beta, task_depth + 1);
                
                // Store the result
                moves[m].score = eval;
            }
        }
        
        // Wait for all tasks to complete
        #pragma omp taskwait
        
        // Find the maximum evaluation
        for (int m = 0; m < move_count; m++) {
            if (moves[m].score > max_eval) {
                max_eval = moves[m].score;
            }
            if (moves[m].score > alpha) {
                alpha = moves[m].score;
            }
        }
        
        return max_eval;
    } else {
        int min_eval = INT_MAX;
        
        // Collect all possible moves
        Move moves[BOARD_SIZE * BOARD_SIZE];
        int move_count = 0;
        
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (state->board[i][j] == EMPTY) {
                    moves[move_count].row = i;
                    moves[move_count].col = j;
                    moves[move_count].score = INT_MAX;
                    move_count++;
                }
            }
        }
        
        // Create tasks for each possible move
        for (int m = 0; m < move_count; m++) {
            #pragma omp task shared(state, moves) firstprivate(m, depth, alpha, beta, task_depth)
            {
                // Create a local copy of the state for this task
                GameState local_state = *state;
                
                int row = moves[m].row;
                int col = moves[m].col;
                
                // Only print for first move (depth 0)
                if (depth == 0) {
                    printf("Thread %d: Exploring move (%d,%d) at depth %d\n", 
                           omp_get_thread_num(), row, col, depth);
                }
                
                make_move(&local_state, row, col, PLAYER_O);
                int eval = minimax_parallel(&local_state, depth + 1, 1, alpha, beta, task_depth + 1);
                
                // Store the result
                moves[m].score = eval;
            }
        }
        
        // Wait for all tasks to complete
        #pragma omp taskwait
        
        // Find the minimum evaluation
        for (int m = 0; m < move_count; m++) {
            if (moves[m].score < min_eval) {
                min_eval = moves[m].score;
            }
            if (moves[m].score < beta) {
                beta = moves[m].score;
            }
        }
        
        return min_eval;
    }
}

// Find the best move using parallel search
Move find_best_move_parallel(GameState *state, int player) {
    Move best_move;
    best_move.row = -1;
    best_move.col = -1;
    best_move.score = (player == PLAYER_X) ? INT_MIN : INT_MAX;
    
    // Reset statistics
    stats.nodes_explored = 0;
    stats.branches_pruned = 0;
    stats.max_depth_reached = 0;
    
    double start_time = omp_get_wtime();
    
    printf("\n----- Starting Parallel Game Tree Search -----\n");
    
    #pragma omp parallel
    {
        #pragma omp single
        {
            printf("Using %d threads for parallel exploration\n\n", omp_get_num_threads());
            
            // Collect all possible moves
            Move moves[BOARD_SIZE * BOARD_SIZE];
            int move_count = 0;
            
            for (int i = 0; i < BOARD_SIZE; i++) {
                for (int j = 0; j < BOARD_SIZE; j++) {
                    if (state->board[i][j] == EMPTY) {
                        moves[move_count].row = i;
                        moves[move_count].col = j;
                        moves[move_count].score = (player == PLAYER_X) ? INT_MIN : INT_MAX;
                        move_count++;
                    }
                }
            }
            
            // Create task for each possible move
            for (int m = 0; m < move_count; m++) {
                #pragma omp task shared(state, moves) firstprivate(m, player)
                {
                    GameState local_state = *state;
                    int row = moves[m].row;
                    int col = moves[m].col;
                    
                    printf("Thread %d: Evaluating root move (%d,%d)\n", 
                           omp_get_thread_num(), row, col);
                    
                    make_move(&local_state, row, col, player);
                    
                    int eval;
                    if (player == PLAYER_X) {
                        eval = minimax_parallel(&local_state, 1, 0, INT_MIN, INT_MAX, 1);
                    } else {
                        eval = minimax_parallel(&local_state, 1, 1, INT_MIN, INT_MAX, 1);
                    }
                    
                    moves[m].score = eval;
                    
                    printf("Thread %d: Move (%d,%d) has score %d\n", 
                           omp_get_thread_num(), row, col, eval);
                }
            }
            
            #pragma omp taskwait
            
            // Find the best move from all evaluated moves
            for (int m = 0; m < move_count; m++) {
                if (player == PLAYER_X) {
                    if (moves[m].score > best_move.score) {
                        best_move = moves[m];
                    }
                } else {
                    if (moves[m].score < best_move.score) {
                        best_move = moves[m];
                    }
                }
            }
        }
    }
    
    double end_time = omp_get_wtime();
    
    printf("\n----- Search Complete -----\n");
    printf("Best move: (%d, %d) with score %d\n", best_move.row, best_move.col, best_move.score);
    printf("Nodes explored: %ld\n", stats.nodes_explored);
    printf("Branches pruned: %ld\n", stats.branches_pruned);
    printf("Max depth reached: %d\n", stats.max_depth_reached);
    printf("Time taken: %.6f seconds\n\n", end_time - start_time);
    
    return best_move;
}

// Interactive game mode
void play_game_interactive(void) {
    GameState state;
    init_board(&state);
    
    printf("\n===== Tic-Tac-Toe with Parallel Minimax AI =====\n");
    printf("You are Player O, AI is Player X\n");
    printf("The AI will use parallel game tree search to find optimal moves.\n");
    
    int current_player = PLAYER_X; // AI starts
    
    while (1) {
        print_board(&state);
        
        int winner = check_winner(&state);
        if (winner != 0) {
            if (winner == PLAYER_X) {
                printf("AI (X) wins!\n");
            } else {
                printf("You (O) win!\n");
            }
            break;
        }
        
        if (is_board_full(&state)) {
            printf("It's a draw!\n");
            break;
        }
        
        if (current_player == PLAYER_X) {
            printf("AI's turn (X)...\n");
            Move best_move = find_best_move_parallel(&state, PLAYER_X);
            make_move(&state, best_move.row, best_move.col, PLAYER_X);
            printf("AI places X at (%d, %d)\n", best_move.row, best_move.col);
            current_player = PLAYER_O;
        } else {
            printf("Your turn (O)\n");
            int row, col;
            int valid_move = 0;
            
            while (!valid_move) {
                printf("Enter row (0-2): ");
                if (scanf("%d", &row) != 1) {
                    // Clear invalid input
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF);
                    printf("Invalid input! Try again.\n");
                    continue;
                }
                printf("Enter column (0-2): ");
                if (scanf("%d", &col) != 1) {
                    // Clear invalid input
                    int c;
                    while ((c = getchar()) != '\n' && c != EOF);
                    printf("Invalid input! Try again.\n");
                    continue;
                }
                
                if (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE) {
                    if (state.board[row][col] == EMPTY) {
                        valid_move = 1;
                    } else {
                        printf("That position is already taken! Try again.\n");
                    }
                } else {
                    printf("Invalid coordinates! Enter values between 0 and 2.\n");
                }
            }
            
            make_move(&state, row, col, PLAYER_O);
            current_player = PLAYER_X;
        }
    }
    
    print_board(&state);
}

// Run automated test scenarios
void run_automated_test(void) {
    printf("\n===== Automated Test: Parallel Game Tree Search =====\n\n");
    
    // Test 1: Find winning move
    printf("--- Test 1: AI should find immediate winning move ---\n");
    GameState state1;
    init_board(&state1);
    state1.board[0][0] = PLAYER_X;
    state1.board[0][1] = PLAYER_X;
    state1.board[1][0] = PLAYER_O;
    state1.board[1][1] = PLAYER_O;
    state1.move_count = 4;
    
    print_board(&state1);
    Move move1 = find_best_move_parallel(&state1, PLAYER_X);
    printf("Expected: (0,2), Got: (%d,%d)\n", move1.row, move1.col);
    printf("Test 1: %s\n\n", (move1.row == 0 && move1.col == 2) ? "PASSED" : "FAILED");
    
    // Test 2: Block opponent's winning move
    printf("--- Test 2: AI should block opponent's winning move ---\n");
    GameState state2;
    init_board(&state2);
    state2.board[0][0] = PLAYER_O;
    state2.board[0][1] = PLAYER_O;
    state2.board[1][1] = PLAYER_X;
    state2.move_count = 3;
    
    print_board(&state2);
    Move move2 = find_best_move_parallel(&state2, PLAYER_X);
    printf("Expected: (0,2), Got: (%d,%d)\n", move2.row, move2.col);
    printf("Test 2: %s\n\n", (move2.row == 0 && move2.col == 2) ? "PASSED" : "FAILED");
    
    // Test 3: Center position on empty board
    printf("--- Test 3: AI should prefer center on empty board ---\n");
    GameState state3;
    init_board(&state3);
    
    print_board(&state3);
    Move move3 = find_best_move_parallel(&state3, PLAYER_X);
    printf("Expected: (1,1), Got: (%d,%d)\n", move3.row, move3.col);
    printf("Test 3: %s\n\n", (move3.row == 1 && move3.col == 1) ? "PASSED" : "FAILED");
}

int main(int argc, char *argv[]) {
    int mode = 0;
    
    if (argc > 1) {
        if (strcmp(argv[1], "test") == 0) {
            mode = 1;
        } else if (strcmp(argv[1], "play") == 0) {
            mode = 2;
        }
    } else {
        printf("Select mode:\n");
        printf("1. Run automated tests\n");
        printf("2. Play against AI\n");
        printf("Enter choice (1 or 2): ");
        if (scanf("%d", &mode) != 1) {
            printf("Invalid input!\n");
            return 1;
        }
    }
    
    if (mode == 1) {
        run_automated_test();
    } else if (mode == 2) {
        play_game_interactive();
    } else {
        printf("Invalid choice!\n");
        return 1;
    }
    
    return 0;
}

