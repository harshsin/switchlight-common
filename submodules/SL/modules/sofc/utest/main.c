/**************************************************************************//**
 * 
 * 
 * 
 *****************************************************************************/
#include <sofc/sofc_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

int aim_main(int argc, char* argv[])
{
    printf("sofc Utest Is Empty\n");
    sofc_config_show(&aim_pvs_stdout);
    return 0;
}

