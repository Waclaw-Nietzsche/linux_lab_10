#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h> 

#include <unistd.h>

using namespace std;

#define BUFFSIZE 5
#define SIZE 256
#define ANY 0

struct sembuf incr[3];
struct sembuf decr[3];

/* Параметры: 1 - имя файла */
int main(int argc, char const *argv[])
{    
    key_t semkey = ftok("trader", 5);
    key_t memkey = ftok("trader", 6);
    key_t memkeyflag = ftok("trader", 7);
    int shsemaphore = semget(semkey, 4, IPC_CREAT | 0666);
    int shmid = shmget(memkey, SIZE, IPC_CREAT | 0666);
    int shmidflag = shmget(memkeyflag, SIZE, IPC_CREAT | 0666);

    for (int i = 0; i < 4; i++)
    {
        incr[i].sem_num = i;
        incr[i].sem_op = 1;
        incr[i].sem_flg = 0;
        decr[i].sem_num = i;
        decr[i].sem_op = -1;
        decr[i].sem_flg = 0;
    }

    ifstream file(argv[1],ios::in);
    int famount = 0;
    file.seekg(0, ios::end);
    famount = file.tellg();
    famount = famount - 1;
    file.seekg(0,ios::beg);

    semctl(shsemaphore, 0, SETVAL, 1); // Мьютекс
    semctl(shsemaphore, 1, SETVAL, 0); // Заполнено
    semctl(shsemaphore, 2, SETVAL, BUFFSIZE); // Пусто
    semctl(shsemaphore, 3, SETVAL, 0); // Факт прочтения последнего символа (пока 0 - не прочитан, как только 1 - прочитан)
    cout << "Initial: Mutex is " << semctl(shsemaphore, 0, GETVAL, 1) << ", Filled is " << semctl(shsemaphore, 1, GETVAL, 1) << ", Empty is " << semctl(shsemaphore, 2, GETVAL, 1) << ", Permission is " << semctl(shsemaphore, 3, GETVAL, 1) << endl;
    char *cyclebuffer = (char*)shmat(shmid, NULL, 0);
    int *amount = (int*)shmat(shmidflag, NULL, 0);
    amount[0] = famount;
    
    char c;
    int j = 0;
    while(j != famount)
    {
        semop(shsemaphore, &decr[2], 1); // Берём пустую ячейку или ждём её появления
        semop(shsemaphore, &decr[0], 1); // Входим в критическую секцию (работа с буфером)
        if (c != '\n')
        {
            cyclebuffer[j % BUFFSIZE] = file.get();
            cout << cyclebuffer[j % BUFFSIZE];
            j++;
        }
        semop(shsemaphore, &incr[0], 1); // Покидаем критическую секцию
        semop(shsemaphore, &incr[1], 1); // Увеличиваем количество заполненных ячеек
    };
    cout << endl;
    cout << "Finished writing." << endl;
    file.close();

    cout << "Trader waiting for exit permission..." << endl;
    semop(shsemaphore, &decr[3], 1); // Ждём сигнала о прочтении последнего символа
    cout << "Final: Mutex is " << semctl(shsemaphore, 0, GETVAL, 1) << ", Filled is " << semctl(shsemaphore, 1, GETVAL, 1) << ", Empty is " << semctl(shsemaphore, 2, GETVAL, 1) << ", Permission is " << semctl(shsemaphore, 3, GETVAL, 1) << endl;
    for (int i = 0; i < 4; i++)
    {
        semctl(shsemaphore, i, IPC_RMID);
    }
    shmctl(shmid, IPC_RMID, NULL);
    shmctl(shmidflag, IPC_RMID, NULL);

    cout << "Trader finished." << endl;
    return 0;
}
