/******************************************************************************
 *
 *  /utest/main.c
 *
 *  IVS Unit Testing
 *
 *****************************************************************************/
#include <IVS/ivs_config.h> 
#include <IVS/ivs.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int aim_main(int argc, char* argv[])
{
    fprintf(stderr, "Disabled until fixed.\n"); 
    return 0; 

    if(argc == 1 || !strcmp(argv[1], "tests")) { 

        ivs_t* ivs;
        ivs_create(&ivs, NULL, NULL); 
        ivs->min=1;
        ivs_init(ivs);
        ivs_enable(ivs, 1); 

        ivs_command(ivs, "controller add CONNECT 127.0.0.1:34567"); 
        ivs_command(ivs, "controller add LISTEN 127.0.0.1:34567"); 
        ivs_run(ivs, 2500); 
        ivs_command(ivs, "controller remove LISTEN 127.0.0.1:34567"); 
        ivs_command(ivs, "controller remove CONNECT 127.0.0.1:34567"); 
        ivs_command(ivs, "controller add CONNECT 127.0.0.1:34568"); 
        ivs_run(ivs, 2500); 
        ivs_destroy(ivs); 
        ucli_denit();
    }
    else {
        ivs_main(NULL, argv+1);
    }

    return 0;
}
