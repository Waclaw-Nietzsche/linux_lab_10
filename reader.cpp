#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <string>

using namespace std;

#define SIZE 256

struct sembuf incr[2];
struct sembuf decr[2];

/* READER */
int main(int argc, char const *argv[])
{
    int id = atoi(argv[1]); // id читателя
    int count = atoi(argv[2]); // число повторений обращений
    cout << "Started READER №" << id << endl;

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

    int *globals = (int*)shmat(shmid, NULL, 0);
    
    ofstream file("RWlog.txt",ios::app);
    ofstream routfile("ROutput.txt",ios::app);
    ifstream readfile("Output.txt",ios::in);

    time_t my_time;
    struct tm *timeinfo;
    int reads;

    for(int i = 0; i < count; i++)
    {
        
        time (&my_time);
        timeinfo = localtime (&my_time);
        file << "Iteration №" << i << " - request by Reader №" << id << " at " << timeinfo->tm_min << ":" << timeinfo->tm_sec << endl;
        
        semop(shsemaphore, &decr[0], 1);  // Ждём разрешения на изменения переменной числа активных читателей
        
        reads = globals[0];
        reads = reads + 1;
        globals[0] = reads; // Увеличиваем число читателей так как теперь активен текущий 

        if(reads == 1)   // Только если это первый читатель
            semop(shsemaphore, &decr[1], 1); // Ждём освобождения КС
        semop(shsemaphore, &incr[0], 1);   // Освобождаем доступ к переменной числа активных читателей

        time (&my_time);
        timeinfo = localtime (&my_time);
        file << "Iteration №" << i << " - entry by Reader №" << id << " at " << timeinfo->tm_min << ":" << timeinfo->tm_sec << endl;
        
        // Выводим содержимое файла
        semop(shsemaphore, &decr[2], 1);
        string str;
        routfile << "Reader №" << id << endl;
        while(!readfile.eof()) 
        {
	        getline(readfile, str); 
	        routfile << str << endl; 
        }
        semop(shsemaphore, &incr[2], 1);

        semop(shsemaphore, &decr[0], 1);    // Ждём разрешения на изменения переменной числа активных читателей
        reads = globals[0];
        reads = reads - 1;
        globals[0] = reads; // Уменьшаем число активных читателей, так как мы закончили чтение
        if(reads == 0)   // Только если это последний читатель
            semop(shsemaphore, &incr[1], 1); // Освобождаем КС

        semop(shsemaphore, &incr[0], 1);   // Освобождаем доступ к переменной числа активных читателей
        
        time (&my_time);
        timeinfo = localtime (&my_time);

        file << "Iteration №" << i << " - exit by Reader №" << id << " at " << timeinfo->tm_min << ":" << timeinfo->tm_sec << endl;
        readfile.clear();
        readfile.seekg(0, ios::beg);
    }
    file.close();
    //cout << "Reader finished" << endl;


    return 0;
}
