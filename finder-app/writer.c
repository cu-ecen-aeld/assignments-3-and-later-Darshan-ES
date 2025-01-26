#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


int main(int argc, char *argv[]) {
    // Open syslog utility for Error Logging
    openlog("writer", LOG_PID | LOG_CONS, LOG_USER);
    // Check if both the arguments are available
    if (argc !=3) {
    //Log Error if two arquments are not present
        syslog(LOG_ERR, "Error Message: Two required arguments - <file_path> <write_string>");
        fprintf(stderr, "Usage: %s <file_path> <write_string>\n", argv[0]);
        //CLose the syslog
        closelog();
        
        return EXIT_FAILURE;
    }

    const char *file_dir = argv[1]; //Assign File directory
    const char *str_wr = argv[2]; //assign String

    // File opening for writing 
    int fd = open(file_dir, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    
    if (fd <0) {
    //Log Error if not able to open file
        syslog(LOG_ERR, "Error Message: File Unable to open '%s'", file_dir);
        perror("open");
        fprintf(stderr, "Error Message: Error opening file '%s' : %s\n",file_dir,strerror(errno));
        closelog();
        
        return EXIT_FAILURE;
    }

    // Writing string to  file
    ssize_t WriteBytes = write(fd, str_wr, strlen(str_wr));
    if (WriteBytes <0) {
    //Log Error if not able to write to file
        syslog(LOG_ERR, "Error: Unable to write to file '%s'", file_dir);
        fprintf(stderr, "Error Message: Error writing file '%s' : %s\n",file_dir,strerror(errno));
        close(fd);
        closelog();
        
        return EXIT_FAILURE;
    }
    // Log Debug write
    syslog(LOG_DEBUG, "Writing '%s' to '%s'", str_wr, file_dir);
    // Close the file
    if (close(fd) <0) {
    //Log Error if not able to close the file
        syslog(LOG_ERR, "Error: Unable to close file '%s'", file_dir);
        fprintf(stderr, "Error Message: Error Closing file '%s' : %s\n",file_dir,strerror(errno));
        closelog();
        
        return EXIT_FAILURE;
    }
    // Close syslog 
    closelog();
    
    return EXIT_SUCCESS;
}

