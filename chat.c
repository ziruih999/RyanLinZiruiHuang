/*
get help with Programming with UDP sockets
reference : https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html

get help with multithreading programming
reference: https://www.geeksforgeeks.org/multithreading-c-2/
reference: http://zhangxiaoya.github.io/2015/05/15/multi-thread-of-c-program-language-on-linux/
*/


#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "list.c"
#include "list.h"

#define MSG_MAX_LEN 1024



// ***** global variables block *****

// local socket
int my_socket;

// socket addresses
struct sockaddr_in my_addr, peer_addr;

// declare threads
pthread_t keyboard, screen, receiveData, sendData;

// Lists
List* sendList; //shared by input & send
List* printList; //shared by output & receive

// mutexs 
pthread_mutex_t mutex_send = PTHREAD_MUTEX_INITIALIZER; //for sendList (consumer:send, producer:input)
pthread_mutex_t mutex_print = PTHREAD_MUTEX_INITIALIZER; //for printList (consumer:output, producer:receive)

// condition variables
pthread_cond_t cond_inputWait = PTHREAD_COND_INITIALIZER; // if more message can be added to sendList?
pthread_cond_t cond_outputWait = PTHREAD_COND_INITIALIZER; // if any message in the printList?
pthread_cond_t cond_senderWait = PTHREAD_COND_INITIALIZER; // if any message in the sendList?
pthread_cond_t cond_receiverWait = PTHREAD_COND_INITIALIZER; // if more message can be added to printList?

// turnoff onChat when chat terminate
bool onChat = true; 

/*
    goal: get message from input and add that message to sendList
    1. lock access to sendList when enter
    2. get message from keyboard
    3. if sendList is full, wait until send thread wake it up
    4. add message to sendList
    5. if message is '!':
            no more message to be received: cancel receive and output thread
*/
void *input_keyboard() {

    char msg[MSG_MAX_LEN];
    while (onChat){
        // lock access to sendList
        pthread_mutex_lock(&mutex_send);

        // get message from keyboard
        // https://stackoverflow.com/a/22065708
        memset(msg, '\0', MSG_MAX_LEN);
        printf("message: ");
        fgets(msg, MSG_MAX_LEN, stdin);
        msg[strlen(msg)-1] = '\0';

        // add message to sendList, then wait until send thread send it out
        List_add(sendList,msg);
        pthread_cond_wait(&cond_inputWait, &mutex_send);

        // printf("length of sendList = %d \n",List_count(sendList));
        // if input message is '!', I want to terminate the conversation
        if (msg[0] == '!' && msg[1] == '\0'){
            printf("...... terminating request by ME ......\n");
            // ready to return to main thread
            // unlock access to printList(send thread only)
            pthread_mutex_unlock(&mutex_send);
            pthread_cond_signal(&cond_senderWait);
            // turnoff onChat, no more input
            onChat = false;
            // destroy condition variables and mutex
            pthread_mutex_destroy(&mutex_send);
            pthread_mutex_destroy(&mutex_print);
            pthread_cond_destroy(&cond_inputWait);
            pthread_cond_destroy(&cond_outputWait);
            pthread_cond_destroy(&cond_senderWait);
            pthread_cond_destroy(&cond_receiverWait);
            // cancel threads
            pthread_cancel(receiveData);
            pthread_cancel(screen);
            pthread_cancel(sendData);

            
            close(my_socket);
            pthread_exit(0);
        }
        else{
            pthread_mutex_unlock(&mutex_send);
            pthread_cond_signal(&cond_senderWait);
        }
    }
    pthread_exit(0);
}

/*
    goal: get a message from printList and print it to screen
    1. if printList is empty, wait until receive thread wake it up
    2. get the message from printList
    3. print message to screen
*/

void *output_screen() {
    char msg[MSG_MAX_LEN];

    while(onChat){
        // block access to printList
        pthread_mutex_lock(&mutex_print);
        // if no message in printList
        // wait until receive thread 
        while(List_count(printList) == 0){
            pthread_cond_wait(&cond_outputWait, &mutex_print);
        }
        // copy and remove the first message from printList
        strcpy(msg, List_first(printList));
        List_remove(printList);

        // if it's a termination message
        if (msg[0] == '!' && msg[1]== '\0'){
            printf("......termination request by REMOTE-USER......\n");
        
            // destroy condition variables and mutex
            pthread_mutex_destroy(&mutex_send);
            pthread_mutex_destroy(&mutex_print);
            pthread_cond_destroy(&cond_inputWait);
            pthread_cond_destroy(&cond_outputWait);
            pthread_cond_destroy(&cond_senderWait);
            pthread_cond_destroy(&cond_receiverWait);
        }
        // leave Critical Section, unlock access to printList
        // if receive thread is waiting, signal it
        pthread_mutex_unlock(&mutex_print);
        pthread_cond_signal(&cond_receiverWait);
        printf("Remote User: %s\n", msg);
    }
   pthread_exit(0);
}


/*
    goal: get a message from sendList, then send it to remote user
    1. lock the access to sendList when enter
    2. if sendList is empty, wait until input thread wake it up
    3. get a message from sendList
    4. send message to remote user
    5. if message = '!':
        destroy CV and mutex
        turn off onChat
*/
void *send_data(void *remaddr) {
    char msg[MSG_MAX_LEN];
    unsigned int sin_len = sizeof(peer_addr);
    while(1){
        // lock access to sendList
        pthread_mutex_lock(&mutex_send);
        // if nothing in sendList, wait
        while(List_count(sendList) == 0){
            pthread_cond_wait(&cond_senderWait, &mutex_send);
        }
        // copy and remove the first message from sendList
        strcpy(msg, List_first(sendList));
        List_remove(sendList);

        // leave Critical Section, unlock access to sendList
        // if input thread is waiting, signal it
        pthread_mutex_unlock(&mutex_send);
        pthread_cond_signal(&cond_inputWait);
        // send message to remote use
        if (sendto(my_socket, msg, strlen(msg), 0, (struct sockaddr *) &peer_addr, sin_len) < 0){
            perror ("sento failed");
            exit(1);
        }   
    }
    printf("send going to exit\n");
    pthread_exit(0);
}


/*
    goal: receive message from socket, and add it to printList
    lock acess to printList at the time of entering,
    if printList is full, wait until output thread wake it up
    receive message
    if message == '!' : close the socket
    add message to printList
*/

void *receive_data(void *remaddr) {
    char msg[MSG_MAX_LEN];
    int addrlen = sizeof(peer_addr);
    while (onChat){
        pthread_mutex_lock(&mutex_print);

        // get message 
        int bytesRx = recvfrom(my_socket, msg, MSG_MAX_LEN, 0, (struct sockaddr *) &peer_addr, &addrlen);
        // Make it null terminated (so string functions work):
        int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
		msg[terminateIdx] = 0;

        // if msg[0] --> remote user want to terminate
        if (msg[0] == '!' && msg[1] == '\0'){
            onChat = false;
            // cancel threads and socket
            pthread_cancel(sendData);
            pthread_cancel(receiveData);
            close(my_socket);            
        }

        List_add(printList, msg);
        // unlock access to printList
        pthread_mutex_unlock(&mutex_print);
        pthread_cond_signal(&cond_outputWait);
        while(List_count(printList) == 1){
            pthread_cond_wait(&cond_receiverWait, &mutex_print);
        }
        

        // unlock access to printList
        pthread_mutex_unlock(&mutex_print);


    }

    pthread_exit(0);
   
}






int main(int argc, char** argv){
    
    // Check argument format
    if (argc != 4){
        printf("Please check your input!\n");
        printf("The correct input format:\n");
        printf("s-talk [my port number] [remote machine name] [remote port number]\n");
        return 0;
    }

    // Set up socket address for both local and remote host
    // Also check port number
    const int MY_PORT = atoi(argv[1]);
    const int REMOTE_PORT = atoi(argv[3]);
    if (MY_PORT < 0 || MY_PORT > 65535){
        printf("Please enter correct [my port number]!\n");
        return 0;
    }
    if (REMOTE_PORT < 0 || REMOTE_PORT > 65535){
        printf("Please enter correct [remote port number]!\n");
        return 0;
    }

    // local socket
    
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(MY_PORT);

    // Create the socket for UDP
    my_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (my_socket < 0){
        printf("Failed to create socket\n");
        return 0;
    }
    // Bind the socket to the port (MY_PORT)
    if (bind(my_socket, (struct sockaddr*) &my_addr, sizeof(my_addr)) < 0){
        printf("Failed to bind socket\n");
        return 0;
    }

    // remote socket

    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(REMOTE_PORT);
    peer_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
   


    // initialize Lists
    sendList = List_create();
    printList = List_create();
    
    pthread_create(&keyboard, NULL, input_keyboard, NULL);
    pthread_create(&screen, NULL, output_screen, NULL);
    pthread_create(&receiveData, NULL, receive_data, NULL);
    pthread_create(&sendData, NULL, send_data, NULL);
    pthread_join(keyboard, NULL);
    pthread_join(screen, NULL);
    pthread_join(receiveData, NULL);
    pthread_join(sendData, NULL);

    exit(1);

    
    return 0;
}
