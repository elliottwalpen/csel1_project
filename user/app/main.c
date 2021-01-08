#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <getopt.h>
#include <ctype.h>
#include <mqueue.h>

#include "../common.h"

#define SIG_NUM            127
#define MAX_COMMAND_LENGTH 100
#define prompt             ">$ "

static struct data_from_deamon data_from_deamon;


static void signal_handler (int signal)
{
    printf("Signal %d catched. Program exit..\n", signal);
    exit (EXIT_SUCCESS);
}

static void display_data(void)
{
    printf("Fan state\n"
           "   temp: %f\n"
           "   freq: %d\n"
           "   mode: %s\n", data_from_deamon.temp, data_from_deamon.freq, modes[data_from_deamon.mode]);
    printf ("%s", prompt);
    fflush( stdout );
}

static void listen_to_deamon_process(void) 
{
    /* open queue */
    mqd_t msgq = mq_open(MQ_DATA_FROM_DEAMON, O_RDWR, MQ_MODE, &from_deamon_attr);
    if (msgq == -1) {
        printf("Failed to open mq state. Program exit !\n");
        exit(EXIT_FAILURE);
    }

    unsigned prio = 0;
    while (1) {
        ssize_t len = mq_receive (msgq, (char*)&data_from_deamon, sizeof(struct data_from_deamon), &prio);
        if (len == -1) {
            printf("Failed to read mq state. Program exit !\n");
            exit(EXIT_FAILURE);
        }
        else
            display_data();
    }

     /* clean and exit */
    mq_close(msgq);   
}

static void shell_helper(void) 
{
    printf ("Commands:\n"
            "   automatic    set mode to automatic\n"
            "   manual       set mode to manual\n"
            "   <value>      set new frequency [2, 5, 10, 20]\n"
            "   help         print shell help\n"
            "   exit         exit program\n");
}

static int is_number(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }
    return 1;
}

static void shell_process(int pid) 
{
    struct msg_to_deamon msg_to_deamon;
    char cmd[MAX_COMMAND_LENGTH + 1];
    int valid;

    /* open queue */
    mqd_t msgq = mq_open(MQ_DATA_TO_DEAMON, O_RDWR, MQ_MODE, &to_deamon_attr);
    if (msgq == -1) {
        printf("Failed to open mq data. Program exit !\n");
        exit(EXIT_FAILURE);
    }

    /* print prompt */
    printf ("%s", prompt);
    fflush( stdout );

    while (1) {
        /* init msgq */
        msg_to_deamon.type   = -1;
        msg_to_deamon.val    = -1;
        valid      = 0;

        /* read command from stdin */
        if ( fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        
        /* remove trailing newline if any */
        if(cmd[strlen(cmd)-1] == '\n') {
            cmd[strlen(cmd)-1] = '\0';
        }
        
        if (strcmp(cmd, "manual") == 0) {
            valid = 1;
            msg_to_deamon.type = MQ_TYPE_MODE;
            msg_to_deamon.val = MODE_MANUAL;
        } else if (strcmp(cmd, "automatic") == 0) {
            valid = 1;
            msg_to_deamon.type = MQ_TYPE_MODE;
            msg_to_deamon.val  = MODE_AUTO;
        } else if (strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            shell_helper();
        } else {
            /* check input is digit */
            if (is_number(cmd)) {
                valid = 1;
                msg_to_deamon.type = MQ_TYPE_FREQ;
                int freq  = atoi(cmd);
                /* correct input */
                if (freq == 2 || freq == 5 || freq == 10 || freq == 20) {
                    msg_to_deamon.val = freq;
                } else {
                    printf("Invalid input frequency !\n");
                }                
            } else {
                printf(0, "Unknown command\n");
            }
        }

        if(valid) {
            mq_send (msgq, (char*)&msg_to_deamon, sizeof(struct msg_to_deamon), 1);
            printf("-->message send\n");
        }
    }

    /* clean and exit */
    msg_to_deamon.type = MQ_TYPE_EXIT;
    mq_send (msgq, (char*)&msg_to_deamon, sizeof(struct msg_to_deamon), 1);
    /* kill state_process */
    kill(pid, SIGTERM);
    mq_close(msgq);
    printf("Program exit...\n");
}

int main (int argc, char** argv)
{
    struct sigaction act;
    /* method to be called when signal occured */
    act.sa_handler = signal_handler;
    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    /* signals which start handler action */
    for(int i = 0; i < SIG_NUM; i++)
        sigaction (i,  &act, 0);

    /* Deamon can be start here */

    /* forking process */
    pid_t pid = fork();

    if (pid == 0) { /* child */
        /* default values */
        listen_to_deamon_process();
    } else { /* parent */
        shell_process(pid);      
    }

    return 0;
}

