#include <sys/shm.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "msg.h"    /* For the message struct */
#include <sys/stat.h>
#include <iostream>

/* The size of the shared memory segment */
#define SHARED_MEMORY_CHUNK_SIZE 1000

/* The ids for the shared memory segment and the message queue */
int shmid, msqid;

/* The pointer to the shared memory */
void* sharedMemPtr;

/**
 * Sets up the shared memory segment and message queue
 * @param shmid - the id of the allocated shared memory
 * @param msqid - the id of the allocated message queue
 */
void init(int& shmid, int& msqid, void*& sharedMemPtr)
{
	/* TODO:
        1. Create a file called keyfile.txt containing string "Hello world" (you may do
 	    so manually or from the code).
	2. Use ftok("keyfile.txt", 'a') in order to generate the key.
	3. Use will use this key in the TODO's below. Use the same key for the queue
	   and the shared memory segment. This also serves to illustrate the difference
 	   between the key and the id used in message queues and shared memory. The key is
	   like the file name and the id is like the file object.  Every System V object
	   on the system has a unique id, but different objects may have the same key.
	*/
	key_t key;
	if((key = ftok("keyfile.txt", 'a')) == -1)
	{
		perror("Error getting key");
		exit(1);
	}
	else
	{
		std::cout << "Generated the key" << std::endl;
	}

	/* TODO: Get the id of the shared memory segment. The size of the segment must be SHARED_MEMORY_CHUNK_SIZE */
	/* TODO: Attach to the shared memory */
	/* TODO: Attach to the message queue */
	/* Store the IDs and the pointer to the shared memory region in the corresponding function parameters */
	shmid = shmget(key, SHARED_MEMORY_CHUNK_SIZE, S_IRUSR | S_IWUSR);
	if(shmid == -1)
	{
		perror("Error getting id of shared memory segment");
		exit(1);
	}
	else
	{
		std::cout << "Attached id with pointer" << std::endl;
	}

	sharedMemPtr = shmat(shmid, NULL, 0);
	if(sharedMemPtr == (void*)-1)
	{
		perror("Error attaching to share memory with pointer");
		exit(1);
	}
	//creates a message queue with key "key" if the queue exists
	//returns the id of the created queue
	msqid = msgget(key, S_IRUSR | S_IWUSR);
	std::cout << "This is the message queue id we linked to: " << msqid << std::endl;
}

/**
 * Performs the cleanup functions
 * @param sharedMemPtr - the pointer to the shared memory
 * @param shmid - the id of the shared memory segment
 * @param msqid - the id of the message queue
 */
void cleanUp(const int& shmid, const int& msqid, void* sharedMemPtr)
{
	/* TODO: Detach from shared memory */
	shmdt(sharedMemPtr);
}

/**
 * The main send function
 * @param fileName - the name of the file
 * @return - the number of bytes sent
 */
unsigned long sendFile(const char* fileName)
{

	/* A buffer to store message we will send to the receiver. */
	message sndMsg;

	/* A buffer to store message received from the receiver. */
	message rcvMsg;

	/* The number of bytes sent */
	unsigned long numBytesSent = 0;

	/* Open the file */
	FILE* fp =  fopen(fileName, "r");

	/* Was the file open? */
	if(!fp)
	{
		perror("fopen");
		exit(-1);
	}

	/* Read the whole file */
	while(!feof(fp))
	{
		/* Read at most SHARED_MEMORY_CHUNK_SIZE from the file and
 		 * store them in shared memory.  fread() will return how many bytes it has
		 * actually read. This is important; the last chunk read may be less than
		 * SHARED_MEMORY_CHUNK_SIZE.
 		 */
		if((sndMsg.size = fread(sharedMemPtr, sizeof(char), SHARED_MEMORY_CHUNK_SIZE, fp)) < 0)
		{
			perror("fread");
			exit(-1);
		}

		/* TODO: count the number of bytes sent. */
		numBytesSent = sizeof(sndMsg);
		/* TODO: Send a message to the receiver telling him that the data is ready
 		 * to be read (message of type SENDER_DATA_TYPE).
 		 */
		sndMsg.mtype = 1;
		msgsnd(msqid, &sndMsg, sizeof(message) - sizeof(long), 0);
		/* TODO: Wait until the receiver sends us a message of type RECV_DONE_TYPE telling us
 		 * that he finished saving a chunk of memory.
 		 */
		msgrcv(msqid, &rcvMsg, sizeof(message) - sizeof(long), 2, 0);
	}


	/** TODO: once we are out of the above loop, we have finished sending the file.
 	  * Lets tell the receiver that we have nothing more to send. We will do this by
 	  * sending a message of type SENDER_DATA_TYPE with size field set to 0.
	  */
	sndMsg.mtype = 1;
	sndMsg.size = 0;
	msgsnd(msqid, &sndMsg, sizeof(message) - sizeof(long), 0);

	/* Close the file */
	fclose(fp);

	return numBytesSent;
}

/**
 * Used to send the name of the file to the receiver
 * @param fileName - the name of the file to send
 */
void sendFileName(const char* fileName)
{
	/* Get the length of the file name */
	int fileNameSize = strlen(fileName);

	/* TODO: Make sure the file name does not exceed
	 * the maximum buffer size in the fileNameMsg
	 * struct. If exceeds, then terminate with an error.
	 */
	if(fileNameSize > 100)
	{
		perror("File exceeds maximum buffer size");
		exit(-1);
	}
	/* TODO: Create an instance of the struct representing the message
	 * containing the name of the file.
	 */
	fileNameMsg msg;
	/* TODO: Set the message type FILE_NAME_TRANSFER_TYPE */
	msg.mtype = FILE_NAME_TRANSFER_TYPE;
	/* TODO: Set the file name in the message */
	strncpy(msg.fileName, fileName, MAX_FILE_NAME_SIZE);
	std::cout << "This is the filename we are sending out: " << msg.fileName << std::endl;
	/* TODO: Send the message using msgsnd */
	msgsnd(msqid, &msg, sizeof(fileNameMsg) - sizeof(long), 0);
}


int main(int argc, char** argv)
{

	/* Check the command line arguments */
	if(argc < 2)
	{
		fprintf(stderr, "USAGE: %s <FILE NAME>\n", argv[0]);
		exit(-1);
	}

	/* Connect to shared memory and the message queue */
	init(shmid, msqid, sharedMemPtr);

	/* Send the name of the file */
        sendFileName(argv[1]);

	/* Send the file */
	fprintf(stderr, "The number of bytes sent is %lu\n", sendFile(argv[1]));

	/* Cleanup */
	cleanUp(shmid, msqid, sharedMemPtr);

	return 0;
}
