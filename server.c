// C Program for Server
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <pthread.h>

// structure for message queue
struct mesg_buffer {
	long mesg_type;
	char mesg_text[100];
} message;

// shared memory initilizations
key_t key;
int shmBuf1id;
int (*buf1Ptr)[9];

void send_message(char text[])
{
	//It is a function for sending a message to the client.
    int msgid;
    // ftok to generate unique key
    key_t key = ftok("server.c", 65);

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    strcpy(message.mesg_text, text);
    // msgsnd to send message
    msgsnd(msgid, &message, sizeof(message), 0);

    // display the sent message
    printf("Message send is : %s \n", message.mesg_text);
}

char* receive_message()
{
	//It is a function for receiving a message to the client. 
	//It returns the message.(it is used for parse size message)
	int msgid;

	// ftok to generate unique key
	key_t key = ftok("client.c", 65);

	// msgget creates a message queue
	// and returns identifier
	msgid = msgget(key, 0666 | IPC_CREAT);

	// msgrcv to receive message
	msgrcv(msgid, &message, sizeof(message), 1, 0);

	// display the message
	printf("Received message is : %s \n",message.mesg_text);

	// to destroy the message queue
	msgctl(msgid, IPC_RMID, NULL);
	return message.mesg_text;
}

int **mulMat(size_t rows1,size_t cols1, int mat1[][cols1],size_t rows2,size_t cols2, int mat2[][cols2]) {
    
	//Matrix multiplication function. It calculates the result than returns.
 	int **rslt;
    rslt = malloc(sizeof(int*) * rows1);
     
    for(int i = 0; i < rows1; i++) {
        rslt[i] = malloc(sizeof(int*) * cols2);
    }
 
    for (int i = 0; i < rows1; i++) {
        for (int j = 0; j < cols2; j++) {
            rslt[i][j] = 0;
 
            for (int k = 0; k < rows2; k++) {
                rslt[i][j] += mat1[i][k] * mat2[k][j];
            }
 
        }
 
    }
	return rslt;
}

int **read_and_multiplicate(int rows, int cols, int rows2, int cols2)
{
	// This function reads the matrices from shared memory and multiplicates two matrices then returns the result matrix.
	int **result;
	key = ftok(".",'b');
    shmBuf1id = shmget(key,sizeof(int[9][9]),IPC_CREAT|0666);
    if(shmBuf1id == -1 )
    {  
        perror("shmget");
        exit(1);
    }
    else
    {  
        printf("Data read from memory:\n");
    	buf1Ptr = shmat(shmBuf1id,0,0);
		int matrix1[rows][cols];
    	int matrix2[rows2][cols2];
		//Matrix1
		for(size_t i = 0; i < rows; ++i)
		{
			for(size_t j = 0; j < cols; ++j)
			{
				matrix1[i][j]=buf1Ptr[i][j];
				printf("%d ", matrix1[i][j]);

			}
			puts("");
		}
		puts("");
		int k=0;
		int l=0;
		//Matrix2
		for(size_t i = rows; i < rows+rows2; ++i)
		{
			for(size_t j = 0; j < cols2; ++j)
			{
                matrix2[k][l] = buf1Ptr[i][j];
				printf("%d ", matrix2[k][l]);
				l++;

			}
			k++;
			l=0;
			puts("");
		}
		puts("");
		//Calculate the multiplication and print the result.
		printf("Multiplication of given two matrices is:\n");
		result = mulMat(rows,cols,matrix1,rows2,cols2,matrix2);
		
		for(int i = 0; i < rows; i++){
			for(int j = 0; j < cols2; j++){
				printf("%d ",result[i][j]);
			}
        	puts("");
    	}
        puts("");

        if(buf1Ptr == (void*) -1 )
        {  
			perror("shmat");
			exit(1);
        }
			
    }
	return result;
}

void write_matrix_to_shared_memory(size_t rows, size_t cols, int (*a)[cols])
{
    //Write the result matrix to shared memory.
	key = ftok(".",'b');
    shmBuf1id = shmget(key,sizeof(int[9][9]),IPC_CREAT|0666);

    if(shmBuf1id == -1 )
    {  
		
        perror("shmget");
        exit(1);
    }
    else
    {  
        buf1Ptr = shmat(shmBuf1id,0,0);
        for(size_t i = 0; i < rows; ++i)
        {
            for(size_t j = 0; j < cols; ++j){
                buf1Ptr[i][j] = *(a[i] + j);
			}
        }
        if(buf1Ptr == (void*) -1 )
        {  
            perror("shmat");
            exit(1);
        }
    }
	printf("Result matrix successfully written to shared memory.\n");

}

void *run(void *vargp)
{
	//This function starts the operations.

	//Receiving and parsing the sizes sent from the client as a message.
    char *sizes = receive_message();
	int rows1,cols1,rows2,cols2;
	rows1 = sizes[0] - '0', cols1 = sizes[1] - '0',rows2 = sizes[2] - '0',cols2 = sizes[3] - '0';

	//Reading, multiplying and returning matrices from shared memory using sizes.
	int **result;
	result = read_and_multiplicate(rows1,cols1,rows2,cols2);
	//Make the result suitable for the writing to shared memory.
	int result_matrix[rows1][cols2];
	for(int i = 0; i < rows1; i++){
		for(int j = 0; j < cols2; j++){
			result_matrix[i][j] = result[i][j];
		}
	}
	
	// Writing the result to shared memory
	write_matrix_to_shared_memory(rows1, cols2, result_matrix);

	//Send a message for the client.
	send_message("Result is written successfully.");
}

int main()
{
	pthread_t thread_id;
	
	while(1){
		printf("Waiting for message ... \n");
		pthread_create(&thread_id, NULL, run, NULL);
		pthread_join(thread_id, NULL);
		printf("Finished.\n\n");
	}
	

	return 0;
}
