#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/socket.h>
#include <signal.h>

#define PORT_NO 9000
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_fd;

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

// Signal handler for SIGINT and SIGTERM
void SigHandler(int sig) {
    syslog(LOG_INFO, "Signal received, exiting...");
    close(server_fd);
    remove(FILE_PATH);
    closelog();
    printf("SIGINT and SIGTERM Detected v1 \n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in server_addr, client_addr;
    char Str_buffer[1024];
    socklen_t ClientLen = sizeof(client_addr);

    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        syslog(LOG_ERR, "Socket creation failed");
        exit(EXIT_FAILURE);
    }
    else {
       syslog(LOG_INFO, "Socket creation success");
    }

    //Enable SO_REUSEADDR to allow reuse of the address
    int Reuse_opt = 1;
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &Reuse_opt, sizeof(Reuse_opt)) < 0) {
        syslog(LOG_ERR, "setsockopt failed");
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
        syslog(LOG_INFO, "No daemon mode requested");
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

    syslog(LOG_INFO, "Server initialized, listening on port %d", PORT_NO);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &ClientLen);
        if (client_fd < 0) continue;

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        // Open file writing
        int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (file_fd < 0) {
            syslog(LOG_ERR, "File not opened");
            close(client_fd);
            continue;
        }
        else {
       syslog(LOG_INFO, "File opened");
     }

        // Receive Data
        ssize_t bytes_rec;
        while ((bytes_rec = recv(client_fd, Str_buffer, sizeof(Str_buffer) - 1, 0)) > 0) {
        
            Str_buffer[bytes_rec] = '\0';  // Null terminate
            
            if (write(file_fd, Str_buffer, bytes_rec) != bytes_rec) {
                syslog(LOG_ERR, "Write operation failed.");
            }
             else {
	       syslog(LOG_INFO, "Write operation Sucess");
	     }
            
            fsync(file_fd);  // Flush disk

            
            if (strchr(Str_buffer, '\n')) {// if newline detected
                close(file_fd);
                file_fd = open(FILE_PATH, O_RDONLY);
                if (file_fd < 0) {
                    syslog(LOG_ERR, "reopening file Error");
                    break;
                }
                 else {
	       syslog(LOG_INFO, "reopening file Sucess");
	     }

                memset(Str_buffer, 0, sizeof(Str_buffer));
                
                for (; (bytes_rec = read(file_fd, Str_buffer, sizeof(Str_buffer))) > 0;) {
                    send(client_fd, Str_buffer, bytes_rec, 0);
                }

                close(file_fd);
                break;
            }
        }

        close(file_fd);
        
        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
        
        close(client_fd);
    }

    return 0;
}

