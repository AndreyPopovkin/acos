#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <semaphore.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <unistd.h>



#define NAMEDPIPE_NAME "/tmp/fifo"
#define SEM_NAME "/semaphore"
#define BUFSIZE        1000
#define DEBUG ;                             // ; for pass, printf for working
#define ACCESSF 0777                        // права доступа при создании семафора/fifo
#define STANDART_FIFO_NAME 1111             // дополнение имени семафора, fifo

sem_t* sem[5];

void init_sems(int index_, int names_addition){//int index){
  if(index_ == 0){
      puts("init0\n");
      sem_unlink(((std::string)SEM_NAME + (std::string)"0" + std::to_string(names_addition)).c_str());
      sem[0] = sem_open(((std::string)SEM_NAME + (std::string)"0" + std::to_string(names_addition)).c_str(), O_CREAT, ACCESSF, 1);
      if(!sem) perror("sem_open0");
      sem_unlink(((std::string)SEM_NAME + (std::string)"1" + std::to_string(names_addition)).c_str());
      sem[1] = sem_open(((std::string)SEM_NAME + (std::string)"1" + std::to_string(names_addition)).c_str(), O_CREAT, ACCESSF, 0);
      if(!sem) perror("sem_open1");
      sem_unlink(((std::string)SEM_NAME + (std::string)"2" + std::to_string(names_addition)).c_str());
      sem[2] = sem_open(((std::string)SEM_NAME + (std::string)"2" + std::to_string(names_addition)).c_str(), O_CREAT, ACCESSF, 0);
      if(!sem) perror("sem_open2");
      sem_unlink(((std::string)SEM_NAME + (std::string)"3" + std::to_string(names_addition)).c_str());
      sem[3] = sem_open(((std::string)SEM_NAME + (std::string)"3" + std::to_string(names_addition)).c_str(), O_CREAT, ACCESSF, 0);
      if(!sem) perror("sem_open3");
      sem_unlink(((std::string)SEM_NAME + (std::string)"4" + std::to_string(names_addition)).c_str());
      sem[4] = sem_open(((std::string)SEM_NAME + (std::string)"4" + std::to_string(names_addition)).c_str(), O_CREAT, ACCESSF, 0);
      if(!sem) perror("sem_open4");
  } else {
      puts("init1\n");
      sem[0] = sem_open(((std::string)SEM_NAME + (std::string)"0" + std::to_string(names_addition)).c_str(), 0);
      if(!sem) perror("sem_open0");
      sem[1] = sem_open(((std::string)SEM_NAME + (std::string)"1" + std::to_string(names_addition)).c_str(), 0);
      if(!sem) perror("sem_open1");
      sem[2] = sem_open(((std::string)SEM_NAME + (std::string)"2" + std::to_string(names_addition)).c_str(), 0);
      if(!sem) perror("sem_open2");
      sem[3] = sem_open(((std::string)SEM_NAME + (std::string)"3" + std::to_string(names_addition)).c_str(), 0);
      if(!sem) perror("sem_open3");
      sem[4] = sem_open(((std::string)SEM_NAME + (std::string)"4" + std::to_string(names_addition)).c_str(), 0);
      if(!sem) perror("sem_open4");
  }
}

int max(int a, int b){
  return (a < b ? b : a);
}

int min(int a, int b){
  return (a < b ? a : b);
}

int getval(sem_t* s){
  int a;
  sem_getvalue(s, &a);
  return a;
}

std::string pipe_name;
int fd;
int index_;

void* waiting_for_input(void*){
    while(1){
      std::string s;
      std::getline(std::cin, s);
      if(s == "") continue;
      sem_wait(sem[0]);            // занимает fifo на запись
      puts("print");
      write(fd, s.c_str(), s.size());
      sem_post(sem[2 - index_]);   // сигнализуем оппоненту о сообщении
      sem_wait(sem[4 - index_]);   // ждем пока прочтет
      sem_post(sem[0]);            // освобождаем fifo
      puts("~print");
    }
}

void print_sems(){
    for(int i = 0; i < 5; ++i)
      std::cout << getval(sem[i]) << " ";
    puts("");
}

int main (int argc, char ** argv) {
    if(argc != 3 || strlen(argv[1]) != 1 || (argv[1][0] < '0' || argv[1][0] > '2')){
      printf("%d\n", argc);
      printf(argv[1]);
      printf(argv[2]);
      printf("wrong parametres\n");
      printf("Using: chat 0 $id, for main user(server)\n");
      printf("chat 1 $id, for second user\n");
      printf("id - seed for fifo and semaphores creating/connecting, 0 for std\n");   
      return 0; 
    }

    index_ = argv[1][0] - '0';
    int sem_name_suffix = std::stoi(std::string(argv[2]));
    if(sem_name_suffix = 0) sem_name_suffix = STANDART_FIFO_NAME;
    init_sems(index_, sem_name_suffix);
    pipe_name = (std::string)NAMEDPIPE_NAME + std::to_string(sem_name_suffix);

    if (mkfifo(pipe_name.c_str(), ACCESSF) ) {
        perror("mkfifo");
        printf("it's normal if file already exist\n");
        printf("otherside please stop the program, or maybe it will drop itself)\n");
    }
    char buf[BUFSIZE];

    if ( (fd = open(pipe_name.c_str(), O_RDWR)) <= 0 ) {
      perror("open");
      return 1;
    }


    pthread_t thread;
    int status = pthread_create(&thread, NULL, waiting_for_input, 0);
    while(1){
      //std::cout << sem[index_ + 1] << ' ' << index_ + 1 << "\n";  
      sem_wait(sem[index_ + 1]);    // висим, пока не придет сигнал о сообщении
      print_sems();
      puts("read");
      int tmp = read(fd, buf, BUFSIZE - 1);
      if(tmp <= 0){
        perror("read");
        return 0;
      }
      buf[tmp] = 0;
      std::cout << (buf) << "\n";
      sem_post(sem[index_ + 3]);    // отчитываемся о принятии сообщения
      puts("~read");
    }
}