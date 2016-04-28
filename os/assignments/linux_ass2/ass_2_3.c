#include<sched.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>

static int child_terminate_counter = 0x00;

void sigchild_handler (int singal_received) {
   
    pid_t return_status = 0x00;

    while (1) {
        int child_status = 0x00;
        
        // wait until Terminate all child process's
        return_status = waitpid (-1, &child_status, WNOHANG);
        
        if (return_status == 0x00) {
            printf ("\nChild process's still running !!!");
        }
        if (return_status < 0) {
            printf ("\nINFO:: All child process's are terminated");
            break;
        }
        else if (return_status > 0) {
            
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
            child_terminate_counter ++;
            break;
        }
    }
}

int main (int argc, char **argv) {
  
    pid_t return_status = 0x00;
    unsigned int child_counter = 0x00;
    char buffer[20] = {0};
    sigset_t s1;
    struct sigaction act1, act2;
    
    sigfillset (&s1);
//     sigdelset (&s1, SIGCHLD);
//     sigprocmask (SIG_SETMASK, &s1, &s2);
    
    while (child_counter < 5) {

        return_status = fork ();    
        if (return_status < 0) {
            printf ("\nERROR:: System Error unable to create more processes");
        }
        else if (return_status == 0x00) {
            
            printf ("\nINFO:: Child %d has PID: %d with PPID: %d", child_counter, getpid (), getppid ());
            
            if (child_counter == 2) {            
            sprintf (buffer, "abc_%02d.txt", child_counter); 
            return_status = execl ("/usr/bin/gedit", "gedit", buffer, NULL);            
            }
            exit(0);
        }
        else if (return_status > 0) {
            child_counter ++;
            continue;
        }
        
    }
    
	act1.sa_handler = sigchild_handler;
	sigfillset (&act1.sa_mask);
	sigaction (SIGCHLD, &act1, &act2);

    while (child_terminate_counter < 5) {
        return_status = sigsuspend (&s1);
    }
    
    printf ("\n\nMain process terminating");
       
    printf ("\n");
    return 0;
}