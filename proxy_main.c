/**
 * Main proxy function listen to ports and create child processes
 */
#define _XOPEN_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>

#include "config.h"
#include "proxy.h"


int get_listen_port()
{
    char buf[32];
    config_open(CONFIG_FILENAME);
    config_get_value("LISTEN_PORT", buf);
    config_close();

    return atoi(buf);
}


int fetch(char *post, char *response)
{
    char *reply =
        "HTTP/1.1 200 OK\n"
        "Date: Thu, 19 Feb 2009 12:27:04 GMT\n"
        "Server: Apache/2.2.3\n"
        "Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\n"
        "ETag: \"56d-9989200-1132c580\"\n"
        "Content-Type: text/html\n"
        "Content-Length: 39\n"
        "Accept-Ranges: bytes\n"
        "Connection: close\n"
        "\n"
        "I guess you want to visit Google right?";

    // look for ipaddr, post request and get response back
    strcpy(response, reply);
}

int write_back(int client_socket_fd, char *response)
{
    printf("Writing[%s]\n", response);
    send(client_socket_fd, response, strlen(response), 0);
}


int process_post(int client_socket_fd)
{
    int sz;
    char post[MSG_SZ];
    char response[MSG_SZ];
    int pid;


    printf("Connected[%d]\n", client_socket_fd);

    while ((sz = recv(client_socket_fd, post, MSG_SZ, 0)) > 0)
    {
        printf("---- start ----\n%s\n---- end ----\n", post);

        // fork a subprocess to fetch data from remote server
        pid = fork();
        if (pid < 0)
        {
            printf("Failed to fork subprocess\n");
            return -4;
        }
        else if (pid == 0)
        {
            // if successfully fetched
            fetch(post, response);

            write_back(client_socket_fd, response);

            close(client_socket_fd);
            exit(0);
        }
        else
        {

        }

    }

    if (sz == 0)
    {
        printf("Disconnected[%d]\n", client_socket_fd);
    }
    else if (sz == -1)
    {
        printf("Failed to receive\n");
    }


    return sz;
}

int listen_port()
{
    struct sockaddr_in server;
    struct sockaddr_in client;

    int client_socket_fd;
    int socket_fd;

    int c;
    int pid;

    // open socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        printf("Failed to create socket\n");
        return -1;
    }

    // set connection
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(get_listen_port());

    // bind local host
    if (bind(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        printf("Failed to bind\n");
        return -3;
    }
    printf("Successfully bind to localhost\n");

    // listen
    listen(socket_fd, 5);

    // Accept incoming connection
    printf("Waiting for incoming connections\n");

    c = sizeof(struct sockaddr_in);

    while (1)
    {
        client_socket_fd = accept(socket_fd, (struct sockaddr *)&client, (socklen_t *)&c);
        if (client_socket_fd < 0)
        {
            printf("Failed to accept, skip\n");
            continue;
        }

        // fork a subprocess to do with post
        pid = fork();
        if (pid < 0)
        {
            printf("Failed to fork subprocess\n");
            return -3;
        }
        else if (pid == 0)
        {
            // child process close server socket
            close(socket_fd);
            process_post(client_socket_fd);
            exit(0);
        }
        else
        {
            // parent proces close incoming connection
            close(client_socket_fd);
        }
    }

    return 0;
}


int main(int argc, char **argv)
{
    // test the number of arguments
    if (argc != 2)
    {
        printf("Invalid number of arguments\n");
        return -1;
    }

    // prevent from creating any unwanted zombie processes
    struct sigaction sigchld_action = {
        .sa_handler = SIG_DFL,
        .sa_flags = SA_NOCLDWAIT
    };
    sigaction(SIGCHLD, &sigchld_action, NULL);


    // start: create the child process that listen to port and allocate
    // fetching tasks to proxy_fetch
    if (!strcmp(argv[1], "start"))
    {
        int pid;

        // create the subprocess and exit the function
        pid = fork();
        if (pid < 0)
        {
            printf("Failed to fork subprocess\n");
            return -2;
        }
        else if (pid == 0)
        {
            // execute daemon listen port
            listen_port();
            exit(0);
        }
        else
        {
            // should handle zombie process
            exit(0);
        }

    }

    // kill daemon processes
    else if (!strcmp(argv[1], "stop"))
    {
        // kill the listen process
        system("killall cproxy.out");
    }

    return 0;

}