/******************************************************************************
 *
 *  OFAD for LB9.
 *
 *****************************************************************************/

#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 

#include <IVS/ivs_config.h>
#define AIM_LOG_MODULE_NAME ivs
#include <AIM/aim_log.h>

#include <AIM/aim_daemon.h>
#include <AIM/aim_pvs_syslog.h>
#include <Faultd/faultd.h>

#define IVS_ERROR AIM_LOG_ERROR
#define IVS_INTERNAL AIM_LOG_INTERNAL

#include <BRCM/brcm.h>
#include <bcm/init.h>

#define TRY(op)                                                         \
    do {                                                                \
        int _rv;                                                        \
        AIM_LOG_TRACE("%s", #op);                                       \
        if ((_rv = (op)) < 0) {                                         \
            AIM_LOG_ERROR("%s", #op);                                   \
            return _rv;                                                 \
        }                                                               \
    } while (0)


#include <signal.h>

int 
aim_main(int argc, char* argv[])
{
    ivs_t* ivs; 
    int rv; 

    /*
     * If the first argument is "-d" we'll daemonize first. 
     * If the first argument is "-dr" we'll daemonize with restart first. 
     * This is not the right place/sequencing/plumbing for this but 
     * we'll go with it until we can straighten up the init seauence. 
     */
    if(argv[1] && (!strcmp(argv[1], "-d") || !strcmp(argv[1], "-dr"))) { 
        aim_daemon_restart_config_t rconfig; 
        aim_daemon_config_t config;     
        
        memset(&config, 0, sizeof(config)); 
        /*
         * This restarts on all exits and all signals. Need to find tune.
         * Allow TERM to terminate properly. 
         */
        aim_daemon_restart_config_init(&rconfig, 1, 1); 
        AIM_BITMAP_CLR(&rconfig.signal_restarts, SIGTERM);
        rconfig.maximum_restarts=50; 

        /* TODO -- decide on these values, either hardcoded or programmable. */
        mkdir("/var/run/ofad", 0022); 
        rconfig.restart_log = "/var/run/ofad/ofad.restart.log"; 
        rconfig.pvs = aim_pvs_syslog_get(); 
        config.stdlog = "/var/run/ofad/ofad.std.log"; 
        config.wd = "/"; 

        if(argv[1][2] == 'r') {         
            /* with restart */
            aim_daemonize(&config, &rconfig); 
        }
        else {
            /* no restart */
            aim_daemonize(&config, NULL); 
        }
        /* skip daemon argument */
        argv++; 
    }

    if(faultd_handler_register(0, "/var/run/faultd.fifo", argv[0]) < 0) { 
        perror("handler_register: "); 
        abort(); 
    }

    /** 
     * Ignore sigpipes everywhere
     */
    signal(SIGPIPE, SIG_IGN); 
     
     
     
    /**
     * Execute a single IVS instance with options. 
     */
    if((rv = brcm_init(&aim_pvs_stdout, NULL)) < 0) { 
        printf("brcm_init failed: %d\n", rv); 
        exit(1); 
    }

    /* Initialize IVS */

    TRY(ucli_init());
    TRY(ivs_create(&ivs, 0, argv + 1));
    TRY(ivs_init(ivs)); 
    TRY(ivs_enable(ivs, 1)); 

    /* Run IVS */
    TRY(ivs_run(ivs, -1)); 

    /* Tear down IVS */

    TRY(ivs_destroy(ivs)); 
    ucli_denit(); 

    return (0);
}

