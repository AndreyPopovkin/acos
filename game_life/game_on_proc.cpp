#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <unistd.h>

using std::cin;
using std::cout;

#define SIZE 16
#define NUM_OF_PROCESSES 8
#define SEM_NAME "/semaphore"

sem_t* sem[4];
int sem_init[4] = {0, 0, NUM_OF_PROCESSES, 0};

int ITERATION;
// get из it
// set в it^1
// процессы отработав ждут it

int get(int x, int y);
int set(int x, int y, int arg);

int alive(int x, int y)
{
	int al = 0;
	for(int i = -1; i <= 1; ++i)
		for(int q = -1; q <= 1; ++q)
			if(i != q)
				al += get(x + i, y + q);
}

int recalc(int x, int y)
{
	int alive_ = alive(x, y);
	if (get(x, y) == 0){
		if (alive_ == 3) return 1;
		else return 0;
	}
	if (get(x, y) == 1){
		if (alive_ == 3 || alive_ == 2) return 1;
		else return 0;
	}
}

int getval(sem_t* s){
  int a;
  sem_getvalue(s, &a);
  return a;
}

void work(int id, int it_num){
	
	// тут mmap на глобальную область памяти

	int rows_num = SIZE / NUM_OF_PROCESSES;
	bool main_ = 0;
	for(int i = 0; i < it_num; ++i){
		ITERATION = i & 1;
		for(int i = 0; i < rows_num; ++i)
			for(int q = 0; q < SIZE; ++q)
				set(i, q, recalc(i, q));
		if(getval(sem[2]) == 1){
			//sem_wait(sem[2]);
			main_ = 1;
			for(int i = 0; i < NUM_OF_PROCESSES - 1; ++i)
				sem_post(sem[2]);
			for(int i = 0; i < NUM_OF_PROCESSES - 1; ++i)
				sem_post(sem[ITERATION]);
		}else{
			main_ = 0;
			sem_wait(sem[2]);
			sem_wait(sem[ITERATION]);
		}
	}
	if(main_){
		sem_post(sem[3]);
	}

	// тут разmmap
}

void init_sems(int suffix){
	sem_unlink(((std::string)SEM_NAME + (std::string)"0" + std::to_string(suffix)).c_str());
	sem[0] = sem_open(((std::string)SEM_NAME + (std::string)"0" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init[0]);
	if(!sem) perror("sem_open0");
	sem_unlink(((std::string)SEM_NAME + (std::string)"1" + std::to_string(suffix)).c_str());
	sem[1] = sem_open(((std::string)SEM_NAME + (std::string)"1" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init[1]);
	if(!sem) perror("sem_open1");
	sem_unlink(((std::string)SEM_NAME + (std::string)"2" + std::to_string(suffix)).c_str());
	sem[2] = sem_open(((std::string)SEM_NAME + (std::string)"2" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init[2]);
	if(!sem) perror("sem_open2");
	sem_unlink(((std::string)SEM_NAME + (std::string)"3" + std::to_string(suffix)).c_str());
	sem[3] = sem_open(((std::string)SEM_NAME + (std::string)"3" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init[3]);
	if(!sem) perror("sem_open3");
}

void gen_field(int seed, int size = SIZE){
	srand(seed);
	ofstream fout;
	fout.open("/tmp/field0");
	for(int i = 0; i < SIZE * SIZE; ++i)
		fout << (char)(rand() & 1);
	fout.close();
	fout.open("/tmp/field1");
	for(int i = 0; i < SIZE * SIZE; ++i)
		fout << (char)0;
	fout.close();
}

int main(int argc, char** argv){
	if(argc != 3){
		printf("Using: game_on_proc $N $SEED\n");
		printf("where N - number of iterations, SEED - seed for field generation\n");
		return 0;
	}
	int seed = std::stoi(argv[2]);
	int num = std::stoi(argv[1]);
	gen_field(seed);
	srand(time(0));
	int sems_names_suffix = rand();
	init_sems(sems_names_suffix);
	for(int i = 0; i < NUM_OF_PROCESSES; ++i){
		int pid = fork();
		if(!pid){
			work(i, num);
			return 0;
		}
	}
	sem_wait(sem[3]);
	if(SIZE > 50){
		cout << "OK\n";
		return 0;
	}
	ifstream fin;
	fin.open(((string)"/tmp/field" + std::to_string(num % 2)).c_str());
	int tmp;
	for(int i = 0; i < SIZE; ++i){
		for(int q = 0; q < SIZE; ++q){
			fin >> tmp;
			cout << tmp;
		}
		cout << "\n";
	}
	fin.close();
}