# Makefile for OpenMP Assignment - Sheet #02
# Optimized Compilation for All Tasks

CC = gcc
# Optimization flags for maximum performance
CFLAGS = -fopenmp -O3 -march=native -funroll-loops -finline-functions -Wall
# Basic flags without aggressive optimizations (for compatibility)
CFLAGS_BASIC = -fopenmp -O2 -Wall

# Task directories
TASK1_DIR = Task1-Logical-Evaluation
TASK2_DIR = Task2-Parallel-Sorting
TASK3_DIR = Task3-File-Compressor
TASK4_DIR = Task4-Sudoku-Solver
TASK5_DIR = Task5-Game-Tree-Search
TASK6_DIR = Task6-N-Queens-Solver

# Executables (Windows .exe extension)
TARGET1 = $(TASK1_DIR)/logical_evaluation.exe
TARGET2 = $(TASK2_DIR)/parallel_merge_sort.exe
TARGET3 = $(TASK3_DIR)/parallel_file_compressor.exe
TARGET4 = $(TASK4_DIR)/sudoku_solver.exe
TARGET5 = $(TASK5_DIR)/game_tree_search.exe
TARGET6 = $(TASK6_DIR)/nqueens_solver.exe

# Build all tasks
all: task1 task2 task3 task4 task5 task6
	@echo ""
	@echo "====================================="
	@echo "All tasks compiled successfully!"
	@echo "====================================="

# Task 1: Parallel Logical Evaluation
task1: $(TARGET1)
	@echo "[✓] Task 1 compiled"

$(TARGET1): $(TASK1_DIR)/logical_evaluation.c
	$(CC) $(CFLAGS_BASIC) -o $(TARGET1) $(TASK1_DIR)/logical_evaluation.c

# Task 2: Parallel Sorting (Merge Sort) - OPTIMIZED
task2: $(TARGET2)
	@echo "[✓] Task 2 compiled (optimized)"

$(TARGET2): $(TASK2_DIR)/parallel_merge_sort.c
	$(CC) $(CFLAGS) -o $(TARGET2) $(TASK2_DIR)/parallel_merge_sort.c

# Task 3: Parallel File Compressor
task3: $(TARGET3)
	@echo "[✓] Task 3 compiled"

$(TARGET3): $(TASK3_DIR)/parallel_file_compressor.c
	$(CC) $(CFLAGS_BASIC) -o $(TARGET3) $(TASK3_DIR)/parallel_file_compressor.c

# Task 4: Parallel Sudoku Solver
task4: $(TARGET4)
	@echo "[✓] Task 4 compiled"

$(TARGET4): $(TASK4_DIR)/sudoku_solver.c
	$(CC) $(CFLAGS_BASIC) -o $(TARGET4) $(TASK4_DIR)/sudoku_solver.c

# Task 5: Parallel Game Tree Search
task5: $(TARGET5)
	@echo "[✓] Task 5 compiled"

$(TARGET5): $(TASK5_DIR)/game_tree_search.c
	$(CC) $(CFLAGS_BASIC) -o $(TARGET5) $(TASK5_DIR)/game_tree_search.c

# Task 6: Parallel N-Queens Solver
task6: $(TARGET6)
	@echo "[✓] Task 6 compiled"

$(TARGET6): $(TASK6_DIR)/nqueens_solver.c
	@rm -f $(TARGET6) 2>/dev/null || true
	$(CC) $(CFLAGS_BASIC) -o $(TARGET6) $(TASK6_DIR)/nqueens_solver.c || \
	$(CC) $(CFLAGS_BASIC) -o $(TASK6_DIR)/nqueens_solver_alt.exe $(TASK6_DIR)/nqueens_solver.c

# Run targets (with recommended parameters)
run-task1: $(TARGET1)
	@echo "Running Task 1: Logical Evaluation..."
	./$(TARGET1)

run-task2: $(TARGET2)
	@echo "Running Task 2: Parallel Merge Sort (10M elements for good speedup)..."
	./$(TARGET2) 10000000

run-task2-small: $(TARGET2)
	@echo "Running Task 2: Parallel Merge Sort (small test)..."
	./$(TARGET2) 10000

run-task2-large: $(TARGET2)
	@echo "Running Task 2: Parallel Merge Sort (50M elements - max speedup)..."
	./$(TARGET2) 50000000

run-task3: $(TARGET3)
	@echo "Running Task 3: Parallel File Compressor..."
	./$(TARGET3)

run-task4: $(TARGET4)
	@echo "Running Task 4: Parallel Sudoku Solver..."
	./$(TARGET4)

run-task5: $(TARGET5)
	@echo "Running Task 5: Parallel Game Tree Search..."
	./$(TARGET5)

run-task6: $(TARGET6)
	@echo "Running Task 6: Parallel N-Queens Solver..."
	@if [ -f $(TARGET6) ]; then ./$(TARGET6); \
	elif [ -f $(TASK6_DIR)/nqueens_solver_alt.exe ]; then ./$(TASK6_DIR)/nqueens_solver_alt.exe; \
	elif [ -f $(TASK6_DIR)/nqueens_solver_new.exe ]; then ./$(TASK6_DIR)/nqueens_solver_new.exe; \
	else echo "Error: No executable found!"; exit 1; fi

# Run all tasks
run-all: run-task1 run-task2 run-task3 run-task4 run-task5 run-task6

# Clean all compiled files
clean:
	@echo "Cleaning all executables..."
	rm -f $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)
	rm -f $(TASK1_DIR)/*.exe $(TASK2_DIR)/*.exe $(TASK3_DIR)/*.exe
	rm -f $(TASK4_DIR)/*.exe $(TASK5_DIR)/*.exe $(TASK6_DIR)/*.exe
	@echo "Clean complete!"

# Windows-specific clean (PowerShell)
clean-windows:
	powershell -Command "Remove-Item -Force $(TASK1_DIR)/*.exe -ErrorAction SilentlyContinue"
	powershell -Command "Remove-Item -Force $(TASK2_DIR)/*.exe -ErrorAction SilentlyContinue"
	powershell -Command "Remove-Item -Force $(TASK3_DIR)/*.exe -ErrorAction SilentlyContinue"
	powershell -Command "Remove-Item -Force $(TASK4_DIR)/*.exe -ErrorAction SilentlyContinue"
	powershell -Command "Remove-Item -Force $(TASK5_DIR)/*.exe -ErrorAction SilentlyContinue"
	powershell -Command "Remove-Item -Force $(TASK6_DIR)/*.exe -ErrorAction SilentlyContinue"
	@echo "Windows clean complete!"

# Rebuild everything
rebuild: clean all

# Help
help:
	@echo "====================================="
	@echo "HPC Assignment - Makefile Commands"
	@echo "====================================="
	@echo ""
	@echo "Build Commands:"
	@echo "  make all          - Compile all tasks"
	@echo "  make task1        - Compile Task 1 only"
	@echo "  make task2        - Compile Task 2 only"
	@echo "  make task3-6      - Similar for other tasks"
	@echo "  make rebuild      - Clean and rebuild all"
	@echo ""
	@echo "Run Commands:"
	@echo "  make run-task1    - Run Task 1"
	@echo "  make run-task2    - Run Task 2 (10M elements)"
	@echo "  make run-task2-large - Run Task 2 (50M - best speedup)"
	@echo "  make run-all      - Run all tasks"
	@echo ""
	@echo "Clean Commands:"
	@echo "  make clean        - Remove all executables"
	@echo "  make clean-windows - Windows-specific clean"
	@echo ""
	@echo "Optimization Notes:"
	@echo "  - Task 2 uses aggressive optimizations (-O3 -march=native)"
	@echo "  - For best Task 2 speedup, use 10M+ elements"
	@echo "  - All tasks compiled with OpenMP support"
	@echo ""

.PHONY: all task1 task2 task3 task4 task5 task6 \
        run-task1 run-task2 run-task2-small run-task2-large \
        run-task3 run-task4 run-task5 run-task6 run-all \
        clean clean-windows rebuild help
