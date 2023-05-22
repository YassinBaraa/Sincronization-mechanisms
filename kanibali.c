#define _POSIX_C_SOURCE 199309L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

#define max_brod 7
#define min_brod 3
int N = 30; // broj misionara i kanibala*2 +1 za camac

int misionari_obala[2] = {0, 0};
int kanibali_obala[2] = {0, 0};
int kanibali_brod = 0;
int misionari_brod = 0;
int na_brodu = 0;

int obala;
int smjer = 0; // 0 desno, 1 lijevo

pthread_mutex_t monitor;
pthread_cond_t prelaze;
pthread_cond_t cekaju;

typedef struct
{
    int order;
    int random;
} ThreadParams;

void *camac()
{
    while (1)
    {
        pthread_mutex_lock(&monitor);

        while (na_brodu < 3 || misionari_brod < kanibali_brod)
        {
            pthread_cond_wait(&prelaze, &monitor);
        }
        pthread_mutex_unlock(&monitor);

        sleep(2);

        pthread_mutex_lock(&monitor);
        if (obala == 1)
        {
            printf("Camac presao na lijevu obalu\n");
            printf("K:%d M:%d\n", kanibali_brod, misionari_brod);
        }
        else
        {
            printf("Camac presao na desnu obalu\n");
            printf("K:%d M:%d\n", kanibali_brod, misionari_brod);
        }
        obala = 1 - obala;
        pthread_mutex_unlock(&monitor);

        pthread_mutex_lock(&monitor);
        na_brodu = 0;
        misionari_brod = 0;
        kanibali_brod = 0;
        pthread_cond_broadcast(&cekaju);
        printf("\n");
        pthread_mutex_unlock(&monitor);
    }
}

void *misionar(void *params)
{
    ThreadParams *threadParams = (ThreadParams *)params;
    int smjer = threadParams->random;
    int broj = threadParams->order;

    // Rest of the misionar thread code
    // ...
    // sleep(2);
    //    lock
    pthread_mutex_lock(&monitor);
    if (smjer == 1)
    {
        printf("M%d: dosao na lijevu obalu\n", broj);
    }
    else
    {
        printf("M%d: dosao na desnu obalu\n", broj);
    }
    misionari_obala[smjer]++;

    while (smjer != obala || na_brodu >= 7)
    {
        if (smjer == 1)
        {
            printf("M%d: ceka na  lijevoj obali\n", broj);
        }
        else
        {
            printf("M%d: ceka na desnoj obali\n", broj);
        }
        pthread_cond_wait(&cekaju, &monitor);
    }
    misionari_obala[smjer]--;
    misionari_brod++;
    na_brodu++;

    if (smjer == 1)
    {
        printf("M%d: usao u brod\n", broj);
    }
    else
    {
        printf("M%d: usao u brod\n", broj);
    }

    if (na_brodu >= 3 && misionari_brod >= kanibali_brod)
    {
        pthread_cond_signal(&prelaze);
        sleep(1);
    }

    // unlock
    pthread_mutex_unlock(&monitor);
}

void *kanibal(void *params)
{
    ThreadParams *threadParams = (ThreadParams *)params;
    int smjer = threadParams->random;
    int broj = threadParams->order;

    // sleep(1);
    //   lock
    pthread_mutex_lock(&monitor);
    if (smjer == 1)
    {
        printf("K%d: dosao na lijevu obalu\n", broj);
    }
    else
    {
        printf("K%d: dosao na desnu obalu\n", broj);
    }
    kanibali_obala[smjer]++;

    while (kanibali_brod >= misionari_brod || smjer != obala || na_brodu >= 7)
    {
        if (smjer == 1)
        {
            printf("K%d: ceka na ljevoj obali\n", broj);
        }
        else
        {
            printf("K%d: ceka na desnoj obali\n", broj);
        }
        pthread_cond_wait(&cekaju, &monitor);
    }
    kanibali_obala[smjer]--;
    kanibali_brod++;
    na_brodu++;

    if (smjer == 1)
    {
        printf("K%d: usao u brod\n", broj);
    }
    else
    {
        printf("K%d: usao u brod\n", broj);
    }

    if (na_brodu >= 3 && misionari_brod >= kanibali_brod)
    {
        pthread_cond_signal(&prelaze);
        sleep(1);
    }

    // unlock
    pthread_mutex_unlock(&monitor);
}

int main()
{
    srand(time(NULL));
    // int N = 30; // Number of missionaries and cannibals

    printf("Legenda: M-misionar, K-kanibal, C-čamac,\nLO-lijeva obala, DO-desna obala\nL-lijevo, D-desno\n\n");

    printf("C: prazan na desnoj obali\n");

    /*pthread_mutex_init(&monitor, NULL);
    pthread_cond_init(&prelaze, NULL);
    pthread_cond_init(&cekaju, NULL);*/

    pthread_t ljudi[N * 2];
    pthread_t camac1;

    // Create the camac thread
    if (pthread_create(&camac1, NULL, camac, NULL) != 0)
    {
        printf("Error creating camac thread!\n");
        exit(1);
    }

    // Create misionar and kanibal threads
    for (int j = 0; j < (N / 2) + 1; j++)
    {

        ThreadParams *params = malloc(sizeof(ThreadParams));
        if (params == NULL)
        {
            printf("Error allocating memory for thread parameters!\n");
            exit(1);
        }

        // Set the order number and random number for the thread
        params->order = j;

        params->random = rand() % 2; // 0 or 1
        // Create kanibal thread
        sleep(1);
        if (pthread_create(&ljudi[N + j], NULL, kanibal, params) != 0)
        {
            printf("Error creating kanibal thread!\n");
            exit(1);
        }

        params->random = rand() % 2; // 0 or 1
        // Create misionar thread
        sleep(1);
        if (pthread_create(&ljudi[j], NULL, misionar, params) != 0)
        {
            printf("Error creating misionar thread!\n");
            exit(1);
        }
    }

    // Join the camac thread
    pthread_join(camac1, NULL);

    // Join misionar and kanibal threads
    for (int j = 0; j < N; j++)
    {
        pthread_join(ljudi[j], NULL);
        pthread_join(ljudi[N + j], NULL);
    }

    return 0;
}

// monitori enkapsuliraju shared data, dok semafori samo upravljaju ko pristupa shared data
