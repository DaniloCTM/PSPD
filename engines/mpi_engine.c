#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <mpi.h>

#define ind2d(i,j) (i)*(tam+2)+j
#define POWMIN 3
#define POWMAX 10
#define MESTRE 0
#define TAG_A 0
#define TAG_B 1

double wall_time(void) {
  struct timeval tv;
  struct timezone tz;

  gettimeofday(&tv, &tz);
  return(tv.tv_sec + tv.tv_usec/1000000.0);
} /* fim-wall_time */

void UmaVida_Paralelo(int* tabulIn, int* tabulOut, int tam, int linhas_locais) {
  int i, j, vizviv;

  for (i=1; i<=linhas_locais; i++) {
    for (j= 1; j<=tam; j++) {
      vizviv =  tabulIn[ind2d(i-1,j-1)] + tabulIn[ind2d(i-1,j  )] +
                tabulIn[ind2d(i-1,j+1)] + tabulIn[ind2d(i  ,j-1)] +
                tabulIn[ind2d(i  ,j+1)] + tabulIn[ind2d(i+1,j-1)] +
                tabulIn[ind2d(i+1,j  )] + tabulIn[ind2d(i+1,j+1)];
      if (tabulIn[ind2d(i,j)] && vizviv < 2)
        tabulOut[ind2d(i,j)] = 0;
      else if (tabulIn[ind2d(i,j)] && vizviv > 3)
        tabulOut[ind2d(i,j)] = 0;
      else if (!tabulIn[ind2d(i,j)] && vizviv == 3)
        tabulOut[ind2d(i,j)] = 1;
      else
        tabulOut[ind2d(i,j)] = tabulIn[ind2d(i,j)];
    } /* fim-for */
  } /* fim-for */
} /* fim-UmaVida_Paralelo */


void InitTabul(int* tabulIn, int tam){
  size_t total_size = (size_t)(tam + 2) * (tam + 2) * sizeof(int);
  memset(tabulIn, 0, total_size);

  tabulIn[ind2d(1,2)] = 1; tabulIn[ind2d(2,3)] = 1;
  tabulIn[ind2d(3,1)] = 1; tabulIn[ind2d(3,2)] = 1;
  tabulIn[ind2d(3,3)] = 1;
} /* fim-InitTabul */


int Correto(int* tabul, int tam, int global_cnt){
  return (global_cnt == 5 && tabul[ind2d(tam-2,tam-1)] &&
      tabul[ind2d(tam-1,tam  )] && tabul[ind2d(tam  ,tam-2)] &&
      tabul[ind2d(tam  ,tam-1)] && tabul[ind2d(tam  ,tam  )]);
} /* fim-Correto */


int main(int argc, char** argv) {
  int pow, i, tam, *tabulIn_global, *tabulIn_local, *tabulOut_local;
  double t0, t1, t2, t3;
  int rank, size;
  int linhas_por_proc, resto, minhas_linhas;
  
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  for (pow=POWMIN; pow<=POWMAX; pow++) {
    tam = 1 << pow;

    // divisao de linhas para cada processo
    linhas_por_proc = tam / size;
    resto = tam % size;
    minhas_linhas = (rank < resto) ? linhas_por_proc + 1 : linhas_por_proc;

    size_t local_size_bytes = (size_t)(minhas_linhas + 2) * (tam + 2) * sizeof(int);
    tabulIn_local = (int *)malloc(local_size_bytes);
    tabulOut_local = (int *)malloc(local_size_bytes);
    memset(tabulIn_local, 0, local_size_bytes);
    memset(tabulOut_local, 0, local_size_bytes);

    if (rank == MESTRE) {
        t0 = wall_time();
        tabulIn_global = (int *)malloc((size_t)(tam + 2) * (tam + 2) * sizeof(int));
        InitTabul(tabulIn_global, tam);
    }
    
    // distribui o tabuleiro inicial
    if (rank == MESTRE) {
        int offset_linhas = 0;
        for(i = 0; i < size; i++) {
            int linhas_p = (i < resto) ? linhas_por_proc + 1 : linhas_por_proc;
            if (i == MESTRE) {
                memcpy(&tabulIn_local[ind2d(1,0)], &tabulIn_global[ind2d(offset_linhas+1, 0)], linhas_p * (tam+2) * sizeof(int));
            } else {
                MPI_Send(&tabulIn_global[ind2d(offset_linhas+1, 0)], linhas_p * (tam+2), MPI_INT, i, TAG_A, MPI_COMM_WORLD);
            }
            offset_linhas += linhas_p;
        }
    } else {
        MPI_Recv(&tabulIn_local[ind2d(1,0)], minhas_linhas * (tam+2), MPI_INT, MESTRE, TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == MESTRE) t1 = wall_time();

    int *temp;
    for (i=0; i < 4 * (tam-3); i++) {
      // troca de halos com vizinhos
      // Fase 1: enviar para baixo, receber de cima
      if (rank < size - 1)
        MPI_Send(&tabulIn_local[ind2d(minhas_linhas, 0)], tam+2, MPI_INT, rank + 1, TAG_A, MPI_COMM_WORLD);
      if (rank > 0)
        MPI_Recv(&tabulIn_local[ind2d(0, 0)], tam+2, MPI_INT, rank - 1, TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      
      // Fase 2: enviar para cima, receber de baixo
      if (rank > 0)
        MPI_Send(&tabulIn_local[ind2d(1, 0)], tam+2, MPI_INT, rank - 1, TAG_B, MPI_COMM_WORLD);
      if (rank < size - 1)
        MPI_Recv(&tabulIn_local[ind2d(minhas_linhas+1, 0)], tam+2, MPI_INT, rank + 1, TAG_B, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      UmaVida_Paralelo(tabulIn_local, tabulOut_local, tam, minhas_linhas);
      
      temp = tabulIn_local;
      tabulIn_local = tabulOut_local;
      tabulOut_local = temp;
    } /* fim-for */
    
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == MESTRE) t2 = wall_time();

    // coleta e verifica os resultados
    int local_cnt = 0;
    for(int k = 1; k <= minhas_linhas; k++)
        for(int j = 1; j <= tam; j++)
            local_cnt += tabulIn_local[ind2d(k,j)];
    
    int global_cnt = 0;
    MPI_Reduce(&local_cnt, &global_cnt, 1, MPI_INT, MPI_SUM, MESTRE, MPI_COMM_WORLD);
    
    // coleta fatias finais
    if (rank == MESTRE) {
        int offset_linhas = 0;
        for(i = 0; i < size; i++) {
            int linhas_p = (i < resto) ? linhas_por_proc + 1 : linhas_por_proc;
            if (i == MESTRE) {
                memcpy(&tabulIn_global[ind2d(offset_linhas+1, 0)], &tabulIn_local[ind2d(1,0)], linhas_p * (tam+2) * sizeof(int));
            } else {
                MPI_Recv(&tabulIn_global[ind2d(offset_linhas+1, 0)], linhas_p * (tam+2), MPI_INT, i, TAG_A, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            offset_linhas += linhas_p;
        }
    } else {
        MPI_Send(&tabulIn_local[ind2d(1,0)], minhas_linhas * (tam+2), MPI_INT, MESTRE, TAG_A, MPI_COMM_WORLD);
    }
    
    if (rank == MESTRE) {
      if (Correto(tabulIn_global, tam, global_cnt))
        printf("**RESULTADO CORRETO**\n");
      else
        printf("**RESULTADO ERRADO**\n");

      t3 = wall_time();
      printf("tam=%d; ranks=%d; tempos: init=%7.7f, comp=%7.7f, fim=%7.7f, tot=%7.7f \n",
            tam, size, t1-t0, t2-t1, t3-t2, t3-t0);

      free(tabulIn_global);
    }
    
    free(tabulIn_local);
    free(tabulOut_local);
  } /* fim-for-pow */
  
  MPI_Finalize();
  return 0;
} /* fim-main */