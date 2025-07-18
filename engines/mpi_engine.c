#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define ind2d(i,j) (i)*(tam+2)+j
#define POWMIN 3
#define POWMAX 10

double wall_time(void) {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    return(tv.tv_sec + tv.tv_usec/1000000.0);
}

void UmaVida_Local(int* sub_tabulIn, int* sub_tabulout, int tam_local_rows, int tam_total_cols) {
    int i, j, vizviv;
    for (i = 1; i <= tam_local_rows; i++) {
        for (j = 1; j <= tam_total_cols - 2; j++) {
            vizviv = sub_tabulIn[i * tam_total_cols + (j-1)] + sub_tabulIn[i * tam_total_cols + j] +
                     sub_tabulIn[i * tam_total_cols + (j+1)] +
                     sub_tabulIn[(i-1) * tam_total_cols + (j-1)] + sub_tabulIn[(i-1) * tam_total_cols + j] +
                     sub_tabulIn[(i-1) * tam_total_cols + (j+1)] +
                     sub_tabulIn[(i+1) * tam_total_cols + (j-1)] + sub_tabulIn[(i+1) * tam_total_cols + j] +
                     sub_tabulIn[(i+1) * tam_total_cols + (j+1)];

            if (sub_tabulIn[i * tam_total_cols + j] && vizviv < 2)
                sub_tabulout[i * tam_total_cols + j] = 0;
            else if (sub_tabulIn[i * tam_total_cols + j] && vizviv > 3)
                sub_tabulout[i * tam_total_cols + j] = 0;
            else if (!sub_tabulIn[i * tam_total_cols + j] && vizviv == 3)
                sub_tabulout[i * tam_total_cols + j] = 1;
            else
                sub_tabulout[i * tam_total_cols + j] = sub_tabulIn[i * tam_total_cols + j];
        }
    }
}

void DumpTabul(int * tabul, int tam, int first, int last, char* msg) {}
void InitTabul (int* tabulIn, int* tabulout, int tam){}
int Correto (int* tabul, int tam){ return 0; }

int main(int argc, char** argv) {
    int rank, num_procs;
    int pow, i, tam_total;
    int *global_tabulIn = NULL, *global_tabulout = NULL;
    double to_total, t1_total, t2_total, t3_total;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    to_total = wall_time();

    for (pow = POWMIN; pow <= POWMAX; pow++) {
        tam_total = 1 << pow;
        int tam_padded = tam_total + 2;
        int rows_per_proc = tam_total / num_procs;
        int local_rows_with_padding = rows_per_proc + 2;
        int local_start_row_logical = rank * rows_per_proc + 1;
        int local_end_row_logical = (rank + 1) * rows_per_proc;

        int *sub_tabulIn = (int*) malloc(local_rows_with_padding * tam_padded * sizeof(int));
        int *sub_tabulout = (int*) malloc(local_rows_with_padding * tam_padded * sizeof(int));

        if (rank == 0) {
            global_tabulIn = (int*) malloc(tam_padded * tam_padded * sizeof(int));
            global_tabulout = (int*) malloc(tam_padded * tam_padded * sizeof(int));
            InitTabul(global_tabulIn, global_tabulout, tam_total);
        }

        t1_total = wall_time();

        for (int p = 0; p < num_procs; ++p) {
            int current_proc_start_row_logical = p * rows_per_proc + 1;
            int current_proc_end_row_logical = (p + 1) * rows_per_proc;
            int src_row_global = current_proc_start_row_logical - 1;
            int num_rows_to_send = rows_per_proc + 2;
            if (p == 0) src_row_global = 0;

            if (rank == 0) {
                for (int r = 0; r < local_rows_with_padding; ++r) {
                    for (int c = 0; c < tam_padded; ++c) {
                        if (r + current_proc_start_row_logical - 1 >= 0 && r + current_proc_start_row_logical - 1 < tam_padded)
                            sub_tabulIn[r * tam_padded + c] = global_tabulIn[(r + current_proc_start_row_logical - 1) * tam_padded + c];
                        else
                            sub_tabulIn[r * tam_padded + c] = 0;
                    }
                }
            } else {
                for (int r = 0; r < local_rows_with_padding; ++r) {
                    for (int c = 0; c < tam_padded; ++c) {
                        sub_tabulIn[r * tam_padded + c] = 0;
                    }
                }
            }
        }

        for (i = 0; i < 2 * (tam_total - 3); i++) {
            MPI_Request reqs[4];
            MPI_Status stats[4];
            int send_tag_up = 1, recv_tag_up = 2;
            int send_tag_down = 3, recv_tag_down = 4;

            if (rank > 0) {
                MPI_Isend(&sub_tabulIn[1 * tam_padded], tam_padded, MPI_INT, rank - 1, send_tag_up, MPI_COMM_WORLD, &reqs[0]);
                MPI_Irecv(&sub_tabulIn[0 * tam_padded], tam_padded, MPI_INT, rank - 1, recv_tag_down, MPI_COMM_WORLD, &reqs[1]);
            }
            if (rank < num_procs - 1) {
                MPI_Isend(&sub_tabulIn[(local_rows_with_padding - 2) * tam_padded], tam_padded, MPI_INT, rank + 1, send_tag_down, MPI_COMM_WORLD, &reqs[2]);
                MPI_Irecv(&sub_tabulIn[(local_rows_with_padding - 1) * tam_padded], tam_padded, MPI_INT, rank + 1, recv_tag_up, MPI_COMM_WORLD, &reqs[3]);
            }

            if (rank > 0) MPI_Waitall(2, reqs, stats);
            if (rank < num_procs - 1) MPI_Waitall(2, reqs + 2, stats + 2);

            UmaVida_Local(sub_tabulIn, sub_tabulout, local_rows_with_padding - 2, tam_padded);

            int *temp = sub_tabulIn;
            sub_tabulIn = sub_tabulout;
            sub_tabulout = temp;
        }

        if (rank == 0) {
            t2_total = wall_time();
            if (Correto(sub_tabulIn, tam_total))
                printf("**Ok, RESULTADO CORRETO**\n");
            else
                printf("**Nok, RESULTADO ERRADO**\n");
            t3_total = wall_time();
            printf("tam=%d; tempos: init=%7.7f, comp=%7.7f, fim=%7.7f tot=%7.7f \n",
                   tam_total, t1_total - to_total, t2_total - t1_total, t3_total - t2_total, t3_total - to_total);
        }

        free(sub_tabulIn);
        free(sub_tabulout);
        if (rank == 0) {
            free(global_tabulIn);
            free(global_tabulout);
        }
    }

    MPI_Finalize();
    return 0;
}