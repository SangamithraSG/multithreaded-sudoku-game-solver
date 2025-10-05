#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define SIZE 9
#define TIME_LIMIT 10  // Time limit for each turn

typedef struct {
    int board[SIZE][SIZE];
    int turn;
    int scores[2];  // Scores for Player 1 & 2
} SharedData;

int shmid, semid;
SharedData *shared;

// Semaphore operations
void sem_wait(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void sem_signal(int semid) {
    struct sembuf op = {0, 1, 0};
    semop(semid, &op, 1);
}

// Signal handler for cleanup
void cleanup(int sig) {
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
    exit(0);
}

// Predefined Sudoku board with blanks (0s)
int predefined_board[SIZE][SIZE] = {
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

// Check if board is full
int is_board_full() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (shared->board[i][j] == 0)
                return 0;
    return 1;
}

// Display Sudoku board
void print_board() {
    printf("\nCurrent Sudoku Board:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf("%d ", shared->board[i][j]);
        }
        printf("\n");
    }
    printf("\n🏆 Scores: Player 1 = %d | Player 2 = %d\n", shared->scores[0], shared->scores[1]);

    if (is_board_full()) {
        printf("\n🎉 Game Over! Sudoku is complete! 🎉\n");
        if (shared->scores[0] > shared->scores[1])
            printf("🏆 Player 1 Wins!\n");
        else if (shared->scores[1] > shared->scores[0])
            printf("🏆 Player 2 Wins!\n");
        else
            printf("🤝 It's a Draw!\n");
        cleanup(0);
    }
}

// Check if the move is valid
int is_valid_move(int row, int col, int num) {
    if (shared->board[row][col] != 0) return 0;

    for (int i = 0; i < SIZE; i++) {
        if (shared->board[row][i] == num || shared->board[i][col] == num)
            return 0;
    }

    int startRow = (row / 3) * 3;
    int startCol = (col / 3) * 3;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (shared->board[startRow + i][startCol + j] == num)
                return 0;

    return 1;
}

// Timeout handler
void timeout_handler(int sig) {
    printf("\n⏳ Time’s up! Switching turn.\n");
    shared->turn = (shared->turn % 2) + 1;
    sem_signal(semid);
}

// Player function
void player(int player_id) {
    signal(SIGALRM, timeout_handler);

    while (1) {
        sem_wait(semid);
        if (shared->turn != player_id) {
            sem_signal(semid);
            continue;
        }

        print_board();
        alarm(TIME_LIMIT);  // Start turn timer

        int row, col, num;
        printf("Player %d, enter row, col, number (1-9): ", player_id);
        if (scanf("%d %d %d", &row, &col, &num) != 3) {
            printf("Invalid input! Try again.\n");
            alarm(0);
            sem_signal(semid);
            continue;
        }
        row--; col--;  // Convert to 0-based index
        alarm(0);  // Stop timer

        if (row < 0 || row >= SIZE || col < 0 || col >= SIZE || num < 1 || num > 9) {
            printf("❌ Invalid input! Try again.\n");
        } else if (!is_valid_move(row, col, num)) {
            printf("🚫 Invalid move! Try again.\n");
        } else {
            shared->board[row][col] = num;
            shared->scores[player_id - 1]++;  // Increase player's score
            shared->turn = (player_id % 2) + 1;
        }

        sem_signal(semid);
        sleep(1);
    }
}

int main() {
    key_t key = ftok("sudoku", 65);
    shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);
    shared = (SharedData *)shmat(shmid, NULL, 0);

    key_t sem_key = ftok("sem", 75);
    semid = semget(sem_key, 1, 0666 | IPC_CREAT);
    semctl(semid, 0, SETVAL, 1);

    // Copy predefined board to shared memory
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            shared->board[i][j] = predefined_board[i][j];

    shared->turn = 1;  // Player 1 starts
    shared->scores[0] = shared->scores[1] = 0;  // Reset scores

    signal(SIGINT, cleanup);

    if (fork() == 0) {
        player(1);
    }
    if (fork() == 0) {
        player(2);
    }

    wait(NULL);
    wait(NULL);

    cleanup(0);
    return 0;
}


