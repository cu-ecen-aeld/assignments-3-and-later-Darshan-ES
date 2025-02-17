#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
   struct thread_data* thread_func_args = (struct thread_data *) thread_param;
   
   if(thread_func_args == NULL)
   {
   	ERROR_LOG("Function Arguments are NULL\n");
   	return NULL;	
   }
   
   DEBUG_LOG("Function Arguments for Threads Created\n");
   /*This 'usleep' was based on content at [ https://www.javatpoint.com/usleep-function-in-c ] with modifications #[ 'usleep' Suspends the excecution for microseconds ].*/
   usleep(thread_func_args->wait_to_obtain_ms*1000); //
   
   if(pthread_mutex_lock(thread_func_args->mutex) == 0)
   {
   	DEBUG_LOG("Mutex Lock is Established\n");
   }
   else
   {
   	ERROR_LOG("Mutex Lock is not Established\n");
   	thread_func_args->thread_complete_success=false;
   	return thread_param; 
   }
   /*This 'usleep' was based on content at [ https://www.javatpoint.com/usleep-function-in-c ] with modifications #[ 'usleep' Suspends the excecution for microseconds ].*/
   usleep(thread_func_args->wait_to_release_ms*1000); //
   
   if(pthread_mutex_unlock(thread_func_args->mutex)==0)
   {
   	DEBUG_LOG("Mutex unLock is Established\n");
   }
   else
   {
   	ERROR_LOG("Mutex unLock is not Established\n");
   	thread_func_args->thread_complete_success=false;
   	return thread_param; 
   }
   
   thread_func_args->thread_complete_success=true;
   
   return thread_param; 
   
   
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
   // return false;
    
     if (thread==NULL || mutex==NULL) {
        ERROR_LOG("Invalid arguments.\n");
        return false;
    }

    
    struct thread_data* thread_args = (struct thread_data*) malloc(sizeof(struct thread_data));
    if (thread_args==NULL) {
        ERROR_LOG("Memory allocation failed.\n");
        return false;
    }

    
    thread_args->mutex = mutex;
    thread_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_args->wait_to_release_ms = wait_to_release_ms;
    thread_args->thread_complete_success = false;

    
    if (pthread_create(thread, NULL, threadfunc, (void*) thread_args) == 0) {
       	DEBUG_LOG("Thread created successfully. \n"); 
    }
    else
    {
    	ERROR_LOG("Thread creation failed.");
        free(thread_args);
        return false;
    }

    
    return true;
}

