#include<sched.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>


int main (int argc, char **argv) {
   
    char continue_flag = 0x00;
    pid_t return_status = 0x00;
    
    continue_flag = 0x01;

    return_status = fork ();
    
    if (return_status < 0) {
        printf ("\nERROR:: System Error unable to create more processes");
        continue_flag = 0x00;
    }
    else if (return_status == 0x00) {
        printf ("\nBefore sched_yield !!!");
        
        return_status = sched_yield ();
        if (return_status == 0x00) {
            printf ("\nAPI:: sched_yield success");
        }
        else {
            printf ("\nAPI:: sched_yield failed");
        }
        
        printf ("\nINFO:: Child has PID: %d with PPID: %d", getpid (), getppid ());
        
        exit(0);
    }
    else if (return_status > 0) {

        while (continue_flag) {
            int child_status = 0x00;
            
            printf ("\nStart Waiting for child to terminate");
            
            // wait until Terminate all child process's
            return_status = waitpid (-1, &child_status, 0);
            
            if (return_status < 0) {
                printf ("\n\nINFO:: All child process's are terminated");
                continue_flag = 0x00;
            }
            else if (return_status > 0) {
                
                printf ("\n\nINFO:: %d child status changed", return_status);
                
                if (WIFEXITED (child_status)) {
                    
                    // Child terminated normal
                    if (WEXITSTATUS (child_status) == 0x00) {
                        // that is exit(0)
                        printf ("\nINFO:: %d child terminated normal & success", return_status);
                    }
                    else {
                        // that is if exit(2) or exit(3)...
                        printf ("\nINFO:: %d child terminated Normal but not success", return_status);
                    }                    
                }
                else {
                    // Abnormal termination of child
                    printf ("\nINFO:: %d child terminated abnormal", return_status);
                }
            }
        }
    }    
       
    printf ("\n");
    return 0;
}