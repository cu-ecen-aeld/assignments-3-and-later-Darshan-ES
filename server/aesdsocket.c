#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/queue.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>

#define PORT_NO 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"
#define TIMESTAMP_INT 10

int server_fd;
bool Exit_Flag = false;
pthread_mutex_t FileMutex = PTHREAD_MUTEX_INITIALIZER;

// Link list Structure 
typedef struct thread_node {
    pthread_t thread;
    int client_fd;
    SLIST_ENTRY(thread_node) NextNode;
} thread_node_t;

SLIST_HEAD(thread_list, thread_node) HeadNode = SLIST_HEAD_INITIALIZER(HeadNode);

// Function to daemonize 
void daemonizeProcess() {
    pid_t pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);  

    setsid();  // Create a new session

    
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    close(fd);
}

// Timestamp in every 10 seconds 
void *WriteTimestamp(void *arg) {
    while (Exit_Flag==0) {
        sleep(TIMESTAMP_INT);

        
        time_t CurrentTime = time(NULL);
        struct tm *tm_CurrentTimeinfo = localtime(&CurrentTime);
        
        char timestamp_arr[100];
        
        strftime(timestamp_arr, sizeof(timestamp_arr), "timestamp:%Y-%m-%d %H:%M:%S\n", tm_CurrentTimeinfo);

        pthread_mutex_lock(&FileMutex);
        
        int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
        (file_fd >= 0) ? (write(file_fd, timestamp_arr, strlen(timestamp_arr)), close(file_fd)): syslog(LOG_ERR, "file Not open for timestamp writing");

        pthread_mutex_unlock(&FileMutex);
    }
    return NULL;
}

//  Function to handle client connections 
void *ClientHandle(void *arg) {

    int client_fd = *(int *)arg;
    
    free(arg); 

    char Str_buffer[1024];
    ssize_t bytes_rec;

    syslog(LOG_INFO, "Accepted connection from client: %d", client_fd);

    
    pthread_mutex_lock(&FileMutex);
    int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    pthread_mutex_unlock(&FileMutex);

    if (file_fd < 0) {
        syslog(LOG_ERR, "Error opening file: %s", strerror(errno));
        close(client_fd);
        return NULL;
    } else {
       syslog(LOG_INFO, "FIle open success");
    }

    while ((bytes_rec = recv(client_fd, Str_buffer, sizeof(Str_buffer) - 1, 0)) > 0) {
        Str_buffer[bytes_rec] = '\0';  // Null terminate

       pthread_mutex_lock(&FileMutex);
        
        ssize_t Total_WR = 0;
        while (Total_WR < bytes_rec) {
            ssize_t written = write(file_fd, Str_buffer + Total_WR, bytes_rec - Total_WR);
            if (written < 0) {
                syslog(LOG_ERR, "unable writing to file");
                break;
            }else {
	       syslog(LOG_INFO, "writing to file Success");
	    }
            Total_WR += written;
        }

        fsync(file_fd);  // Ensure data is written immediately
        pthread_mutex_unlock(&FileMutex);

        if (strchr(Str_buffer, '\n')) {  // If newline detected, read back the file
            close(file_fd);
            pthread_mutex_lock(&FileMutex);
            file_fd = open(FILE_PATH, O_RDONLY);
            pthread_mutex_unlock(&FileMutex);

            if (file_fd < 0) {
                syslog(LOG_ERR, "Error reopening file for reading");
                break;
            } else {
       syslog(LOG_INFO, "File Reponed");
    }

            memset(Str_buffer, 0, sizeof(Str_buffer));
            for (ssize_t bytes_rec = read(file_fd, Str_buffer, sizeof(Str_buffer)); bytes_rec > 0; bytes_rec = read(file_fd, Str_buffer, sizeof(Str_buffer))) {
    
               send(client_fd, Str_buffer, bytes_rec, 0);
}

            close(file_fd);
            break;
        }
    }

    close(client_fd);
    syslog(LOG_INFO, "Closed connection from client: %d", client_fd);

    return NULL;
}

// Signal handler for SIGINT and SIGTERM
void SigHandler(int sig) {
    syslog(LOG_INFO, "Signal received, exiting...");
    Exit_Flag= true;
    printf("SIGINT And SIGTERM Detected  \n");
    shutdown(server_fd, SHUT_RDWR);
    
}

int main(int argc, char *argv[]) {
printf("v7 \n");

    struct sockaddr_in server_addr, client_addr;
    
    socklen_t ClientLen = sizeof(client_addr);
    
    int *client_File_ptr;
    pthread_t timestamp_th;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "Socket Creation failed");
        exit(EXIT_FAILURE);
    }
    else {
       syslog(LOG_INFO, "Socket creation success");
    }

    //Enable SO_REUSEADDR to allow reuse of the address
    int Reuse_opt = 1;
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &Reuse_opt, sizeof(Reuse_opt)) < 0) {
        syslog(LOG_ERR, "Setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    else {
       syslog(LOG_INFO, "setsockopt passed");
    }

    // Bind socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_NO);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        syslog(LOG_ERR, "Failed to bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    else {
       syslog(LOG_INFO, "Sucess to bind");
    }

    
    // Handle daemon mode
    if (argc > 1) {
    if (strcmp(argv[1], "-d") == 0) {
        daemonizeProcess();
    } else {
        syslog(LOG_INFO, "No Daemon mode requested");
    }
}


    // Signal Handling
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    // Listen for connections
    if (listen(server_fd, 10) < 0) {
        syslog(LOG_ERR, "Failed to Listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    else {
       syslog(LOG_INFO, "Sucess to Listen");
    }
	pthread_create(&timestamp_th, NULL, WriteTimestamp, NULL);
    syslog(LOG_INFO, "Server initialized, listening on port %d", PORT_NO);
    
    while (Exit_Flag==0)
    {
    	 client_File_ptr = malloc(sizeof(int));  // Allocate memory 
        if (client_File_ptr==0) {
            syslog(LOG_ERR, "Memory Not Allocated");
            continue;
        }
        else{
        	syslog(LOG_INFO, "Memory Allocated");
        }
        
        *client_File_ptr = accept(server_fd, (struct sockaddr*)&client_addr, &ClientLen);
        
        if (*client_File_ptr < 0) {
            free(client_File_ptr);
            continue;
        }
        
        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));
        
        thread_node_t *NewNode = malloc(sizeof(thread_node_t)); //Create New Node for Handling CLient
        if(NewNode==0)
        {
           syslog(LOG_ERR, "Memory Not allocated for thread ");
           close(*client_File_ptr);
           free(client_File_ptr);
           continue;
        }
        else
        {
          syslog(LOG_INFO, "Memory Allocated for Thread");
        }
        
        NewNode->client_fd = *client_File_ptr;
        pthread_create(&NewNode->thread, NULL, ClientHandle, client_File_ptr);
        SLIST_INSERT_HEAD(&HeadNode, NewNode, NextNode);
        
    }
    
    thread_node_t *node;
    
    while (SLIST_EMPTY(&HeadNode)==0) { //wait till all threads finish
        node = SLIST_FIRST(&HeadNode);
        pthread_join(node->thread, NULL);
        SLIST_REMOVE_HEAD(&HeadNode, NextNode);
        free(node);
    }
    
      close(server_fd);
	
    // Graceful Cleanup
    pthread_cancel(timestamp_th);
    pthread_join(timestamp_th, NULL);
    pthread_mutex_destroy(&FileMutex);
    close(server_fd);
    remove(FILE_PATH);
    closelog();
    syslog(LOG_INFO, "gracefully exit server");


    return 0;
}
