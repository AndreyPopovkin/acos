#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/sem.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>

#define NAMEDPIPE_NAME "/my_named_pipe3"
#define BUFSIZE        1000
#define DEBUG ;                             // ; for pass, printf for working
#define SEMNAME 1113                        // имя (номер) семафора 
#define ACCESSF 0777                        // права доступа при создании семафора
#define CHECK_DELAY 1000                    // задержка между проверками о пришедших сообщениях
#define INPUT_DELAY max(CHECK_DELAY * 2, 1) // задержка приема сообщений должна быть \
                                            // больше CHECK_DELAY, иначе сообщения будут пропадать

int max(int a, int b){
  return (a < b ? b : a);
}

int min(int a, int b){
  return (a < b ? a : b);
}

union semun {
    int val;
};

// выставляет семафору member из группы sid значение z
int setval(int sid, int member, int z)
{
    int semval;
    semun arg;
    arg.val = z;
    semval = semctl(sid, member, SETVAL, arg);
    return(semval);
}

// выбирает значение семафора member из группы sid
int getval(int sid, int member)
{
    int semval;
    semval = semctl(sid, member, GETVAL, 0);
    return(semval);
}

// блокирует единицу ресурса семафора (вычитает 1 и возвращает 0, а если не может, то возвращает 1)
int semlock(int sid, int member){
  sembuf comm[1];
  comm[0].sem_num = member;
  comm[0].sem_op = -1;
  comm[0].sem_flg = IPC_NOWAIT;
  if(semop(sid, comm, 1)){
    //perror("semop");
    return 1;
  }
  return 0;
}

// освободает единицу ресурса (прибавляет 1)
int semfree(int sid, int member){
  sembuf comm[1];
  comm[0].sem_num = member;
  comm[0].sem_op = 1;
  comm[0].sem_flg = IPC_NOWAIT;
  if(semop(sid, comm, 1)){
    perror("semop");
    return 1;
  }
  return 0;
}

char input[BUFSIZE];
bool active_flag = 1;
int SEMID;
int INDEX;

#include <string>

void* waiting_for_input(void*){
    while(1){
      //pthread_mutex_lock(&lock);
      while(semlock(SEMID, INDEX)) usleep(1);

      if(!active_flag) break;
      std::string tmp;
      std::getline(std::cin, tmp);
      int i;
      tmp.resize(min(tmp.size(), BUFSIZE - 1));
      for(i = 0; i < tmp.size(); ++i)
        input[i] = tmp[i];
      input[tmp.size()] = '\0';
      DEBUG("GETTING INPUT %s\n", input);
      if(tmp == "-exit") active_flag = 1;
      semfree(SEMID, INDEX);
      usleep(INPUT_DELAY);
      //pthread_mutex_unlock(&lock);
    }
}

char to_income[BUFSIZE];

int getmsg(int semid, int from){
  if(getval(semid, from + 3) != 0)
    return 0;
  if(getval(semid, 2) == 0) return 0;
  DEBUG("TRY TO READ\n");
  int fd;
  if ( (fd = open(NAMEDPIPE_NAME, O_RDONLY)) <= 0 ) {
    perror("open");
    return 1;
  }
  DEBUG("OPENED\n");
  int i;
  while(!semlock(semid, 2)){
    for(i = 0; i < BUFSIZE; ++i)
      to_income[i] = '\0';
    read(fd, to_income, BUFSIZE - 1);
    printf("income: %s\n", to_income);
  }
  close(fd);
}

int runDaemon(){
  int semid = semget(SEMNAME, 5, ACCESSF);
    if(semid == -1)
      semid = semget(SEMNAME, 5, IPC_CREAT | IPC_EXCL | ACCESSF);
  while(1){
    usleep(300000);
    printf("%d%d%d%d%d\n", 
      getval(semid, 0), 
      getval(semid, 1), 
      getval(semid, 2),
      getval(semid, 3),
      getval(semid, 4)
    );
  }
}

int user(int index)  // index == 0, 1
{
    if(index == 2) {
      runDaemon();
      return 0;
    }

    int semid = semget(SEMNAME, 5, ACCESSF);
    if(semid == -1)
      semid = semget(SEMNAME, 5, IPC_CREAT | IPC_EXCL | ACCESSF);
    setval(semid, index, 1);
    setval(semid, index + 3, 1);
    if(index == 0) 
      setval(semid, 2, 0);

    INDEX = index;
    SEMID = semid;

    if ( mkfifo(NAMEDPIPE_NAME, 0777) ) {
        perror("mkfifo");
    }
    char buf[BUFSIZE];


    pthread_t thread;
    int i = 0;
    int status = pthread_create(&thread, NULL, waiting_for_input, 0);
    while(active_flag){
      usleep(CHECK_DELAY);
      if(!semlock(semid, index)){
        DEBUG("TRY TO WRITE\n");
        semlock(semid, index + 3);
        getmsg(semid, index ^ 1);
        semfree(semid, 2);
        int fd;
        if ( (fd = open(NAMEDPIPE_NAME, O_WRONLY)) <= 0 ) {
          perror("open");
          return 1;
        }
        DEBUG("WAITING FOR ANOTHER SIDE\n");
        write(fd, input, strlen(input));
        while(getval(semid, 2) > 0);
        DEBUG("OK\n");
        close(fd);
        semfree(semid, index);
        semfree(semid, index + 3);
        usleep(INPUT_DELAY * 2);
      }
      getmsg(semid, index ^ 1);
      ++i;
    }
}

int main (int argc, char ** argv) {
    user(argv[1][0] - '0');
    return 0;
}