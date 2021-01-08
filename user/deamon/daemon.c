#define _XOPEN_SOURCE 600
#define _DEFAULT_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/timerfd.h>

#include "fan.h"
#include "gpio.h"
#include "lcd.h"

#define K1_OBJ_IDX 0
#define K2_OBJ_IDX 1
#define K3_OBJ_IDX 2
#define TMR_OBJ_IDX 3
#define TIMER_PERIOD 200

#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

extern int errno;

enum action {INCR, MODE, DECR, TIC};
enum state {FALSE, TRUE};

typedef void (*obj_t)(struct object*);

static struct object {
    int fd;
    enum action act;
    obj_t obj;
    struct epoll_event event;
};

struct itimerspec tm_specs;
enum state LCD_UPDATE = FALSE;

static void button_handler(struct object *obj);
static void ipc_handler(struct object *obj);
static void timer_handler(struct object *obj);

struct object obj[4] = {
    [K1_OBJ_IDX] = {.obj=button_handler, .act=INCR, .event = {.events=EPOLLERR, .data.ptr=&obj[K1_OBJ_IDX],}, },
    [K2_OBJ_IDX] = {.obj=button_handler, .act=MODE, .event = {.events=EPOLLERR, .data.ptr=&obj[K2_OBJ_IDX],}, },
    [K3_OBJ_IDX] = {.obj=button_handler, .act=DECR, .event = {.events=EPOLLERR, .data.ptr=&obj[K3_OBJ_IDX],}, },
    [TMR_OBJ_IDX] = {.obj=timer_handler, .act=TIC, .event = {.events=EPOLLIN, .data.ptr=&obj[TMR_OBJ_IDX],}, },
};

static void button_handler(struct object *obj)
{
	int mode = driver_get_mode();
	int freq = driver_get_freq();

    switch(obj->act)
    {
    case INCR:
		if(mode == MANUAL)
		{
			if(freq == TWO)
				driver_set_freq(FIVE);
			else if(freq == FIVE)
				driver_set_freq(TEN);
			else if(freq == TEN)
				driver_set_freq(TWENTY);
			LCD_UPDATE = TRUE;
		}
        break;
    case MODE:
		if(mode == AUTO)
        	driver_set_mode(MANUAL);
		else if(mode == MANUAL)
			driver_set_mode(AUTO);
		LCD_UPDATE = TRUE;
        break;
    case DECR:
		if(mode == MANUAL)
		{
			if(freq == TWENTY)
				driver_set_freq(TEN);
			else if(freq == TEN)
				driver_set_freq(FIVE);
			else if(freq == FIVE)
				driver_set_freq(TWO);
			LCD_UPDATE = TRUE;
		}
        break;
    default:
        break;
    }
    char buf[10];
    pread(obj->fd, buf, ARRAY_SIZE(buf), 0);

	/* Activate led and timer go for short time*/
	pwrite(led_fd, LED_ON, sizeof(LED_ON), 0);
	timerfd_settime(obj[TMR_OBJ_IDX].fd, 0, &tm_specs, NULL);
}

static void timer_handler(struct object *obj)
{
    switch(obj->act)
    {
    case TIC:
        pwrite(led_fd, LED_OFF, sizeof(LED_OFF), 0);
        break;
    default:
        break;
    }
    char buf[10];
    pread(obj->fd, buf, ARRAY_SIZE(buf), 0);
}

static int signal_catched = 0;

static void catch_signal (int signal)
{
	if(signal == SIGQUIT)
	{
		lcd_end();
		free_gpio(K1_NB);
		free_gpio(K2_NB);
		free_gpio(K3_NB);
		free_gpio(LED);
		if(deinit_fan() == EXIT_SUCCESS)
			exit(EXIT_SUCCESS);
		else
			exit(EXIT_FAILURE);
	}
	syslog (LOG_INFO, "signal=%d catched\n", signal);
	signal_catched++;
}

static void fork_process()
{
	pid_t pid = fork();
	switch (pid) {
	case  0: break; // child process has been created
	case -1: syslog (LOG_ERR, "ERROR while forking"); exit (1); break;
	default: exit(0);  // exit parent process with success
	}
}

int main(int argc, char* argv[])
{
	UNUSED(argc); UNUSED(argv);

	pid_t pid;

	/* init the driver here */
    if(init_fan() == EXIT_FAILURE)
		exit(EXIT_FAILURE);

	// 1. fork off the parent process
	fork_process();

	// 2. create new session
	if (setsid() == -1) {
		syslog (LOG_ERR, "ERROR while creating new session");
		exit (1);
	}

	// 3. fork again to get rid of session leading process
	fork_process();

	// 4. capture all required signals
	struct sigaction act = {.sa_handler = catch_signal,};
	sigaction (SIGHUP,  &act, NULL);  //  1 - hangup
	sigaction (SIGINT,  &act, NULL);  //  2 - terminal interrupt
	sigaction (SIGQUIT, &act, NULL);  //  3 - terminal quit
	sigaction (SIGABRT, &act, NULL);  //  6 - abort
	sigaction (SIGTERM, &act, NULL);  // 15 - termination
	sigaction (SIGTSTP, &act, NULL);  // 19 - terminal stop signal

	// 5. update file mode creation mask
	umask(0027);

	// 6. change working directory to appropriate place
	if (chdir ("/") == -1) {
		syslog (LOG_ERR, "ERROR while changing to working directory");
		exit (1);
	}

	// 7. close all open file descriptors
	for (int fd = sysconf(_SC_OPEN_MAX); fd >= 0; fd--) {
		close (fd);
	}

	// 8. redirect stdin, stdout and stderr to /dev/null
	if (open ("/dev/null", O_RDWR) != STDIN_FILENO) {
		syslog (LOG_ERR, "ERROR while opening '/dev/null' for stdin");
		exit (1);
	}
	if (dup2 (STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO) {
		syslog (LOG_ERR, "ERROR while opening '/dev/null' for stdout");
		exit (1);
	}
	if (dup2 (STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO) {
		syslog (LOG_ERR, "ERROR while opening '/dev/null' for stderr");
		exit (1);
	}

	// 9. option: open syslog for message logging
	openlog (NULL, LOG_NDELAY | LOG_PID, LOG_DAEMON);
	syslog (LOG_INFO, "Daemon has started...");

	// 10. option: get effective user and group id for appropriate's one
	struct passwd* pwd = getpwnam ("root");
	if (pwd == 0) {
		syslog (LOG_ERR, "ERROR while reading daemon password file entry");
		exit (1);
	}

	// 11. option: change root directory
	if (chroot (".") == -1) {
		syslog (LOG_ERR, "ERROR while changing to new root directory");
		exit (1);
	}

	// 12. option: change effective user and group id for appropriate's one
	if (setegid (pwd->pw_gid) == -1) {
		syslog (LOG_ERR, "ERROR while setting new effective group id");
		exit (1);
	}
	if (seteuid (pwd->pw_uid) == -1) {
		syslog (LOG_ERR, "ERROR while setting new effective user id");
		exit (1);
	}

	// get the deamon pid
	if((pid = getpid()) < 0) {
		syslog(LOG_ERR, "Unable to get deamon pid\n");
	}
	else {
		syslog(LOG_INFO, "Fan driver deamon pid : %d\n", pid);
	}

	// 13. implement daemon body...
	
	/* configure gpio's*/
	syslog (LOG_INFO, "Configure led\n");
	led_fd = configure_gpio(LED, "out", NON_EDGE);
	pwrite(led_fd, LED_OFF, sizeof(LED_OFF), 0);

	syslog (LOG_INFO, "Configure buttons\n");
    obj[K1_OBJ_IDX].fd = configure_gpio(K1_NB, "in", "falling");
    obj[K2_OBJ_IDX].fd = configure_gpio(K2_NB, "in", "falling");
    obj[K3_OBJ_IDX].fd = configure_gpio(K3_NB, "in", "falling");
	
	syslog (LOG_INFO, "Configure timer\n");
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);    
    obj[TMR_OBJ_IDX].fd = tfd;
    tm_specs.it_interval.tv_sec = 0;
    tm_specs.it_interval.tv_nsec = 0;
    tm_specs.it_value.tv_sec = TIMER_PERIOD / 1000;
    tm_specs.it_value.tv_nsec = (TIMER_PERIOD % 1000) * 1000000;
    timerfd_settime(obj[TMR_OBJ_IDX].fd, 0, &tm_specs, NULL);

	/* init ipc */ 

	/* epoll */
    // multiplexing with epoll
    syslog(LOG_INFO, "Create epoll\n");
    int epfd = epoll_create1(0);
    if(epfd == -1)
    {
        syslog(LOG_ERR, "Error\n");
        exit(EXIT_FAILURE);
    }

    // control of epoll context
    for(int i=0; i<ARRAY_SIZE(obj); i++){
        epoll_ctl(epfd, EPOLL_CTL_ADD, obj[i].fd, &obj[i].event);
    }

	/* lcd init*/
	lcd_init();

	/* loop to handle switches and communication events */
	syslog (LOG_INFO, "Entering deamon loop\n");
	while(1) {
        struct epoll_event events[2];
        int nr = epoll_wait(epfd, events, ARRAY_SIZE(events), -1);
        if(nr == -1)
            syslog(LOG_ERR, "Error epoll event : %s\n", strerror(errno));
        if(nr == 0)
        {
            syslog(LOG_DEBUG, "Epoll wait timeout reached \n");
        }
        if(nr > 0)
        {
            for(int i=0; i< nr; i++)
            {
                struct object* o = events[i].data.ptr;
                o->obj(o);
            }
        }
		if(lcd_update) {
			lcd_update(driver_get_freq(), driver_get_temperature(), driver_get_mode());
			LCD_UPDATE = FALSE;
		}
	}

	syslog (LOG_INFO, "daemon stopped. Number of signals catched=%d\n", signal_catched);
	closelog();

	return 0;
}
