/**************************************************************************//**
 *
 *
 *
 *****************************************************************************/
#include <icmpv6/icmpv6_config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AIM/aim.h>

int aim_main(int argc, char* argv[])
{
    printf("icmpv6 Utest Is Empty\n");
    icmpv6_config_show(&aim_pvs_stdout);
    return 0;
}

