#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

using std::cin;
using std::cout;
using std::ofstream;
using std::ifstream;
using std::string;
using std::min;

//#define SIZE 8000
#define NUM_OF_PROCESSES 8
#define SEM_NAME "/semaphore"
#define ACCESSF 0777

int SIZE = 100;

sem_t* sem[5];
int sem_init_[5] = {0, 0, NUM_OF_PROCESSES, 0, 1};
char* file_mem[2];

int ITERATION;
// get из it
// set в it^1
// процессы отработав ждут it

int Xshift;
int Yshift;

int get(int x, int y){
	x += Xshift;
	y += Yshift;
	if(x == -1) x = SIZE - 1;
	if(y == -1) y = SIZE - 1;
	if(x == SIZE) x = 0;
	if(y == SIZE) y = 0;
	//cout << x * SIZE + y << "\n";
	//cout << (int)(file_mem[ITERATION][x * SIZE + y]) << "\n";
	return file_mem[ITERATION][x * SIZE + y];
}

void set(int x, int y, int arg){
	//puts("!");
	//if(arg != 0) puts("!");
	x += Xshift;
	y += Yshift;
	if(x == -1) x = SIZE - 1;
	if(y == -1) y = SIZE - 1;
	if(x == SIZE) x = 0;
	if(y == SIZE) y = 0;
	file_mem[ITERATION ^ 1][x * SIZE + y] = arg;
	//cout << x * SIZE + y << "\n";
	//cout << file_mem[ITERATION ^ 1][x * SIZE + y] 
}

char* map_it(std::string name, int begin, int size, int* file_descr){
	int fd;
	//struct stat file_info;
	fd = open(name.c_str(), O_RDWR);
	*file_descr = fd;
	//fstat(fd, &file_info);
	return (char*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, begin * sizeof(char));
}

void print_field(int num){
	int size_ = min(SIZE, 20);

	/*
	if(SIZE > 50){
		cout << "OK\n";
		return;
	}
	*/
	int save[2];
	file_mem[0] = map_it("/tmp/field0", 0, SIZE*SIZE, save);
	file_mem[1] = map_it("/tmp/field1", 0, SIZE*SIZE, save + 1);
	for(int i = 0; i < size_; ++i){
		for(int q = 0; q < size_; ++q)
			cout << (file_mem[num % 2][i * SIZE + q] == 1 ? '#' : '-') << ' ';
		cout << "\n";
	}
	cout << "\n";
	close(save[0]);
	close(save[1]);

	return;

	//num = 0;
	ifstream fin;
	fin.open(((string)"/tmp/field" + std::to_string(num % 2)).c_str());
	char tmp;
	for(int i = 0; i < SIZE; ++i){
		for(int q = 0; q < SIZE; ++q){
			fin >> tmp;
			cout << (tmp == 0 ? '-' : '#');
		}
		cout << "\n";
	}
	cout << "\n";
	fin.close();
}

int alive(int x, int y)
{
	int al = 0;
	for(int i = -1; i <= 1; ++i)
		for(int q = -1; q <= 1; ++q)
			if(i || q)
				al += get(x + i, y + q);
	return al;
}

int recalc(int x, int y)
{
	int alive_ = alive(x, y);
	//cout << alive_ << "\n";
	//if(x == 1 && y == 2) cout << alive_ << "\n";
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
	Xshift = id * rows_num;
	Yshift = 0;
	bool main_ = 0;
	for(int i = 0; i < it_num; ++i){
		//cout << "start calc " << i << ' ' << id << "\n";
		ITERATION = i & 1;
		for(int i = 0; i < rows_num; ++i)
			for(int q = 0; q < SIZE; ++q)
				set(i, q, recalc(i, q));

		sem_wait(sem[4]);
		main_ = (getval(sem[2]) == 1);
		sem_wait(sem[2]);
		sem_post(sem[4]);

		if(main_){
			for(int i = 0; i < NUM_OF_PROCESSES; ++i)
				sem_post(sem[2]);
			print_field(ITERATION^1);
			//sleep(1);
			//getchar();
			cout << i << "\n";
			for(int i = 0; i < NUM_OF_PROCESSES - 1; ++i)
				sem_post(sem[ITERATION]);
		}else{
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
	sem[0] = sem_open(((std::string)SEM_NAME + (std::string)"0" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init_[0]);
	if(!sem) perror("sem_open0");
	sem_unlink(((std::string)SEM_NAME + (std::string)"1" + std::to_string(suffix)).c_str());
	sem[1] = sem_open(((std::string)SEM_NAME + (std::string)"1" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init_[1]);
	if(!sem) perror("sem_open1");
	sem_unlink(((std::string)SEM_NAME + (std::string)"2" + std::to_string(suffix)).c_str());
	sem[2] = sem_open(((std::string)SEM_NAME + (std::string)"2" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init_[2]);
	if(!sem) perror("sem_open2");
	sem_unlink(((std::string)SEM_NAME + (std::string)"3" + std::to_string(suffix)).c_str());
	sem[3] = sem_open(((std::string)SEM_NAME + (std::string)"3" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init_[3]);
	if(!sem) perror("sem_open3");
	sem_unlink(((std::string)SEM_NAME + (std::string)"4" + std::to_string(suffix)).c_str());
	sem[4] = sem_open(((std::string)SEM_NAME + (std::string)"4" + std::to_string(suffix)).c_str(), O_CREAT, ACCESSF, sem_init_[4]);
	if(!sem) perror("sem_open4");
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
	if(argc != 4){
		printf("Using: game_on_proc $N $SEED $FIELD_SIZE\n");
		printf("where N - number of iterations, SEED - seed for field generation\n");
		return 0;
	}
	int seed = std::stoi(argv[2]);
	int num = std::stoi(argv[1]);
	SIZE = std::stoi(argv[3]);
	gen_field(seed);
	print_field(num);
	srand(time(0));
	int sems_names_suffix = rand();
	init_sems(sems_names_suffix);
	for(int i = 0; i < NUM_OF_PROCESSES; ++i){
		int pid = fork();
		if(pid > 0){
			int save[2];
			file_mem[0] = map_it("/tmp/field0", 0, SIZE*SIZE, save);
			file_mem[1] = map_it("/tmp/field1", 0, SIZE*SIZE, save + 1);
			work(i, num);
			close(save[0]);
			close(save[1]);
			return 0;
		}
		if(pid < 0){
			cout << "ВСЕ ПЛОХО!!!\n";
			cout << "не выделено процесса\n";
			return 0;
		}
	}
	sem_wait(sem[3]);
	print_field(num);
}