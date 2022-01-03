# C Multithreading
 
Subjects : Unix/Linux IPC, C, Mailbox, Multithreading, Pthreads, Shared memory

There is a server program for matrix multiplication. Server will take the matrices and return the multiplication matrices back to the client.

Run

```
gcc -pthread server.c -o server  
./server

gcc client.c -o client  
./client
```
