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

#define DEBUG ;    // ; for pass, printf for working

#define SEMNAME 1113

#define ACCESSF 0777

#define CHECK_DELAY 1000

#define INPUT_DELAY max(CHECK_DELAY * 2, 1)



int max(int a, int b){

  return (a < b ? b : a);

}



pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;



union semun {

    int val;

};





int setval(int sid, int member, int z)

{

    int semval;

    semun arg;

    arg.val = z;

    semval = semctl(sid, member, SETVAL, arg);

    return(semval);

}



int getval(int sid, int member)

{

    int semval;

    semval = semctl(sid, member, GETVAL, 0);

    return(semval);

}



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

      while(semlock(SEMID, INDEX));



      if(!active_flag) break;

      std::string tmp;

      std::getline(std::cin, tmp);

      int i;

      for(i = 0; i < tmp.size(); ++i)

        input[i] = tmp[i];

      input[tmp.size()] = '\0';

      DEBUG("GETTING INPUT %s\n", input);

      semfree(SEMID, INDEX);

      usleep(INPUT_DELAY);

      //pthread_mutex_unlock(&lock);

    }

}





char to_income[BUFSIZE];



int getmsg(int semid, int from){

  //("!0\n");

  //printf("getmsg\n");

  if(getval(semid, from + 3) != 0)

    return 0;

  //printf("!1\n");

  if(getval(semid, 2) == 0) return 0;

  DEBUG("TRY TO READ\n");

  int fd;

  if ( (fd = open(NAMEDPIPE_NAME, O_RDONLY)) <= 0 ) {

    perror("open");

    return 1;

  }

  //printf("!\n");

  DEBUG("OPENED\n");

  int i;

  while(!semlock(semid, 2)){

    //printf("!\n");

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

    //printf("!\n");

    while(1){

      usleep(CHECK_DELAY);

      //int err = pthread_mutex_trylock(&lock);

      if(!semlock(semid, index)){

        DEBUG("TRY TO WRITE\n");

        //semlock(semid, index);

        semlock(semid, index + 3);

        getmsg(semid, index ^ 1);

        semfree(semid, 2);

        int fd;

        //printf("!\n");

        if ( (fd = open(NAMEDPIPE_NAME, O_WRONLY)) <= 0 ) {

          perror("open");

          return 1;

        }

        DEBUG("WAITING OTHER SIDE\n");

        write(fd, input, strlen(input));

        while(getval(semid, 2) > 0);

        DEBUG("OK\n");

        close(fd);

        semfree(semid, index);

        semfree(semid, index + 3);

        //pthread_mutex_unlock(&lock);

        usleep(INPUT_DELAY * 2);

      }

      //printf("!\n");

      getmsg(semid, index ^ 1);

      //printf("STEP%d\n", i);

      ++i;

    }

}



int main (int argc, char ** argv) {

    user(argv[1][0] - '0');

    return 0;

}