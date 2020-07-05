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

static int my_socket;
static struct sockaddr_in my_addr, peer_addr;
static pthread_t keyboard, screen, receiveData, sendData;

static List* sendList; //shared by input & send (consumer:send, producer:input)
static List* printList; //shared by output & receive (consumer:output, producer:receive)

// mutexs 
static pthread_mutex_t mutex_send = PTHREAD_MUTEX_INITIALIZER; //for sendList 
static pthread_mutex_t mutex_print = PTHREAD_MUTEX_INITIALIZER; //for printList 

// condition variables
static pthread_cond_t cond_inputWait = PTHREAD_COND_INITIALIZER; // wait send thread to send message
static pthread_cond_t cond_outputWait = PTHREAD_COND_INITIALIZER; // wait new message from receive thread
static pthread_cond_t cond_senderWait = PTHREAD_COND_INITIALIZER; // wait new message from input thread
static pthread_cond_t cond_receiverWait = PTHREAD_COND_INITIALIZER; // wait output thread to print the message

// is chat still open?
static bool onChat = true; 

/*
    goal: get message from input and add that message to sendList
    1. get message from keyboard
    2. lock mutex_send
    3. add message to sendList
    4. unlock mutex_send
    4. let send thread send the message out
    5. if message is '!':
            no more message to be received: cancel other threads, exit
*/
void *input_keyboard() {

    char msg[MSG_MAX_LEN];
    while (onChat){

        // get message from keyboard
        // https://stackoverflow.com/a/22065708
        memset(msg, '\0', MSG_MAX_LEN);
        printf("Enter message\n");
        fgets(msg, MSG_MAX_LEN, stdin);
        msg[strlen(msg)-1] = '\0';

        // Start critical section
        pthread_mutex_lock(&mutex_send);
        List_add(sendList,msg); 
        // End critical section

        // OK, now it is turn for send thread, unlock it
        // send thread is either blocked on mutex or condition variable
        pthread_cond_signal(&cond_senderWait);
        pthread_cond_wait(&cond_inputWait, &mutex_send);
        // if input message is '!', I want to terminate the conversation
        // ready to return to main thread
        if (msg[0] == '!' && msg[1] == '\0'){
            printf("...... terminating request by ME ......\n");            
            // turnoff onChat
            onChat = false;
            // cancel threads
            pthread_cancel(sendData);
            pthread_cancel(receiveData);
            pthread_cancel(screen);
            
            close(my_socket);
            // destroy condition variables and mutex
            pthread_mutex_unlock(&mutex_print);
            pthread_mutex_unlock(&mutex_send);
            pthread_mutex_destroy(&mutex_send);
            pthread_mutex_destroy(&mutex_print);
            pthread_cond_destroy(&cond_outputWait);
            pthread_cond_destroy(&cond_inputWait);
            pthread_cond_destroy(&cond_senderWait);
            pthread_cond_destroy(&cond_receiverWait);     
            pthread_exit(0);
            
        }
        else{
             // have to unlock mutex otherwise input thread cannot continue
             pthread_mutex_unlock(&mutex_send);
        }
    }
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
    while(onChat){

        // Start critical section
        pthread_mutex_lock(&mutex_send);
        // if nothing in sendList, wait
        if(List_count(sendList) == 0){
            pthread_testcancel();
            pthread_cond_wait(&cond_senderWait, &mutex_send);
        }
        strcpy(msg, List_first(sendList));
        List_remove(sendList);

        // leave Critical Section, unlock access to sendList
        // input thread is waiting, signal it
        pthread_mutex_unlock(&mutex_send);
        pthread_cond_signal(&cond_inputWait);
        // send message to remote use
        if (sendto(my_socket, msg, strlen(msg), 0, (struct sockaddr *) &peer_addr, sin_len) < 0){
            perror ("sento failed");
            exit(1);
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
    pthread_testcancel();
    while(onChat){
        // block access to printList
        pthread_mutex_lock(&mutex_print);
        // if no message in printList
        // wait until receive thread 
        
        if(List_count(printList) == 0){
            pthread_testcancel();
            pthread_cond_wait(&cond_outputWait, &mutex_print);
        }
        // copy and remove the first message from printList
        strcpy(msg, List_first(printList));
        List_remove(printList);

        // leave Critical Section, unlock access to printList
        // if receive thread is waiting, signal it
        pthread_mutex_unlock(&mutex_print);
        pthread_cond_signal(&cond_receiverWait);
        printf("Remote User: %s\n", msg);
    }
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

        // if it's a termination message
        if (msg[0] == '!' && msg[1]== '\0'){
            printf("......termination request by REMOTE-USER......\n");
            onChat = false;
            // cancel threads and socket
            pthread_cancel(sendData);
            pthread_cancel(screen);
            pthread_cancel(keyboard);
            close(my_socket);
            // destroy condition variables and mutex
            pthread_mutex_unlock(&mutex_print);
            pthread_mutex_unlock(&mutex_send);
            pthread_mutex_destroy(&mutex_send);
            pthread_mutex_destroy(&mutex_print);
            pthread_cond_destroy(&cond_inputWait);
            pthread_cond_destroy(&cond_outputWait);
            pthread_cond_destroy(&cond_senderWait);
            pthread_cond_destroy(&cond_receiverWait);
    
            pthread_exit(0);
        }
        // critical section
        List_add(printList, msg);

        // unlock access to printList
        // pthread_mutex_unlock(&mutex_print);
        pthread_cond_signal(&cond_outputWait);
        pthread_cond_wait(&cond_receiverWait, &mutex_print);
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
    pthread_join(sendData, NULL);    
    pthread_join(receiveData, NULL);
    pthread_join(screen, NULL);
    return 0;
}
