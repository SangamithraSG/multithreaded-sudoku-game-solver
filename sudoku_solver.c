#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define SIZE 9
#define NUM_THREADS SIZE

int sudoku[SIZE][SIZE];
sem_t mutex;

// Function to check if a number can be placed in a cell
bool is_safe(int row, int col, int num) {
    for (int i = 0; i < SIZE; i++) {
        if (sudoku[row][i] == num || sudoku[i][col] == num) {
            return false;
        }
    }
    
    int startRow = row - row % 3, startCol = col - col % 3;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (sudoku[i + startRow][j + startCol] == num) {
                return false;
            }
        }
    }
    return true;
}

// Backtracking function to solve Sudoku
bool solve_sudoku() {
    for (int row = 0; row < SIZE; row++) {
        for (int col = 0; col < SIZE; col++) {
            if (sudoku[row][col] == 0) {
                for (int num = 1; num <= SIZE; num++) {
                    if (is_safe(row, col, num)) {
                        sudoku[row][col] = num;
                        if (solve_sudoku()) {
                            return true;
                        }
                        sudoku[row][col] = 0;
                    }
                }
                return false;
            }
        }
    }
    return true;
}

// Thread function to solve Sudoku
void* thread_solve(void* arg) {
    sem_wait(&mutex);
    solve_sudoku();
    sem_post(&mutex);
    pthread_exit(NULL);
}

// Function to print Sudoku grid
void print_sudoku() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", sudoku[i][j]);
        }
        printf("\n");
    }
}

int main() {
    printf("Enter the Sudoku puzzle (use 0 for empty cells):\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            scanf("%d", &sudoku[i][j]);
        }
    }

    pthread_t solver_thread;
    sem_init(&mutex, 0, 1);
    
    pthread_create(&solver_thread, NULL, thread_solve, NULL);
    pthread_join(solver_thread, NULL);
    
    sem_destroy(&mutex);
    
    printf("\nSolved Sudoku:\n");
    print_sudoku();
    
    return 0;
}
