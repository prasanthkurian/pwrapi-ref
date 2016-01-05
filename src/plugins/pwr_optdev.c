/*
 * Copyright 2014-2015 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work
 * by or on behalf of the U.S. Government. Export of this program may require
 * a license from the United States Government.
 *
 * This file is part of the Power API Prototype software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
*/

#include "pwr_optdev.h"
#include "pwr_dev.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

#define OPT_CPU_MODEL_6272            1
#define OPT_CPU_MODEL_A10_5800        16

/* D18F0x0[15:0] */
#define MSR_OPT_BASE_ADDR             0xd18f000
#define MSR_OPT_CORE                  0x18
#define MSR_OPT_VENDOR_ID             0x1022
#define MSR_OPT_VENDOR_MASK           0xffff

/* D18F3xE8[31:29] */
#define MSR_OPT_MULTI_NODE            0x30e8
#define MSR_OPT_MULTINODE_BIT         29
#define MSR_OPT_INTERNAL_BIT1         30
#define MSR_OPT_INTERNAL_BIT2         31

/* D18F4x1B8[15:0] */
#define MSR_OPT_PROC_TDP              0x41b8
#define MSR_OPT_PROC_TDP_MASK         0xffff
#define MSR_OPT_BASE_TDP_SHIFT        16

/* D18F5xE0[3:0] */
#define MSR_OPT_RUN_AVG_RANGE         0x50e0
#define MSR_OPT_RUN_AVG_RANGE_MASK    0x000f

/* D18F5xE8[15:0] */
#define MSR_OPT_TDP_TO_WATT           0x50e8
#define MSR_OPT_TDP_TO_WATT_HI_MASK   0x03ff
#define MSR_OPT_TDP_TO_WATT_LO_MASK   0xfc00
#define MSR_OPT_TDP_TO_WATT_HI_SHIFT  10
#define MSR_OPT_TDP_TO_WATT_LO_SHIFT  -6

#define MSR(X,Y,Z) ((X&Y)>>Z)
#define MSR_BIT(X,Y) ((X&(1LL<<Y))?1:0)

#ifdef USE_DEBUG
static int optdev_verbose = 1;
#else
static int optdev_verbose = 0;
#endif

#define OPT_NODE_MAX 8

typedef struct {
    int number;

    unsigned short tdp_to_watt;
    unsigned short run_avg_range;
    unsigned short avg_divide_by;
    unsigned short proc_tdp;
    unsigned short base_tdp;
    unsigned short off_core_pwr_watt;
} node_t;

typedef struct {
    int fd;

    int cpu_model;
    int node_count;
    node_t node[OPT_NODE_MAX];

    int core;
} pwr_optdev_t;
#define PWR_OPTDEV(X) ((pwr_optdev_t *)(X))

typedef struct {
    pwr_optdev_t *dev;
} pwr_optfd_t;
#define PWR_OPTFD(X) ((pwr_optfd_t *)(X))

static plugin_devops_t devops = {
    .open         = pwr_optdev_open,
    .close        = pwr_optdev_close,
    .read         = pwr_optdev_read,
    .write        = pwr_optdev_write,
    .readv        = pwr_optdev_readv,
    .writev       = pwr_optdev_writev,
    .time         = pwr_optdev_time,
    .clear        = pwr_optdev_clear,
#if 0
    .stat_get     = pwr_dev_stat_get,
    .stat_start   = pwr_dev_stat_start,
    .stat_stop    = pwr_dev_stat_stop,
    .stat_clear   = pwr_dev_stat_clear,
#endif
    .private_data = 0x0
};

static int optdev_parse( const char *initstr, int *core )
{
    char *token;

    if( optdev_verbose )
        printf( "Info: received initialization string %s\n", initstr );

    if( (token = strtok( (char *)initstr, ":" )) == 0x0 ) {
        printf( "Error: missing core separator in initialization string %s\n", initstr );
        return -1;
    }
    *core = atoi(token);

    if( optdev_verbose )
        printf( "Info: extracted initialization string (CORE=%d)\n", *core );

    return 0;
}

static int optdev_identify( int *cpu_model )
{
    FILE *file;
    char *str = 0x0;
    char cpuinfo[80] = "";
    int retval = 0;

    int family;
    char vendor[80] = "";

    if( (file=fopen( "/proc/cpuinfo", "r" )) == 0x0 ) {
        printf( "Error: OPT cpuinfo open failed\n" );
        return -1;
    }

    *cpu_model = -1;
    while( (str=fgets( cpuinfo, 80, file )) != 0x0 ) {
        if( strncmp( str, "vendor_id", 8) == 0 ) {
            sscanf( str, "%*s%*s%s", vendor );
            if( strncmp( vendor, "AuthenticAMD", 12 ) != 0 ) {
                printf( "Warning: %s, only AMD supported\n", vendor );
                retval = -1;
            }
        }
        else if( strncmp( str, "cpu_family", 10) == 0 ) {
            sscanf( str, "%*s%*s%*s%d", &family );
            if( family != 21 ) {
                printf( "Warning: Unsupported CPU family %d\n", family );
                retval = -1;
            }
        }
        else if( strncmp( str, "model", 5) == 0 ) {
            sscanf( str, "%*s%*s%d", cpu_model );

            switch( *cpu_model ) {
                case OPT_CPU_MODEL_6272:
                case OPT_CPU_MODEL_A10_5800:
                    break;
                default:
                    printf( "Warning: Unsupported model %d\n", *cpu_model );
                    retval = -1;
                    break;
            }
        }
    }

    fclose( file );
    return retval;
}

static int optdev_read( int fd, int offset, long long *msr )
{
    uint64_t value;
    int size = sizeof(uint64_t);

    if( pread( fd, &value, size, offset ) != size ) {
        printf( "Error: OPT read failed\n" );
        return -1;
    }
 
    *msr = (long long)value;
    return 0;
}

static int optdev_gather( int fd, int node,
                           double *power, double *time )
{
    long long msr;

    /* FILL IN READ OF POWER */
#if 0
    if( optdev_read( fd, MSR_ENERGY_STATUS, &msr ) < 0 ) {
        printf( "Error: PWR OPT device read failed\n" );
        return -1;
    }
    *energy = (double)msr;
#endif

    return 0;
}

plugin_devops_t *pwr_optdev_init( const char *initstr )
{
    char file[80] = "";
    int core = 0, i;
    long long msr;
    plugin_devops_t *dev = malloc( sizeof(plugin_devops_t) );
    *dev = devops;

    dev->private_data = malloc( sizeof(pwr_optdev_t) );
    bzero( dev->private_data, sizeof(pwr_optdev_t) );

    if( optdev_verbose ) 
        printf( "Info: PWR OPT device open\n" );
    if( optdev_parse( initstr, &core ) < 0 ) {
        printf( "Error: PWR OPT device initialization string %s invalid\n", initstr );
        return 0x0;
    }

    if( optdev_identify( &(PWR_OPTDEV(dev->private_data)->cpu_model) ) < 0 ) {
        printf( "Error: PWR OPT device model identification failed\n" );
        return 0x0;
    }

    sprintf( file, "/dev/cpu/%d/msr", core );
    if( (PWR_OPTDEV(dev->private_data)->fd=open( file, O_RDONLY )) < 0 ) {
        printf( "Error: PWR OPT device open failed\n" );
        return 0x0;
    }

    for( i=0; i<OPT_NODE_MAX; i++ ) {
        if( optdev_read( PWR_OPTDEV(dev->private_data)->fd, MSR_OPT_CORE+i, &msr ) < 0 ) {
            printf( "Error: PWR OPT device read failed\n" );
            return 0x0;
        }

        if( (unsigned short)(MSR(msr, MSR_OPT_VENDOR_MASK, 0)) == MSR_OPT_VENDOR_ID ) {
            if( optdev_read( PWR_OPTDEV(dev->private_data)->fd, MSR_OPT_CORE+i, &msr ) < 0 ) {
                printf( "Error: PWR OPT device read failed\n" );
                return 0x0;
            }
            if( (unsigned short)(MSR_BIT(msr, MSR_OPT_MULTINODE_BIT)) ) {
                if( (unsigned short)(MSR_BIT(msr, MSR_OPT_INTERNAL_BIT1)) == 0 &&
                    (unsigned short)(MSR_BIT(msr, MSR_OPT_INTERNAL_BIT2)) == 0 ) {
                    PWR_OPTDEV(dev->private_data)->node[PWR_OPTDEV(dev->private_data)->node_count++].number = i;
                }
            } else {
                PWR_OPTDEV(dev->private_data)->node[PWR_OPTDEV(dev->private_data)->node_count++].number = i;
            }
        }
    }

    for( i=0; i<PWR_OPTDEV(dev->private_data)->node_count; i++ ) {
        if( optdev_read( PWR_OPTDEV(dev->private_data)->fd, MSR_OPT_TDP_TO_WATT +
                         PWR_OPTDEV(dev->private_data)->node[i].number, &msr ) < 0 ) {
            printf( "Error: PWR OPT device read failed\n" );
            return 0x0;
        }
        PWR_OPTDEV(dev->private_data)->node[i].tdp_to_watt =
            (unsigned short)(MSR(msr, MSR_OPT_TDP_TO_WATT_HI_MASK, MSR_OPT_TDP_TO_WATT_HI_SHIFT)) |
            (unsigned short)(MSR(msr, MSR_OPT_TDP_TO_WATT_LO_MASK, MSR_OPT_TDP_TO_WATT_LO_SHIFT));

        if( optdev_read( PWR_OPTDEV(dev->private_data)->fd, MSR_OPT_RUN_AVG_RANGE +
                         PWR_OPTDEV(dev->private_data)->node[i].number, &msr ) < 0 ) {
            printf( "Error: PWR OPT device read failed\n" );
            return 0x0;
        }
        PWR_OPTDEV(dev->private_data)->node[i].run_avg_range =
            (unsigned short)(MSR(msr, MSR_OPT_RUN_AVG_RANGE_MASK, 0));
        PWR_OPTDEV(dev->private_data)->node[i].avg_divide_by =
            pow( PWR_OPTDEV(dev->private_data)->node[i].run_avg_range + 1, 2.0 );

        if( optdev_read( PWR_OPTDEV(dev->private_data)->fd, MSR_OPT_PROC_TDP +
                         PWR_OPTDEV(dev->private_data)->node[i].number, &msr ) < 0 ) {
            printf( "Error: PWR OPT device read failed\n" );
            return 0x0;
        }
        PWR_OPTDEV(dev->private_data)->node[i].proc_tdp =
            (unsigned short)(MSR(msr, MSR_OPT_PROC_TDP_MASK, 0));

        if( optdev_verbose ) {
            printf( "Info: node[%d].number            - %d\n", i, PWR_OPTDEV(dev->private_data)->node[i].number );
            printf( "Info: node[%d].tdp_to_watt       - %u\n", i, PWR_OPTDEV(dev->private_data)->node[i].tdp_to_watt );
            printf( "Info: node[%d].run_avg_range     - %u\n", i, PWR_OPTDEV(dev->private_data)->node[i].run_avg_range );
            printf( "Info: node[%d].avg_divide_by     - %u\n", i, PWR_OPTDEV(dev->private_data)->node[i].avg_divide_by );
            printf( "Info: node[%d].proc_tdp          - %u\n", i, PWR_OPTDEV(dev->private_data)->node[i].proc_tdp );
            printf( "Info: node[%d].base_tdp          - %u\n", i, PWR_OPTDEV(dev->private_data)->node[i].base_tdp );
            printf( "Info: node[%d].off_core_pwr_watt - %u\n", i, PWR_OPTDEV(dev->private_data)->node[i].off_core_pwr_watt );
        }
    }

    return dev;
}

int pwr_optdev_final( plugin_devops_t *dev )
{
    if( optdev_verbose ) 
        printf( "Info: PWR OPT device close\n" );

    close( PWR_OPTDEV(dev->private_data)->fd );
    free( dev->private_data );
    free( dev );

    return 0;
}

pwr_fd_t pwr_optdev_open( plugin_devops_t *dev, const char *openstr )
{
    pwr_fd_t *fd = malloc( sizeof(pwr_optfd_t) );
    PWR_OPTFD(fd)->dev = PWR_OPTDEV(dev->private_data);

    return fd;
}

int pwr_optdev_close( pwr_fd_t fd )
{
    PWR_OPTFD(fd)->dev = 0x0;
    free( fd );

    return 0;
}

int pwr_optdev_read( pwr_fd_t fd, PWR_AttrName attr, void *value, unsigned int len, PWR_Time *timestamp )
{
    double energy = 0.0;
    double time = 0.0;
    int policy = 0;

    if( optdev_verbose ) 
        printf( "Info: PWR OPT device read\n" );

    if( len != sizeof(double) ) {
        printf( "Error: value field size of %u incorrect, should be %ld\n", len, sizeof(double) );
        return -1;
    }

    if( optdev_gather( (PWR_OPTFD(fd)->dev)->fd,
                        (PWR_OPTFD(fd)->dev)->core,
                        &energy, &time ) < 0 ) {
        printf( "Error: PWR OPT device gather failed\n" );
        return -1;
    }

    switch( attr ) {
        case PWR_ATTR_ENERGY:
            *((double *)value) = energy;
            break;
        default:
            printf( "Warning: unknown PWR reading attr requested\n" );
            break;
    }
    *timestamp = (unsigned int)time*1000000000ULL + 
                 (time-(unsigned int)time)*1000000000ULL;

    return 0;
}

int pwr_optdev_write( pwr_fd_t fd, PWR_AttrName attr, void *value, unsigned int len )
{
    return 0;
}

int pwr_optdev_readv( pwr_fd_t fd, unsigned int arraysize,
    const PWR_AttrName attrs[], void *values, PWR_Time timestamp[], int status[] )
{
    unsigned int i;

    for( i = 0; i < arraysize; i++ )
        status[i] = pwr_optdev_read( fd, attrs[i], (double *)values+i, sizeof(double), timestamp+i );

    return 0;
}

int pwr_optdev_writev( pwr_fd_t fd, unsigned int arraysize,
    const PWR_AttrName attrs[], void *values, int status[] )
{
    unsigned int i;

    for( i = 0; i < arraysize; i++ )
        status[i] = pwr_optdev_write( fd, attrs[i], (double *)values+i, sizeof(double) );

    return 0;
}

int pwr_optdev_time( pwr_fd_t fd, PWR_Time *timestamp )
{
    double value;

    if( optdev_verbose ) 
        printf( "Info: PWR OPT device time\n" );

    return pwr_optdev_read( fd, PWR_ATTR_POWER, &value, sizeof(double), timestamp );
}

int pwr_optdev_clear( pwr_fd_t fd )
{
    return 0;
}

static plugin_dev_t dev = {
    .init   = pwr_optdev_init,
    .final  = pwr_optdev_final,
};

plugin_dev_t* getDev() {
    return &dev;
}