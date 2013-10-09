/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <sl/sl_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

int aim_main(int argc, char* argv[])
{
    printf("sl Utest Is Empty\n");
    sl_config_show(&aim_pvs_stdout);
    return 0;
}

