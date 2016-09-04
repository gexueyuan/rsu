/* zebra daemon main routine.
 * Copyright (C) 1997, 98 Kunihiro Ishiguro
 *
 * This file is part of GNU Zebra.
 *
 * GNU Zebra is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * GNU Zebra is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Zebra; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#include "zebra.h"
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sched.h>
#include "cli_version.h"
#include "command.h"
#include "memory.h"
#include "vtysh.h"
#include "cli_lib.h"
#include "rc_pthread.h"


/* Command line options. */
struct option longopts[] =
{
    { "help",        no_argument,       NULL, 'h'},
    { "vty_addr",    required_argument, NULL, 'A'},
    { "vty_port",    required_argument, NULL, 'P'},
    { "version",     no_argument,       NULL, 'v'},
    { NULL, 0, NULL, 0}
};


/* This is called from main when a daemon is invoked with -v or --version. */
static void print_version (const char *progname)
{
  printf ("%s version %s\n", progname, CLI_AGNET_VERSION);
}

/* Help information display. */
static void usage (char *progname, int status)
{
    if (status != 0)
        fprintf (stderr, "Try `%s --help' for more information.\n", progname);
    else
    {
        printf ("Usage : %s [OPTION...]\n\n"\
                "Daemon which manages kernel routing table management and "\
                "redistribution between different routing protocols.\n\n"\
                "-A, --vty_addr     Set vty's bind address\n"\
                "-P, --vty_port     Set vty's port number\n"\
                "-v, --version      Print program version\n"\
                "-h, --help         Display this help and exit\n"\
                "\n", progname);
    }

    exit (status);
}

/* Signale wrapper. */
void *signal_set (int signo, void (*func)(int))
{
  int ret;
  struct sigaction sig;
  struct sigaction osig;

  sig.sa_handler = func;
  sigemptyset (&sig.sa_mask);
  sig.sa_flags = 0;
#ifdef SA_RESTART
  sig.sa_flags |= SA_RESTART;
#endif /* SA_RESTART */

  ret = sigaction (signo, &sig, &osig);

  if (ret < 0)
    return (SIG_ERR);
  else
    return (osig.sa_handler);
}

static void* sigmgr_thread(void *args)
{
    sigset_t   waitset;
    int        sig;
    int        rc;

    sigemptyset(&waitset);
    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGPIPE);
    sigaddset(&waitset, SIGTSTP);

    while (1)
    {
        rc = sigwait(&waitset, &sig);
        if (rc != 0)
        {
            printf("sigwait err: %d; %s\n", errno, strerror(errno));
        }
        else
        {
            switch (sig)
            {
                case SIGINT:
                    vty->iBreak = 1;
                    if (vty->stChildPId > 0)
                    {
                        kill(vty->stChildPId, SIGINT);
                        vty->stChildPId = 0;
                    }
                    break;
                case SIGPIPE:
                    break;
                default:
                    break;
            }
        }
    }

    return NULL;
}
void vtysh_reset(void);

void sigsegv_pro(int sig)
{
    vtysh_reset();
    return;
}

/* Initialization of signal handles. */
void signal_init (void)
{
    sigset_t bset, oset;
    pthread_t       ppid;

    //解除对SIGINT的阻塞
    sigemptyset(&bset);
    sigaddset(&bset, SIGINT);
    //sigprocmask(SIG_UNBLOCK, &bset, NULL);

    //sigaddset(&bset, SIGINT);
    sigaddset(&bset, SIGPIPE);
    sigaddset(&bset, SIGTSTP);
    if (pthread_sigmask(SIG_BLOCK, &bset, &oset) != 0)
        printf("!! Set pthread mask failed\n");

    //signal_set(SIGSEGV, sigsegv_pro);
    signal_set(SIGABRT, sigsegv_pro);

    (void) rc_pthread_create(&ppid, "tk_sigmgr", TK_PRIO_DEFAULT, sigmgr_thread, NULL);

    return;
}

/* Main startup routine. */
int main (int argc, char **argv)
{
    char *p;
    char *progname;
    int opt;
    
    rc_setprocparam(0, NULL, TK_PRIO_DEFAULT);

    /* Set umask before anything for security */
    umask (0027);

    /* preserve my name */
    progname = ((p = strrchr (argv[0], '/')) ? ++p : argv[0]);

    while (1)
    {
        opt = getopt_long (argc, argv, "hA:P:v", longopts, 0);

        if (opt == EOF)
            break;

        switch (opt)
        {
            case 0:
                break;
            case 'v':
                print_version (progname);
                exit (0);
                break;
            case 'h':
                usage (progname, 0);
                break;
            default:
                usage (progname, 1);
                break;
        }
    }

    signal_init();


    cmd_init();
    vty_init();

    /* Sort VTY commands. */
    sort_node();

    /* telnet初始化 */
    TelnetInit();

    /* BEGIN: Added by wanglei, 2015/12/6 */
    /* oam client proc task */
    cv_oam_init();
    /* END:   Added by wanglei, 2015/12/6 */
    
    vty_shell_sock(NULL);

    /* Not reached... */
    return 0;
}
