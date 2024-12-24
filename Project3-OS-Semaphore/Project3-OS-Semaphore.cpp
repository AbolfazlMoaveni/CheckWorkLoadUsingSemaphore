#include <iostream>
#include <windows.h>
#include <thread>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <semaphore>

using namespace std;

//Parameters
const auto logical_cores = std::thread::hardware_concurrency();
string file_addr;
int* jw = new int[2];
int* jtimes;
bool* cando;
int timetodo;
int* result;
int read_count = 0;
int write_count = 0;
int activeThreads = logical_cores;

int UpdatesCount = 0;

//semaphores
HANDLE p = CreateSemaphore(NULL, 0, 1, NULL);
HANDLE r = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE rw = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE q = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE AT = CreateSemaphore(NULL, 1, 1, NULL);
HANDLE ReadTry = CreateSemaphore(NULL, 1, 1, NULL);

//functions
void d_file_addr();
void jobs(int rs[]);
void work_times(int* wt, int jbs);
void can_do(bool* canwork, int jbs1, int jbs2);

DWORD WINAPI jb(LPVOID param)
{
	int tcorenum = *static_cast<int*>(param);
	delete param;
	int local_result = INT_MAX;
	long int t = static_cast<long long int> (time(NULL));
	srand(time(0) + GetCurrentThreadId() + t);
	int* ttime = new int[jw[1]];
	int* whodoes = new int[jw[0]];
	int c_whodoes = 0;

	chrono::time_point<std::chrono::system_clock> end;
	chrono::milliseconds ms((timetodo) * 1000);
	end = chrono::system_clock::now() + ms;

	while (std::chrono::system_clock::now() < end)
	{
		c_whodoes = 0;
		for (int i = 0; i < jw[1]; i++)
			ttime[i] = 0;
		for (int i = 0; i < jw[0]; i++)
			whodoes[i] = 0;

		for (int count = 0; count < jw[0]; count++)
		{
			int temp;
			do
			{
				temp = rand() % (jw[1]);
			} while (cando[(count * jw[1]) + temp] != 1);

			ttime[temp] += jtimes[count];
			whodoes[c_whodoes] = temp;
			c_whodoes++;
		}

		int maxttime = INT_MIN;
		int minttime = INT_MAX;

		for (int c = 0; c < jw[1]; c++)
		{
			if (ttime[c] < minttime) {
				minttime = ttime[c];
			}
			if (ttime[c] > maxttime)
			{
				maxttime = ttime[c];
			}
		}
		int difference = maxttime - minttime;

		if (difference < local_result) {
			local_result = difference;
			
			//WaitForSingleObject(ReadTry, INFINITE);
			//cout << "waitReadTry" << endl;
			
			WaitForSingleObject(q, INFINITE);
			WaitForSingleObject(r, INFINITE);
			if (read_count == 0)
				WaitForSingleObject(rw, INFINITE);
			read_count++;
			ReleaseSemaphore(q, 1, NULL);
			ReleaseSemaphore(r, 1, NULL);
			//start reading
			if (difference < result[0])
			{
				//end reading
				WaitForSingleObject(r, INFINITE);
				read_count--;
				if (read_count == 0)
					ReleaseSemaphore(rw, 1, NULL);
				ReleaseSemaphore(r, 1, NULL);

				// try to write
				WaitForSingleObject(ReadTry, INFINITE);
				WaitForSingleObject(rw, INFINITE);
				
				result[0] = difference;
				result[1] = tcorenum;
				for (int c = 0; c < jw[1]; c++)
				{
					result[2 + c] = ttime[c];
				}
				for (int c = 0; c < jw[0]; c++)
					result[2 + jw[1] + c] = whodoes[c];
				//end of writing

				UpdatesCount++;
				ReleaseSemaphore(p, 1, NULL);
				ReleaseSemaphore(rw, 1, NULL);
			}
			else {
				//end reading
				WaitForSingleObject(r, INFINITE);
				read_count--;
				if (read_count == 0)
					ReleaseSemaphore(rw, 1, NULL);
				ReleaseSemaphore(r, 1, NULL);

			}
			//delete[] whodoes_loop;
			//delete[] ttime;
		}
	}

	//to ensure that parent does not wait for releasing p
	WaitForSingleObject(AT, INFINITE);
	activeThreads--;
	ReleaseSemaphore(AT, 1, NULL);
	ReleaseSemaphore(p, 1, NULL);
	return 0;
}

int main()
{

	d_file_addr();
	jobs(jw);
	jtimes = new int[jw[0]];
	cando = new bool[jw[0] * jw[1]];

	work_times(jtimes, jw[0]);
	can_do(cando, jw[0], jw[1]);
	cout << "How long should the CPU Proccess (in seconds) => ";
	cin >> timetodo;

	//difference v zaman har kargar v har kar be che kargari
	result = new int[jw[0] + jw[1] + 2];

	cout << "time => " << timetodo << endl;
	cout << "jobs: " << jw[0] << "\nWorkers: " << jw[1] << "\nJob Times:\n";
	for (int i = 0; i < jw[0]; i++)
		cout << jtimes[i] << '\t';
	cout << endl;
	cout << "logical cores : " << logical_cores << endl;

	for (int i = 0; i < jw[0] + jw[1] + 1; i++) {
		result[i] = INT_MAX;
	}

	HANDLE* thrds = new HANDLE[logical_cores];
	int* corenum;
	for (int i = 0; i < logical_cores; i++)
	{
		corenum = new int(i);
		thrds[i] = CreateThread(NULL, 0, jb, corenum, 0, NULL);

		if (!thrds[i])
		{
			cout << "Create Process " << i << " failed" << endl;
			return 1;
		}
	}
	int count = 0;
	while (true) {
		WaitForSingleObject(p, INFINITE);
		if (activeThreads == 0) {
			break;
		}
		else {
			WaitForSingleObject(q, INFINITE);
			WaitForSingleObject(r, INFINITE);
			if (read_count == 0)
				WaitForSingleObject(rw, INFINITE);
			read_count++;
			ReleaseSemaphore(q, 1, NULL);
			ReleaseSemaphore(r, 1, NULL);
			//reading
			count++;
			cout << "--------------------------" << endl;
			cout << "update # " << count << endl;
			cout << "UpdatesCount = " << UpdatesCount << endl;
			cout << "Found by the logical core " << result[1] << "\n";
			//for (int i = 0; i < jw[0]; i++)
			//{
			//    cout << "job " << i << ": " << result[2 + jw[1] + i] << "th worker" << endl;
			//}
			//cout << "--------------------------" << endl;
			//for (int temp = 0; temp < jw[1]; temp++)
			//{
			//    cout << temp+1 << "th worker => " << result[2 + temp] << endl;
			//}
			cout << "--------------------------" << endl;
			cout << "Difference : " << result[0] << endl;
			cout << "--------------------------" << endl;
			//end of reading
			WaitForSingleObject(r, INFINITE);
			read_count--;
			if (read_count == 0)
				ReleaseSemaphore(rw, 1, NULL);
			ReleaseSemaphore(r, 1, NULL);
			//ReleaseSemaphore(rw, 1, NULL);
			ReleaseSemaphore(ReadTry, 1, NULL);
			//cout << "Release Readtry" << endl;
		}
	}

	for (int i = 0; i < logical_cores; i++)
	{
		if (thrds[i] != 0)
			CloseHandle(thrds[i]);
	}

	return 0;
}



void d_file_addr()
{
	int choice;
	cout << "which task file you want?\n";
	for (int i = 0; i < 4; i++)
	{
		cout << i + 1 << ".";
		switch (i)
		{
		case 0: cout << "7 job and 3 worker"; break;
		case 1: cout << "basiri"; break;
		case 2: cout << "khodam:)"; break;
		}
		cout << endl;
	}
	do
	{
		cin >> choice;
		switch (choice)
		{
		case 1: file_addr = "tasks1.txt"; break;
		case 2: file_addr = "basiri.txt"; break;
		case 3: file_addr = "moaveni.txt"; break;
		default: cout << "wrong choice" << endl; break;
		}
	} while (choice > 4);
}

void jobs(int rs[]) {
	ifstream file(file_addr);
	if (!file) {
		cout << "not file - jobs\n";
		exit(-1);
	}
	file >> rs[0];
	file >> rs[1];
	file.close();
	return;
}

void work_times(int* wt, int jbs) {
	int temp;
	ifstream file(file_addr);
	if (!file) {
		cout << "not file - jobs\n";
		exit(-1);
	}
	file >> temp;
	file >> temp;
	for (int i = 0; i < jbs; i++)
	{
		file >> wt[i];
	}
	file.close();
	return;
}

void can_do(bool* canwork, int jbs1, int jbs2) {
	int temp;
	ifstream file(file_addr);
	if (!file) {
		cout << "not file - jobs\n";
		exit(-1);
	}
	file >> temp;
	file >> temp;
	for (int i = 0; i < jbs1; i++)
	{
		file >> temp;
	}
	for (int i = 0; i < (jbs1 * jbs2); i++) {
		file >> canwork[i];
	}

	file.close();
	return;

}