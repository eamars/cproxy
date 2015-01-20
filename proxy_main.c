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
#include <curl/curl.h>

#include "config.h"
#include "proxy.h"

static const char *usage = "Usage: cproxy.out [start | stop]";

struct MemoryStruct {
    char *memory;
    size_t size;
};


int get_listen_port()
{
    char buf[32];
    config_open(CONFIG_FILENAME);
    config_get_value("LISTEN_PORT", buf);
    config_close();

    return atoi(buf);
}



int extract_host(char *post, char *host)
{
    // Search for host in http header
    static const char *HOST = "Host:";
    while (*post != '\0')
    {
        if (!strncmp(post, HOST, strlen(HOST)))
        {
            break;
        }
        post++;
    }

    // move pointer to the address
    post += strlen(HOST) + 1;

    // copy the host char by char
    while (*post != '\n' && *post != '\r' && *post != '\0')
    {
        *host = *post;
        host++;
        post++;
    }

    // terminate it
    *host = '\0';

    return 0;
}

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
    strncpy(buffer, userdata, size *nitems);
    return size *nitems;
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int handle_request(char *post, char *host, int client_socket_fd)
{
    CURL *curl = NULL;
    CURLcode res;
    struct MemoryStruct chunk;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    // pass url
    curl_easy_setopt(curl, CURLOPT_URL, host);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, post);

    curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);


    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {
        printf("ERROR\n");
    }
    else
    {
        printf("%lu bytes retrieved\n", (long)chunk.size);

        // write to client
        send(client_socket_fd, chunk.memory, chunk.size, 0);
    }





    curl_easy_cleanup(curl);
    if(chunk.memory)
    {
        free(chunk.memory);
    }
    curl_global_cleanup();

    return 0;
}

int process_post(int client_socket_fd)
{
    int sz;
    char post[MSG_SZ];
    char host[MSG_SZ];
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
            // get host from incoming http header
            extract_host(post, host);

            handle_request(post, host, client_socket_fd);

            close(client_socket_fd);
            exit(0);
        }
        else
        {
            continue;
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

            continue;
        }
    }

    return 0;
}


int main(int argc, char **argv)
{
    // test the number of arguments
    if (argc != 2)
    {
        printf("Invalid number of arguments\n%s\n", usage);
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

    else
    {
        printf("Unidentified parameter\n%s\n", usage);
    }

    return 0;

}