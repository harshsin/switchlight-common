/**************************************************************************//**
 *
 *
 *
 *
 *****************************************************************************/
#include <AIM/aim.h>
#include <Faultd/faultd.h>

#define AIM_LOG_MODULE_NAME faultd
#include <AIM/aim_log.h>
#include <AIM/aim_pvs.h>
#include <AIM/aim_pvs_syslog.h>

#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

AIM_LOG_STRUCT_DEFINE(
                      AIM_LOG_OPTIONS_DEFAULT, 
                      AIM_LOG_BITS_DEFAULT, 
                      NULL, 0); 


static int test__(const char* binaryname, const char* pipename); 

/**
 * This is the basic faultd server for Pandora. 
 * 
 * Its hardcoded to :
 * - Listen on /var/run/faultd.fifo
 * - Output messages to syslog and stderr (if a tty)
 * - Daemonize and Restart on "-d", "-dr"
 */    

#ifndef PIPENAME_DEFAULT
#define PIPENAME_DEFAULT "/var/run/faultd.fifo"
#endif


static char help__[] = 
"NAME\n\
        faultd - Fault Reporting Agent\n\
\n\
SYNOPSIS\n\
\n\
        faultd [-dr|-d] [-pid file] [-t] [-h | --help]\n\
\n\
OPTIONS\n\
        -d            Daemonize.\n\
\n\
        -dr           Daemonize with automatic restart.\n\
        -p            Server pipe. Default is %s\n\
\n\
        -pid file     Write PID to the given filename.\n\
\n\
        -t            Test mode. Sends a test backtrace the the existing faultd\n\
                      server.\n\
\n\
        -h            This help message.\n\
        -help\n\
\n"; 
        

int
aim_main(int argc, char* argv[])
{       
    char** arg; 
    
    char* pidfile = NULL; 
    int daemonize = 0; 
    int restart = 0; 
    int test = 0; 
    char* pipename = PIPENAME_DEFAULT; 
    
    aim_pvs_t* aim_pvs_syslog = NULL; 
    faultd_server_t* faultd_server = NULL; 
    int sid = -1; 


    for(arg = argv+1; *arg; arg++) { 
        if(!strcmp(*arg, "-dr")) { 
            daemonize=1; 
            restart=1; 
        }
        else if(!strcmp(*arg, "-d")) { 
            daemonize=1; 
            restart=0; 
        }
        else if(!strcmp(*arg, "-pid")) { 
            arg++; 
            pidfile = *arg; 
            if(!pidfile) { 
                fprintf(stderr, "-pid requires an argument.\n"); 
                exit(1);
            }
        }
        else if(!strcmp(*arg, "-p")) { 
            arg++; 
            pipename = *arg; 
            if(!pipename) { 
                fprintf(stderr, "-p requires an argument.\n"); 
                exit(1); 
            }
        }
        else if(!strcmp(*arg, "-t")) { 
            test = 1; 
        }
        else if(!strcmp(*arg, "-h") || !strcmp(*arg, "--help")) { 
            printf(help__, PIPENAME_DEFAULT); 
            exit(0); 
        }
    }

    if(test) { 
        return test__(argv[0], pipename); 
    }

    /**
     * Start Server
     */
    aim_pvs_syslog = aim_pvs_syslog_open("faultd", LOG_PID, LOG_DAEMON);
    aim_printf(aim_pvs_syslog, "faultd starting");
    faultd_server_create(&faultd_server); 
    sid = faultd_server_add(faultd_server, pipename); 
    
    if(sid < 0) { 
        perror("server_add:"); 
        abort(); 
    }

    if(daemonize) { 
        aim_daemon_restart_config_t rconfig; 
        aim_daemon_config_t config; 
        
        memset(&config, 0, sizeof(config)); 
        aim_daemon_restart_config_init(&rconfig, 1, 1); 
        AIM_BITMAP_CLR(&rconfig.signal_restarts, SIGTERM); 
        AIM_BITMAP_CLR(&rconfig.signal_restarts, SIGKILL); 
        rconfig.maximum_restarts=0; 
        rconfig.pvs = aim_pvs_syslog_get(); 

        config.wd = "/";
        if(restart) { 
            aim_daemonize(&config, &rconfig); 
        }
        else {
            aim_daemonize(&config, NULL); 
        }
    }

    /*
     * Write our PID file if requested. 
     */
    if(pidfile) { 
        FILE* fp = fopen(pidfile, "w"); 
        if(fp == NULL) { 
            int e = errno; 
            aim_printf(aim_pvs_syslog, "fatal: open(%s): %s\n", 
                       pidfile, strerror(e)); 
            aim_printf(&aim_pvs_stderr, "fatal: open(%s): %s\n", 
                       pidfile, strerror(e)); 

            /* Don't attempt restart */
            raise(SIGTERM); 
        }
        fprintf(fp, "%d\n", getpid()); 
        fclose(fp); 
    }
            
    /**
     * Process Fault Messages
     */
    for(;;) { 
        faultd_info_t faultd_info;
        memset(&faultd_info, 0, sizeof(faultd_info)); 
        if(faultd_server_read(faultd_server, &faultd_info, sid) >= 0) { 
            faultd_info_show(&faultd_info, aim_pvs_syslog, 0); 
            if(aim_pvs_isatty(&aim_pvs_stderr)) { 
                faultd_info_show(&faultd_info, &aim_pvs_stderr, 0); 
            }
        }
    }
}
                
static int
test__(const char* binaryname, const char* pipename)
{
    /**
     * Send a backtrace to the current faultd server. 
     */
    faultd_handler_register(0, pipename, binaryname); 
    raise(SIGUSR2); 
    return 0;
}
