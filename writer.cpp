#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <chrono>

using namespace std;

#define SIZE 256

struct sembuf incr[2];
struct sembuf decr[2];

/* WRITER */
int main(int argc, char const *argv[])
{
    
    int id = atoi(argv[1]); // id писателя
    int count = atoi(argv[2]); // число повторений обращений
    cout << "Started WRITER №" << id << endl;
    
    key_t semkey = ftok("writer", 5);
    key_t memkey = ftok("writer", 6);
    int shsemaphore = semget(semkey, 3, IPC_CREAT | IPC_EXCL | 0666);
    if (shsemaphore != -1)
    {
        for (int i = 0; i < 3; i++)
        {
            semctl(shsemaphore, i, IPC_RMID);
        }
        cerr << "BEGIN is not opened!" << endl;
        return -1;
    }
    else
    {
        shsemaphore = semget(semkey, 3, IPC_CREAT | 0666);
        //cout << "Client connected to trader's semaphores!" << endl;
    }

    int shmid = shmget(memkey, SIZE, IPC_CREAT | IPC_EXCL | 0666);
    if (shmid != -1)
    {
        shmctl(shmid, IPC_RMID, NULL);
        cerr << "BEGIN is not opened!" << endl;
        return -1;
    }
    else
    {
        shmid = shmget(memkey, SIZE, IPC_CREAT | 0666);
        //cout << "Client connected to trader's memory!" << endl;
    }

    for (int i = 0; i < 3; i++)
    {
        incr[i].sem_num = i;
        incr[i].sem_op = 1;
        incr[i].sem_flg = 0;
        decr[i].sem_num = i;
        decr[i].sem_op = -1;
        decr[i].sem_flg = 0;
    }

    ofstream file("RWlog.txt",ios::app);
    ofstream outfile("Output.txt",ios::app);

    time_t my_time;
    struct tm *timeinfo;

    for (int i = 0; i < count; i++)
    {
        // Вычисляем время запроса писателя     
        time (&my_time);
        timeinfo = localtime (&my_time);
        file << "Iteration №" << i << " - request by Writer №" << id << " at " << timeinfo->tm_min << ":" << timeinfo->tm_sec << endl;
        
        semop(shsemaphore, &decr[1], 1); // Вход писателя в КС

        // Вычисляем время входа писателя
        time (&my_time);
        timeinfo = localtime (&my_time);
        file << "Iteration №" << i << " - entry by Writer №" << id << " at " << timeinfo->tm_min << ":" << timeinfo->tm_sec << endl;

        // Пишем в файл строку данного писателя
        outfile << "Writer №" << id << ", PID is " << getpid() << endl;
        //cout << "Writer №" << id << ", PID is " << getpid() << endl;
        
        semop(shsemaphore, &incr[1], 1); // Выход писателя из КС

        // Вычисляем время выхода писателя
        time (&my_time);
        timeinfo = localtime (&my_time);
        file << "Iteration №" << i << " - exit by Writer №" << id << " at " << timeinfo->tm_min << ":" << timeinfo->tm_sec << endl;

    }
    file.close();
    outfile.close();
    return 0;
}
