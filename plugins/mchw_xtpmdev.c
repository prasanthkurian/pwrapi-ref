#include "mchw_xtpmdev.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int mchw_xtpmdev_init( mchw_dev_t *dev, char *initstr )
{
    return 0;
}

int mchw_xtpmdev_final( mchw_dev_t *dev )
{
    return 0;
}

int mchw_xtpmdev_read( mchw_dev_t dev, unsigned int arraysize,
	mchw_read_type_t type[], float reading[], unsigned long long *timestamp )
{
    return 0;
}

int mchw_xtpmdev_time( mchw_dev_t dev, unsigned long long *time )
{
    return 0;
}
