#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "greenis.h"

#define PORT 7379 /*default port*/
#define MAX_CONNECTIONS_QUEUE_SIZE 5 /*this is the maximum amount of pending connections that the pending connections queue will ever grow at*/
#define MAX_BUF_SIZE 1024 /*max buffer size. adjust this value if needed*/
#define MAX_RES_SIZE 1024 /*max response size (see parse arguments). adjust this value if needed*/

/*default server responses*/
#define OK_RESP "+OK\r\n"
#define OK_RESP_LEN 6 /*number of character of OK_RESP*/
#define ERR_RESP "-ERR\r\n" 
#define ERR_RESP_LEN 7 /*number of character of ERR_RESP*/
#define NULL_VALUE "$-1\r\n"
#define NULL_VALUE_LEN 6

/*
    REQUIREMENTS

In questo compito vogliamo creare un Redis minimale con la sola capacità di set e get di chiavi, compatibile con il protocollo Redis.
Redis accetta connessioni TCP e riceve pacchetti dati fatti da righe terminate con \r\n.
I dettagli del protocollo che ci interessano possono essere visti qui: https://redis.io/docs/reference/protocol-spec

Utilizzare il programma python e le risorse presenti qui: 
https://github.com/SENSES-Lab-Sapienza/Lab_Reti/tree/aa_2023_24/Resources/greenis

Per creare un mini Redis server (che chiameremo Greenis) che permetta di effettuare set/get di chiavi con e senza tempo di scadenza.

Il server deve permettere l'esecuzione senza errori dello script python "client.py"
*/

/*
    +, $, * sono i simboli fondamentali ($-1\r\n è il null)
    aggiungo il '-' per inviare errore?
    ci sono dei ghirigori da sistemare all'avvio della connessione, poi si comincia con i get e set


*/


/*variables declaration. These will be used by all functions and processes*/

typedef struct connection_data{ /*struct that contains every useful information for the connection*/

    int sock_fd;
    struct sockaddr_in addr;
    socklen_t addrlen;

} connection_data;

connection_data server;

void exit_with_error(const char * msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void clearbuf(char* buf, int size){/*clear buf (fill buf with zeros)*/

    memset(buf, 0, size);
}

void open_socket(){/*creates a socket*/

    /*create a tcp socket*/
    server.sock_fd = socket(AF_INET, SOCK_STREAM, 0); /*'0' means no flags are specified*/

    if(server.sock_fd == -1){/*error management*/

        exit_with_error("socket error. ");
    }

    /*fill addr with the client informations (port, ip, ecc)*/

    memset(&server.addr, 0, sizeof(server.addr)); /*fill with zero 'addr'*/
    server.addr.sin_family = AF_INET; /*socket uses AF_INET domain*/
    server.addr.sin_port = htons(PORT); /*expected connections from standard port 7379*/
    server.addr.sin_addr.s_addr = htonl(INADDR_ANY); /*expected connection request from any ip addr*/

    /*bind the socket*/
    if(bind(server.sock_fd, (struct sockaddr*) &server.addr, sizeof(server.addr)) == -1){

        exit_with_error("bind error. ");
    }

    /*listen for inbound connections*/
    if(listen(server.sock_fd, MAX_CONNECTIONS_QUEUE_SIZE) == -1){

        exit_with_error("listen error. ");
    }

    /*the socket is now ready to accept inbound connections*/
}

void close_connection(connection_data conn){/*close the socket*/
    
    if(close(conn.sock_fd) != 0){

        exit_with_error("close error. ");
    }
}

int accept_new_connection(connection_data * client){/*This function will create a new process and accept the new inbound connection. Returns the pid of the newly created process on success (see fork(2) on RETURN VALUE section), -1 on error*/

    pid_t pid = -1;/*init value. if -1 is maintained then some kind of error occured*/
    if((client->sock_fd = accept(server.sock_fd, (struct sockaddr*) &(client->addr), &(client->addrlen))) == -1){/*keeps trying to accept a new connection*/

        perror("accept error. Trying again. ");
        return -1;
    }
    
    /*now that the inbound connection has been accepted i create a new process that keeps trying to accept a new connection */
    pid = fork();
    if(pid == -1){/*in case of error the current */

        perror("error on fork. The current connection will be closed and there will be another try. ");
        close_connection(*client);
        return -1;
    }

    /* if the pid is 0 this is the child process. The memory of the child process is filled with all the informations that he needs*/
    return pid;
}

void father_handler(int sig){

    puts("\nServer is closing the connection...");
    close_connection(server);
    exit(EXIT_SUCCESS);
}

void child_handler(int sig){/*i want the server socket to be closed only once*/

    exit(EXIT_SUCCESS);
}

int main (){

    puts("\nUse SIGINT (CTRL+C) to kill the process\n");

    /*signal handler to kill the process*/
    signal(SIGINT, father_handler);

    // Open Socket and receive connections (server side)
    open_socket();


    /*create a process*/
    /*The parent process tries to accept a connection. When a connection is successfully accepted a new process whit the data of the newly accepted connection is created and the father move on trying to accept another connection*/
    
    connection_data client;
    int pid = -1;
    do{

        connection_data* client_ptr = &client;
        pid = accept_new_connection(client_ptr);
        
        if(pid == -1){

            puts("accept_new_connection error");
        }

    } while(pid != 0); /*the 'father' process cannot exit this loop*/

    if(pid == 0){/*only the child process has to manage the requests of its client*/

        /*signal handler to kill the process*/
        signal(SIGINT, child_handler);

        char buf[MAX_BUF_SIZE]; /*buffer*/
        clearbuf(buf, MAX_BUF_SIZE); /*initialize the buffer with zeros*/
        
        while(read(client.sock_fd, buf, MAX_BUF_SIZE) > 0){/*in case of error (-1) or connection closed by the peer (0) break the loop and close the client socket*/

            /*parse the request*/
            char* response;
            int tmp = parse(buf, &response);
            if(tmp == 0 || tmp == 2){/*parsed successfully*/

                puts("\nparsed successfully. proceeding.");
                if(write(client.sock_fd, OK_RESP, OK_RESP_LEN) < 0){break;}/*if write returns -1 close the connection*/
            }
            else if(tmp == -1){/*parsing error. invalid command*/

                puts("\nparse error. respond with error code.");
                if(write(client.sock_fd, ERR_RESP, ERR_RESP_LEN) < 0){break;}/*if write returns -1 close the connection*/
            }
            else if(tmp == 1){

                puts("\nnull get value.");
                if(write(client.sock_fd, NULL_VALUE, NULL_VALUE_LEN) < 0){break;}/*if write returns -1 close the connection*/
            }
            else{

                puts("\nvalid get value.");
                if(write(client.sock_fd, response, strlen(response)) < 0){break;}/*if write returns -1 close the connection*/
            }
            
            puts(buf);
            clearbuf(buf, MAX_BUF_SIZE);
        }

        puts("client closed the connection");
        close_connection(client);
        return 0;
    }

    puts("serv sock closed");
    /*the father of all processes is responsible for closing the server socket*/
    close_connection(server);
    return 0;
}