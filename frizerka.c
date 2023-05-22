#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

// redoslijed ne valja treba bit random
// radno vrijeme ne radi
// ispisuje spavam dok cekam 7 puta nakon sto je zavrsio s frizurama

#define uk_mjesta 5
// zajednicka varijabla
int *N = 0; // broj kljenata u cekaonici
// int otvoreno = 0;      // nije otvoreno
int *radno_vrijeme; // radno vrijeme salona

// semafori
sem_t *sem1; // globalna varijabla = za kazaljku na objekt u zajedničkoj memoriji
sem_t *sem2;
sem_t *sem_radno_vrijeme;
// sem_t *sem_otvoreno;
sem_t *br_klijenata;

void vrijeme()
{
    // pusti da klijenti završe s frizurom
    sleep(10);
    // označi kraj radnog vremena
    *radno_vrijeme = 0;
}

void frizerka()
{
    // otvoreno = 1;
    printf("Frizerka: Otvaram salon\n");

    while (1)
    {
        // printf("%d \n", *radno_vrijeme);
        //  sem_wait(sem_radno_vrijeme);
        if (*radno_vrijeme == 0)
        {
            // otvoreno = 0;
            printf("Frizerka: Postavljam znak ZATVORENO\n");
            printf("Frizerka: Zatvaram salon\n");
            sem_post(sem_radno_vrijeme); //--------->
            exit(0);
        }

        else if (*N == 0 && *radno_vrijeme == 1)
        {
            // spavaj(semafor)
            printf("Frizerka: Spavam dok klijenti ne dođu\n");
            sem_wait(sem2);
            // printf("bruh");
        }

        else if (*N > 0 && *radno_vrijeme == 1)
        {
            // uzmi prvog klijenta(semafor)
            sleep(2); // radi na frizuri
            sem_post(sem1);
            printf("Frizerka: Uzimam novog klijenta\n");

            // printf("Frizerka: Frizura gotova.\n");
        }
    }
}

void klijent(int id)
{

    printf("\tKlijent(%d): Zelim frizuru \n", id);
    if (*radno_vrijeme == 1 && *N < uk_mjesta)
    {
        // signal frizerki(semafor)
        printf("Klijent %d: Čekam u čekaonici.\n", id);
        sem_post(sem2);

        // printf("%d \n", *N);
        sem_wait(br_klijenata);
        *N += 1;
        sem_post(br_klijenata);
        // printf("%d \n", *N);

        // cekaj_red(semafor)
        sem_wait(sem1);

        // printf("%d ", *N);
        sem_wait(br_klijenata);
        *N -= 1;
        sem_post(br_klijenata);
        // printf("%d \n", *N);

        printf("Klijent %d: Gotov s frizurom.\n", id);
        // sem_wait(&sem_radno_vrijeme);
        exit(0);
    }
    else if (*radno_vrijeme == 0)
    {
        sem_wait(sem_radno_vrijeme); //--------->
        printf("\tKlijent(%d): gotovo radno vrijeme\n", id);
        sem_post(sem_radno_vrijeme); //--------->
        exit(0);
    }
    else
    {
        printf("\tKlijent(%d): danas ništa od frizure\n", id);
        exit(0);
    }
}

int main()
{
    printf("\n");
    int id = shmget(IPC_PRIVATE, sizeof(int) * 2, 0600);
    N = (int *)shmat(id, NULL, 0);
    *N = 0;
    radno_vrijeme = N + 1;
    *radno_vrijeme = 1;

    int ID = shmget(IPC_PRIVATE, sizeof(sem_t) * 4, 0600);
    if (ID == -1)
    {
        exit(1);
    }
    sem1 = shmat(ID, NULL, 0);
    sem2 = sem1 + 1;
    sem_radno_vrijeme = sem1 + 2;
    br_klijenata = sem1 + 3;

    sem_init(sem1, 1, 0);              // početna vrijednost = 0
    sem_init(sem2, 1, 0);              // početna vrijednost = 0
    sem_init(sem_radno_vrijeme, 1, 0); // početna vrijednost = 1 //--------->
    sem_init(br_klijenata, 1, 1);      // početna vrijednost = 1
    // sem_init(zvono, 1, 0);             // početna vrijednost = 0

    int M = 7; // broj klijenata------------------------------->

    for (int i = 0; i < M + 1 + 1; i++)
    {
        sleep(1);
        switch (fork())
        {
        case 0:
            // funkcija koja obavlja posao djeteta

            if (i == 1)
            {
                frizerka();
            }
            else if (i == 0)
            {
                vrijeme();
            }
            else if (i == M || i == M + 1)
            {
                sleep(5);
                klijent(i - 1); // zbog ovoga (if statement) procesi idu po redu
            }
            else
            {
                klijent(i - 1); // zbog ovoga (if statement) procesi idu po redu
            }
            exit(0);
        case -1:
            // ispis poruke o nemogućnosti stvaranja procesa;
            printf("Ne mogu stvoriti novi proces!\n");
            exit(0);
        default:
            break;
        }
    }

    for (int i = 0; i < M + 1 + 1; i++)
    {
        wait(NULL);
    }

    sem_destroy(sem1);
    sem_destroy(sem2);
    sem_destroy(sem_radno_vrijeme);
    // sem_destroy(sem_otvoreno);
    sem_destroy(br_klijenata);

    (void)shmdt((char *)N);
    (void)shmdt((char *)radno_vrijeme);

    shmdt(sem1);
    shmdt(sem2);
    shmdt(sem_radno_vrijeme);
    // shmdt(sem_otvoreno);
    shmdt(br_klijenata);
    shmctl(id, IPC_RMID, NULL);
    shmctl(ID, IPC_RMID, NULL);

    exit(0);
}