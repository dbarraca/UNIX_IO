#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>

#define NUM_CHILD 5
#define CHILD_LIFETIME 30
#define BUFFER_SIZE 32

/**
 * Gives String representation of the time between start of program execution and the
 * given time stamp.
 *
 * param timeStr Pointer to where string is to be stored
 * param timestamp The time of a message event
 * param start Time of the start of program execution
 *
 * returns String of time between start and message event
 */
void timeString(char *timeStr, struct timeval timestamp, struct timeval start) {
   long int usecs;

   usecs = 1000000 * (timestamp.tv_sec - start.tv_sec)
    + timestamp.tv_usec - start.tv_usec;

   sprintf(timeStr, "0:%02ld:%03ld", usecs / 1000000,(usecs % 1000000) / 1000);
}

int main(int argc, char **argv) {
   int pid = 1;
   int child;
   int childNum;
   int childSleep;
   ssize_t readResult = -1;

   // pipe variables
   int fd[NUM_CHILD][2];
   char timeFormat[8];
   char pipeReadBuf[BUFFER_SIZE];
   char pipeWriteBuf[BUFFER_SIZE];

   // select variables
   fd_set readfds;
   int numReady;
   struct timeval timeout;

   struct timeval start;
   struct timeval timestamp;

   char outBuf[BUFFER_SIZE];
   int outputFD;
   int messageLen;
   int childPids[NUM_CHILD];

   // set timeout for select block interval
   timeout.tv_sec = 2;
   timeout.tv_usec = 0;

   srand(0); // initialize random number generator
   gettimeofday(&start, NULL); // get time stamp of current time

   for (child = 0; child < NUM_CHILD; child++) // count children
      if(pid != 0) { // check if child
         childSleep = random() % 3; // set random sleep time for child
         childNum = child; // store child number
         pipe(fd[child]); // make pipe to be used between child and parent
         pid = fork(); // fork to make a child
         childPids[child] =  pid;
      }

   if (pid < 0)
      printf("unable to fork\n");
   else if (pid != 0) { // parent
      outputFD = open("output.txt", O_CREAT | O_RDWR | O_TRUNC, 0660); // open output file

      for (child = 0; child < NUM_CHILD; child++) // count children
         close(fd[child][1]); // close writing end of pipe

      do {
         FD_ZERO(&readfds); // initialize read file descriptors to zero

         for (child = 0; child < NUM_CHILD; child++) // count children
            FD_SET(fd[child][0], &readfds); // set bit a file descriptor for a child

         // check file descriptor for ready pipes to read
         numReady = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         if(numReady > 0) // check if there was something to read
            for (child = 0; child < NUM_CHILD; child++) // count children
               if(FD_ISSET(fd[child][0], &readfds) > 0) { // check if a child's file descriptor has something to read
                  readResult = read(fd[child][0], pipeReadBuf, BUFFER_SIZE); // read from pipe

                  if (readResult > 0) {// check if at end of pipe
                     gettimeofday(&timestamp, NULL);
                     timeString(timeFormat, timestamp, start);
                     messageLen = sprintf(outBuf, "%s %s\n", timeFormat, pipeReadBuf);
                     write(outputFD, outBuf, messageLen);
                  }
               }

         gettimeofday(&timestamp, NULL);
      } while(readResult != 0 && (timestamp.tv_sec - start.tv_sec) < CHILD_LIFETIME); // stop checking pipe when there is nothing to read

      for (child = 0; child < NUM_CHILD; child++) { // count children
         kill(childPids[child], SIGTERM);
         close(fd[child][0]); // close reading end of pipe
      }

      close(outputFD);
   }
   else {  //child
      close(fd[childNum][0]); // close reading end of pipe

      if (childNum < 4) {
         gettimeofday(&timestamp, NULL);
         timeString(timeFormat, timestamp, start);
         sprintf(pipeWriteBuf, "%s Child %d Message 1", timeFormat, childNum + 1);
         write(fd[childNum][1], pipeWriteBuf, strlen(pipeWriteBuf) + 1);

         sleep(childSleep);

         gettimeofday(&timestamp, NULL);
         timeString(timeFormat, timestamp, start);
         sprintf(pipeWriteBuf, "%s Child %d Message 2", timeFormat, childNum + 1);
         write(fd[childNum][1], pipeWriteBuf, strlen(pipeWriteBuf) + 1);

      }
      else {
         while(readResult != 0) {
            printf("Enter a message: ");
            fflush(stdout);
            readResult = read(STDIN_FILENO, pipeReadBuf, BUFFER_SIZE); // read from standard in

            if (readResult > 0) {
               gettimeofday(&timestamp, NULL);
               timeString(timeFormat, timestamp, start);
               sprintf(pipeWriteBuf, "%s %s", timeFormat, pipeReadBuf);
               write(fd[childNum][1], pipeWriteBuf, strlen(pipeWriteBuf) + 1);
               memset(pipeReadBuf, 0, BUFFER_SIZE);
            }
         }
      }
      close(fd[childNum][1]);
   }

   return 0;
}
