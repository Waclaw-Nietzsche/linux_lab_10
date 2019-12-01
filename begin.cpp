#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

#define SIZE 256

struct sembuf incr[2];
struct sembuf decr[2];

/* Параметры: 1 - количество писателей, 2 - количество читателей, 3 - число обращений писателей, 4 - число обращений читателей */
int main(int argc, char const *argv[])
{
    int numwriters = atoi(argv[1]);
    int numreaders = atoi(argv[2]);
    int countwriters = atoi(argv[3]);
    int countreaders = atoi(argv[4]);
    int readers = 0; // количество читателей в критической секции
    pid_t fork_w_pid[numwriters];
    pid_t fork_r_pid[numreaders];

    key_t semkey = ftok("writer", 5);
    key_t memkey = ftok("writer", 6);
    int shsemaphore = semget(semkey, 3, IPC_CREAT | 0666);
    int shmid = shmget(memkey, SIZE, IPC_CREAT | 0666);

    int *globals = (int*)shmat(shmid, NULL, 0);
    globals[0] = readers; // количество читателей в критической секции (как разделенная переменная)

    for (int i = 0; i < 3; i++)
    {
        incr[i].sem_num = i;
        incr[i].sem_op = 1;
        incr[i].sem_flg = 0;
        decr[i].sem_num = i;
        decr[i].sem_op = -1;
        decr[i].sem_flg = 0;
    }

    semctl(shsemaphore, 0, SETVAL, 1); // для безопасного доступа к общей переменной числа активных читателей
    semctl(shsemaphore, 1, SETVAL, 1); // для доступа писателя в критическую секцию
    semctl(shsemaphore, 2, SETVAL, 1); // для записи прочитанного читателем в файл
    char temp[8];
    for (int i = 0; i < numwriters; i++)
    {
        snprintf(temp, sizeof(temp), "%d", i);
        if ((fork_w_pid[i] = fork()) == 0)
        {
            execl("writer", "writer", temp, argv[3], NULL);
            cout << "Must not output" << endl;
            exit(0);
        }
        else if (fork_w_pid[i] == -1)
        {
                cerr << "Can't do writer_fork! Exiting...";
        }
    }

    for (int j = 0; j < numreaders; j++)
    {
        snprintf(temp, sizeof(temp), "%d", j);
        if ((fork_r_pid[j] = fork()) == 0)
        {
            execl("reader", "reader", temp, argv[4], NULL);
            cout << "Must not output" << endl;
            exit(0);
        }
        else if (fork_r_pid[j] == -1)
        {
                cerr << "Can't do reader_fork! Exiting...";
        }
    }
    
    for (int k = 0; k < numwriters; k++)
    {
        waitpid(fork_w_pid[k], NULL, 0);
    }
    for (int l = 0; l < numreaders; l++)
    {
        waitpid(fork_r_pid[l], NULL, 0);
    }
    

    for (int i = 0; i < 3; i++)
    {
        semctl(shsemaphore, i, IPC_RMID);
    }
    shmctl(shmid, IPC_RMID, NULL);

    cout << "Finished" << endl;

    return 0;
}
