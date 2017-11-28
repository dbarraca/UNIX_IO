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
#define BUFFER_SIZE 80
#define TIME_FORM 8
#define CHILD_SLEEP 3

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

   usecs = 1000000 * (timestamp.tv_sec - start.tv_sec) + timestamp.tv_usec -
    start.tv_usec; // calculate time since start
   sprintf(timeStr, "0:%02ld:%03ld", usecs / 1000000, (usecs % 1000000) / 1000); // format time stamp string
}

int main(int argc, char **argv) {
   int pid = 1;
   int child;
   int childNum;
   int childPids[NUM_CHILD];
   ssize_t readResult = -1;
   char outBuf[BUFFER_SIZE];
   int outputFD;
   int messageLen;
   int message;

   // pipe variables
   int fd[NUM_CHILD][2];
   char timeFormat[TIME_FORM];
   char pipeReadBuf[BUFFER_SIZE];
   char pipeWriteBuf[BUFFER_SIZE];

   // select variables
   fd_set readfds;
   int numReady;
   struct timeval timeout;

   struct timeval start;
   struct timeval timestamp;

   // set timeout for select block interval
   timeout.tv_sec = 2;
   timeout.tv_usec = 0;


   gettimeofday(&start, NULL); // store time stamp for start

   for (child = 0; child < NUM_CHILD; child++) // count children
      if(pid != 0) { // check if child
         childNum = child; // store child number
         pipe(fd[child]); // make pipe to be used between child and parent
         pid = fork(); // fork to make a child
         childPids[child] =  pid; // store child pid
      }

   if (pid < 0) // check for fork error
      printf("unable to fork\n"); // print forking error
   else if (pid != 0) { // parent
      outputFD = open("output.txt", O_CREAT | O_RDWR | O_TRUNC, 0660); // open output file

      for (child = 0; child < NUM_CHILD; child++) // count children
         close(fd[child][1]); // close writing end of pipe

      do {
         FD_ZERO(&readfds); // initialize read file descriptors to zero

         for (child = 0; child < NUM_CHILD; child++) // count children
            FD_SET(fd[child][0], &readfds); // set fd set bit  for a child

         // check file descriptor for ready pipes to read
         numReady = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);

         if(numReady > 0) // check if there was something to read
            for (child = 0; child < NUM_CHILD; child++) // count children
               if(FD_ISSET(fd[child][0], &readfds) > 0) { // check if a child's file descriptor has something to read
                  memset(pipeReadBuf, 0, BUFFER_SIZE); // set buffer to zeros
                  readResult = read(fd[child][0], pipeReadBuf, BUFFER_SIZE); // read from pipe

                  if (readResult > 0) {// check if at end of pipe
                     gettimeofday(&timestamp, NULL); // get time stamp of current time
                     timeString(timeFormat, timestamp, start); // get string format of time
                     messageLen = sprintf(outBuf, "%s %s", timeFormat, pipeReadBuf); // prepend time stamp
                     write(outputFD, outBuf, messageLen); // write output file
//                     printf("%s\n",outBuf);
                  }
               }

         gettimeofday(&timestamp, NULL); // get time stamp of current time
      } while(readResult != 0 && (timestamp.tv_sec - start.tv_sec) < CHILD_LIFETIME); // repeat for 30 seconds

      for (child = 0; child < NUM_CHILD; child++) { // count children
         kill(childPids[child], SIGTERM); //  terminate child
         close(fd[child][0]); // close reading end of pipe
      }

      close(outputFD); // close output file
   }
   else { //child
      close(fd[childNum][0]); // close reading end of pipe

      srand(childNum); // initialize random number generator per child

      if (childNum < NUM_CHILD - 1) { // first four children
         for (message = 1; ; message++) { // count messages
            gettimeofday(&timestamp, NULL); // get time stamp of current time
            timeString(timeFormat, timestamp, start); // get string format of time
            sprintf(pipeWriteBuf, "%s: Child %d message %d\n", timeFormat, childNum + 1, message); // format child first message
            write(fd[childNum][1], pipeWriteBuf, strlen(pipeWriteBuf) + 1); // write to pipe connected to parent

            sleep(random() % 3); // child sleeps for random interval
         }
      }
      else { // last child
         while(readResult != 0) { // continue until end of file command
            printf("Enter a message: "); // print prompt
            fflush(stdout); // flush standard out
            readResult = read(STDIN_FILENO, pipeReadBuf, BUFFER_SIZE); // read from standard in

            if (readResult > 0) { // check if something to read
               gettimeofday(&timestamp, NULL); // get time stamp of current time
               timeString(timeFormat, timestamp, start); // get string format of time
               sprintf(pipeWriteBuf, "%s: Child 5: %s", timeFormat, pipeReadBuf); // prepend time to read in message
               write(fd[childNum][1], pipeWriteBuf, strlen(pipeWriteBuf) + 1); // write to pipe connected to parent
               memset(pipeReadBuf, 0, BUFFER_SIZE); // set buffer to zeros
            }
         }
      }
      close(fd[childNum][1]);  // close writing end of pipe
   }

   return 0;
}
