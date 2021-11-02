// C Program for Client
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#define MAX 50

// shared memory initilizations
key_t key;
int shmBuf1id;
int (*buf1Ptr)[9];

// structure for message queue
struct mesg_buffer {
	long mesg_type;
	char mesg_text[100];
} message;

void send_message(char text[])
{
    //It is a function to send a message to server. Message content is given as parameter.
    int msgid;
    // ftok to generate unique key
    key_t key = ftok("client.c", 65);

    // msgget creates a message queue
    // and returns identifier
    msgid = msgget(key, 0666 | IPC_CREAT);
    message.mesg_type = 1;

    strcpy(message.mesg_text, text);
    // msgsnd to send message
    msgsnd(msgid, &message, sizeof(message), 0);

    // display the message
    printf("Message send is : %s \n", message.mesg_text);
}

void receive_message()
{
    //It is a function for receive the messages that is sent by server.
	int msgid;

	// ftok to generate unique key
	key_t key = ftok("server.c", 65);

	// msgget creates a message queue
	// and returns identifier
	msgid = msgget(key, 0666 | IPC_CREAT);

	// msgrcv to receive message
	msgrcv(msgid, &message, sizeof(message), 1, 0);

	// display the message
	printf("Received message is : %s \n",message.mesg_text);

	// to destroy the message queue
	msgctl(msgid, IPC_RMID, NULL);
}

int get_matrix_sizes(int *rows,int *cols, const char* filename)
{

    //Read the matrix size from the file given as parameter.Then assign the values to parameter rows and cols. (Pass-by-reference)
    FILE *pf;
    pf = fopen (filename, "r");
    if (pf == NULL)
        return 0;

    fscanf(pf, "%d",rows);
    fscanf(pf, "%d",cols);
    fclose (pf); 
    return 1; 
}

int check_multiplicability(int cols1,int rows2)
{
    //It checks the multiplicability of two matrices using number of columns of matrix1 and number of rows of matrix2.
    //If they are suitable then funtion returns 1 o.w. 0
    if( cols1 != rows2 )
    {
       return 0;
    }
    return 1;
}

int read_matrix_from_file(size_t rows, size_t cols, int (*a)[cols], const char* filename)
{
    // Bu fonksiyon dosyadan matrix'leri okuyup pass by ref seklinde gelen 2d array'a atar.
    FILE *pf;
    pf = fopen (filename, "r");
    if (pf == NULL)
        return 0;
    //size degerlerini ignore
    int temp;
    fscanf(pf, "%d",&temp);
    fscanf(pf, "%d",&temp);
    for(size_t i = 0; i < rows; ++i)
    {
        for(size_t j = 0; j < cols; ++j)
            fscanf(pf, "%d", a[i] + j);
    }

    fclose (pf); 
    return 1; 
}

void write_matrices_to_shared_memory(size_t rows, size_t cols, int (*a)[cols], size_t rows2, size_t cols2, int (*b)[cols2])
{
    //Function that puts two matrices read from the file into shared memory in a single buffer.
    int key = ftok(".",'b');
    shmBuf1id = shmget(key,sizeof(int[9][9]),IPC_CREAT|0666);

    if(shmBuf1id == -1 )
    {  
        perror("shmget");
        exit(1);
    }
    else
    {  
        buf1Ptr = shmat(shmBuf1id,0,0);
        //Matrix 1
        for(size_t i = 0; i < rows; ++i)
        {
            for(size_t j = 0; j < cols; ++j)
                buf1Ptr[i][j] = *(a[i] + j);
        }
        //Matrix 2
        for(size_t i = rows; i < rows+rows2; ++i)
        {
            for(size_t j = 0; j < cols2; ++j)
                buf1Ptr[i][j] = *(b[i-rows] + j);
        }

        if(buf1Ptr == (void*) -1 )
        {  
            perror("shmat");
            exit(1);
        }
    }
    //Printing info
    printf("Input matrices successfully written to shared memory.\n");
     
}

void read_shared_memory(int rows, int cols, int rows2, int cols2)
{
    //It allows reading and printing the result matrix written to the shared memory
	key = ftok(".",'b');
    shmBuf1id = shmget(key,sizeof(int[9][9]),IPC_CREAT|0666);
    if(shmBuf1id == -1 )
    {  
        perror("shmget");
        exit(1);
    }
    else
    {  
        //Printing the result matrix.
        printf("Result matrix :\n");
    	buf1Ptr = shmat(shmBuf1id,0,0);

		for(size_t i = 0 ; i < rows; ++i)
		{
			for(size_t j = 0; j < cols2; ++j)
			{
				printf("%d ", buf1Ptr[i][j]);
			}
			puts("");
		}

        if(buf1Ptr == (void*) -1 )
        {  
			perror("shmat");
			exit(1);
        }
			
    }
}

int main()
{
    char FILE1[MAX];
    char FILE2[MAX];
    //Take the input and remove the newline charachter.
    printf("First file name : ");
    fgets(FILE1, MAX, stdin);
    FILE1[strcspn(FILE1, "\n")] = 0;

    printf("Second file name : ");
    fgets(FILE2, MAX, stdin);
    FILE2[strcspn(FILE2, "\n")] = 0;

    //Get matrix sizes.
    int rows1, cols1;
    int rows2, cols2;

    get_matrix_sizes(&rows1,&cols1,FILE1);
    get_matrix_sizes(&rows2,&cols2,FILE2);

    //Check multiplicability.
    if(!check_multiplicability(cols1,rows2)){
        printf("Two matrices cannot be multiplied !!!\n");
        exit(1);
    }

    //Read the matrices from files.
    int matrix1[rows1][cols1];
    int matrix2[rows2][cols2];

    read_matrix_from_file(rows1, cols1, matrix1, FILE1);
    read_matrix_from_file(rows2, cols2, matrix2, FILE2);
    
    // Create a sizes string to send to the server.
    char str[4];
    str[0] = rows1+'0', str[1] = cols1+'0', str[2] = rows2+'0', str[3] = cols2+'0';
    
    //Write the matrices to shared memory and send the sizes to server. Then server starts the operations.
    write_matrices_to_shared_memory(rows1,cols1,matrix1,rows2,cols2,matrix2);
    send_message(str);

    //To wait a message from server to realize the calculation is finish.
    receive_message();

    //Reading the result matrix from the shared memory when a message is received from the server that the operations are completed.
    read_shared_memory(rows1,cols1,rows2,cols2);
    //It is also print the result matrix.

	return 0;
}

