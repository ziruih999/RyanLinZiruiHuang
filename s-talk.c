/*
get help with Programming with UDP sockets
reference: https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html

get help with P2P socket programming
reference: https://stackoverflow.com/a/15027222

get help with multithreading programming
reference: https://www.geeksforgeeks.org/multithreading-c-2/
reference: http://zhangxiaoya.github.io/2015/05/15/multi-thread-of-c-program-language-on-linux/

get help with finding ip address of remote host
reference: http://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
*/



#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <pthread.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include "list.h"

#define MSG_MAX_LEN 1024



// ***** global variables block *****

static int my_socket;
static struct sockaddr_in my_addr, peer_addr;
static pthread_t keyboard_in, screen_out, network_in, network_out;

static List* list_input; //shared by input & send (consumer:send, producer:input)
static List* list_output; //shared by output & receive (consumer:output, producer:receive)

// mutexs 
static pthread_mutex_t mutex_send = PTHREAD_MUTEX_INITIALIZER; //for input list
static pthread_mutex_t mutex_print = PTHREAD_MUTEX_INITIALIZER; //for output list

// condition variables
static pthread_cond_t cond_inputWait = PTHREAD_COND_INITIALIZER; // wait send thread to send message
static pthread_cond_t cond_outputWait = PTHREAD_COND_INITIALIZER; // wait new message from receive thread
static pthread_cond_t cond_senderWait = PTHREAD_COND_INITIALIZER; // wait new message from input thread
static pthread_cond_t cond_receiverWait = PTHREAD_COND_INITIALIZER; // wait output thread to print the message


// for fgets read text file
// fgets will make msg = "\0" after reading the file
// so we have to stop the chat when the reading complete
static bool nullchar = false; 


// from test program provided by instructor in Assignment1
static int complexTestFreeCounter = 0;
static void complexTestFreeFn(void* pItem) 
{
    if(pItem != NULL){
        complexTestFreeCounter++;
    }
}

/*
    goal: get message from input and add that message to sendList
    1. get message from keyboard or text file
    2. add message to list_input (critical section)
    3. let send thread send the message out
    4. if message is '!' or (it is '\0' and it is from reading a txt file) :
            no more message to be received: cancel other threads, exit
*/
void *input_keyboard() {
    
    char msg[MSG_MAX_LEN];
    while (1){
        
        // get message from keyboard
        // https://stackoverflow.com/a/22065708
        printf("Enter message:\n");
        memset(msg, '\0', MSG_MAX_LEN);
        if(fgets(msg, MSG_MAX_LEN, stdin) == NULL){
            nullchar = true;
        }
        // don't want '\n' at end of msg 
        msg[strlen(msg)-1] = '\0';
        // Start critical section
        pthread_mutex_lock(&mutex_send);
        if(List_add(list_input,msg) < 0){
            printf("failed to add input message into list\n");
            exit(1);
        }
        // End critical section

        // OK, now it is turn for send thread, unlock it
        // send thread is either blocked on mutex or condition variable
        pthread_cond_signal(&cond_senderWait);
        pthread_cond_wait(&cond_inputWait, &mutex_send);
        // if input message is '!', I want to terminate the conversation
        // ready to return to main thread
        if ((msg[0] == '!' && strlen(msg) == 1) || nullchar == true){
            printf("...... terminating request by ME ......\n");            
            sleep(1);
            List_free(list_input, complexTestFreeFn);
            List_free(list_output, complexTestFreeFn);  
            // cancel threads
            pthread_cancel(network_out);
            pthread_cancel(network_in);
            pthread_cancel(screen_out);
            
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
   

            return NULL;
            
        }
        else{
             // have to unlock mutex otherwise input thread cannot continue
             pthread_mutex_unlock(&mutex_send);
        }
    }
}

/*
    goal: get a message from list_input, then send it to remote user
    1. if list_input is empty, wait until input thread wake it up
    2. get a message from list_input (critical section)
    3. send message to remote user
    4. wake up input thread
*/
void *send_data(void *remaddr) {
    char msg[MSG_MAX_LEN];
    unsigned int sin_len = sizeof(peer_addr);
    while(1){
        // Start critical section
        pthread_mutex_lock(&mutex_send);
        // if nothing in list_input, wait
        if(List_count(list_input) == 0){
            // testcancel would be needed when output thread want to cancel this thread
            pthread_testcancel();
            pthread_cond_wait(&cond_senderWait, &mutex_send);
        }
        strcpy(msg, List_first(list_input));
        List_remove(list_input);
        // leave Critical Section, unlock access to list_input
        // input thread is waiting, signal it

        pthread_mutex_unlock(&mutex_send);
        pthread_cond_signal(&cond_inputWait);
        // send message to remote use
        if (sendto(my_socket, msg, strlen(msg), 0, (struct sockaddr *) &peer_addr, sin_len) < 0){
            perror ("sento failed");
            exit(1);
        }   
    }

    return NULL;
}


/*
    goal: get a message from list_output and print it to screen
    1. if list_output is empty, wait until receive thread wake it up
    2. get the message from list_output (critical section)
    3. print message to screen
    4. wake up receive thread
*/

void *output_screen() {
    char msg[MSG_MAX_LEN];
    while(1){
        // start critical section
        pthread_mutex_lock(&mutex_print);
        // if no message in list_output
        // wait until receive thread 
        if(List_count(list_output) == 0){
            // testcancel needed when input thread want to cancel this thread
            pthread_testcancel();
            pthread_cond_wait(&cond_outputWait, &mutex_print);
        }
        
        // copy and remove the first message from list_output
        strcpy(msg, List_first(list_output));
        List_remove(list_output);
        // leave Critical Section

        // if receive thread is waiting, signal it
        pthread_mutex_unlock(&mutex_print);
        pthread_cond_signal(&cond_receiverWait);
        printf("Remote User:");
        puts(msg);
    }

    return NULL;
}





/*
    goal: receive message from socket, and add it to list_output
    1. receive message
    2. add message to list_output (critical section)
    3. if message == '!' : close the socket, cancel threads, exit
    4. switch turn to output thread
*/

void *receive_data(void *remaddr) {
    char msg[MSG_MAX_LEN];
    int addrlen = sizeof(peer_addr);
    while (1){
        // memset(msg, '\0', MSG_MAX_LEN);
        // get message (reference: Dr.Brian's workshop code)
        int bytesRx = recvfrom(my_socket, msg, MSG_MAX_LEN, 0, (struct sockaddr *) &peer_addr, &addrlen);
        if (bytesRx < 0){
            printf("recvfrom() failed\n");
            exit(1);
        }
        // Make it null terminated (so string functions work):
        int terminateIdx = (bytesRx < MSG_MAX_LEN) ? bytesRx : MSG_MAX_LEN - 1;
		msg[terminateIdx] = 0;
        
        // enter critical section
        pthread_mutex_lock(&mutex_print);
        if(List_add(list_output, msg) < 0){
            printf("failed to add received message into list");
            exit(1);
        }
        // wake up output thread and wait until it print the message
        pthread_cond_signal(&cond_outputWait);
        pthread_cond_wait(&cond_receiverWait, &mutex_print);

        // if it's a termination message
        if ((msg[0] == '!' && msg[1]== '\0')){
            printf("......termination request by REMOTE-USER......\n");
            sleep(1);
            List_free(list_input, complexTestFreeFn);
            List_free(list_output, complexTestFreeFn);
            // cancel threads and socket
            pthread_cancel(network_out);
            pthread_cancel(screen_out);
            pthread_cancel(keyboard_in);

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
   
    
            return NULL;
        }

        else{
            // unlock access to list_output
            pthread_mutex_unlock(&mutex_print);
        }
    }
}



void network_init(char** argv){
    // reference:https://stackoverflow.com/a/15027222
    // Set up socket address for both local and remote host
    // Also check port number
    const int MY_PORT = atoi(argv[1]);
    const int REMOTE_PORT = atoi(argv[3]);
    if (MY_PORT < 0 || MY_PORT > 65535){
        printf("Please enter correct [my port number]!\n");
        exit(1);
    }
    if (REMOTE_PORT < 0 || REMOTE_PORT > 65535){
        printf("Please enter correct [remote port number]!\n");
        exit(1);
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
        exit(1);
    }
    // Bind the socket to the port (MY_PORT)
    if (bind(my_socket, (struct sockaddr*) &my_addr, sizeof(my_addr)) < 0){
        printf("Failed to bind socket\n");
        exit(1);
    }

    // set up ip address of remote host
    // reference: Beej's Guide to Network Programming
    struct addrinfo hint, *result, *temp;
    int rval;
    char remote_ip[45];
    memset(&hint, 0, sizeof(peer_addr));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_DGRAM;

    if ((rval = getaddrinfo(argv[2], argv[3], &hint, &result))!=0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rval));
        exit(1);
    }
    for (temp = result; temp != NULL; temp = temp->ai_next){
        void *addr;
        if (temp->ai_family == AF_INET){
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)temp->ai_addr;
            addr = &(ipv4->sin_addr);
            inet_ntop (temp->ai_family, addr, remote_ip, sizeof(remote_ip));
        }
    }
    freeaddrinfo(result);
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(REMOTE_PORT);
    peer_addr.sin_addr.s_addr = inet_addr(remote_ip);

    // Network name resolution 
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("     IP ADDRESS OF REMOTE USER : %s \n", remote_ip);
    printf("         CHAT SESSION GOING TO START \n");
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * \n");

}



int main(int argc, char** argv){
    
    // Check argument format
    if (argc != 4){
        printf("Incorrect form entered, please reenter <local port> <remote hostname> <remote port>\n");
        return 0;
    }

    // initialize network stuff
    network_init(argv);
    // initialize Lists
    list_input = List_create();
    list_output = List_create();
    
    if(pthread_create(&keyboard_in, NULL, input_keyboard, NULL) != 0){
        perror("pthread_create() error");
        exit(1);
    }
    if(pthread_create(&screen_out, NULL, output_screen, NULL) != 0){
        perror("pthread_create() error");
        exit(1);
    }
    if(pthread_create(&network_in, NULL, receive_data, NULL) != 0){
        perror("pthread_create() error");
        exit(1);
    }
    if(pthread_create(&network_out, NULL, send_data, NULL) != 0){
        perror("pthread_create() error");
        exit(1);
    }

    if(pthread_join(keyboard_in, NULL) !=0){
        perror("pthread_join() error");
        exit(1);
    }
    if(pthread_join(network_out, NULL) !=0){
        perror("pthread_join() error");
        exit(1);
    }
    if(pthread_join(network_in, NULL) !=0){
        perror("pthread_join() error");
        exit(1);
    }
    if(pthread_join(screen_out, NULL) !=0){
        perror("pthread_join() error");
        exit(1);
    }


    return 0;
}
