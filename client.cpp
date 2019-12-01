#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h> 

using namespace std;

#define BUFFSIZE 5
#define SIZE 256
#define ANY 0

struct sembuf incr[3];
struct sembuf decr[3];

/* Параметры:  */
int main(int argc, char const *argv[])
{    
    key_t semkey = ftok("trader", 5);
    key_t memkey = ftok("trader", 6);
    key_t memkeyflag = ftok("trader", 7);
    int shsemaphore = semget(semkey, 4, IPC_CREAT | IPC_EXCL | 0666);
    if (shsemaphore != -1)
    {
        for (int i = 0; i < 4; i++)
        {
            semctl(shsemaphore, i, IPC_RMID);
        }
        cerr << "Trader is not opened!" << endl;
        return -1;
    }
    else
    {
        shsemaphore = semget(semkey, 4, IPC_CREAT | 0666);
        cout << "Client connected to trader's semaphores!" << endl;
    }
    
    
    int shmid = shmget(memkey, SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid != -1)
    {
        shmctl(shmid, IPC_RMID, NULL);
        cerr << "Trader is not opened!" << endl;
        return -1;
    }
    else
    {
        shmid = shmget(memkey, SIZE, IPC_CREAT | 0666);
        cout << "Client connected to trader's cyclebuffer!" << endl;
    }

    int shmidflag = shmget(memkeyflag, SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmidflag != -1)
    {
        shmctl(shmidflag, IPC_RMID, NULL);
        cerr << "Trader is not opened!" << endl;
        return -1;
    }
    else
    {
        shmidflag = shmget(memkeyflag, SIZE, IPC_CREAT | 0666);
        cout << "Client connected to trader's flaginfo!" << endl;
    }
    
    for (int i = 0; i < 4; i++)
    {
        incr[i].sem_num = i;
        incr[i].sem_op = 1;
        incr[i].sem_flg = 0;
        decr[i].sem_num = i;
        decr[i].sem_op = -1;
        decr[i].sem_flg = 0;
    }
    cout << "Initial: Mutex is " << semctl(shsemaphore, 0, GETVAL, 1) << ", Filled is " << semctl(shsemaphore, 1, GETVAL, 1) << ", Empty is " << semctl(shsemaphore, 2, GETVAL, 1) << ", Permission is " << semctl(shsemaphore, 3, GETVAL, 1) << endl;
    char *cyclebuffer = (char*)shmat(shmid, NULL, 0);
    int *amount = (int*)shmat(shmidflag, NULL, 0);
    
    int j = 0;

    while(j != amount[0])
    {
        semop(shsemaphore, &decr[1], 1); // Берём заполненную ячейку или ждём её появления
        semop(shsemaphore, &decr[0], 1); // Входим в критическую секцию (работа с буфером)
        cout << cyclebuffer[j % BUFFSIZE];
        j++;
        semop(shsemaphore, &incr[0], 1); // Покидаем критическую секцию
        semop(shsemaphore, &incr[2], 1); // Увеличиваем количество пустых ячеек
    };
    cout << endl;
    cout << "Finished reading." << endl;
    cout << "Client gives permission for trader to exit." << endl;
    semop(shsemaphore, &incr[3], 1); // Увеличиваем значение симофора и сигнализируем о прочтении последнего символа
    cout << "Final: Mutex is " << semctl(shsemaphore, 0, GETVAL, 1) << ", Filled is " << semctl(shsemaphore, 1, GETVAL, 1) << ", Empty is " << semctl(shsemaphore, 2, GETVAL, 1) << ", Permission is " << semctl(shsemaphore, 3, GETVAL, 1) << endl;
    
    cout << "Client finished." << endl;
    
    return 0;
}
