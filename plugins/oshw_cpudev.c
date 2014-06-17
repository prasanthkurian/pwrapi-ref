#include "oshw_cpudev.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int online_cpulist[1000];
int num_cpus;
int started = 0;

int online_cpus(int number_desired)
{
    int i = 0;
    
    //int num_cpus = sysconf(_SC_NPROCESSORS_CONF);

    for (i = 1; i <= number_desired - 1; i++){
        if (online_cpulist[i] == 0){ 
            int fd;
            char one = '1';
            char cpupath[100];
            sprintf(cpupath,"/sys/devices/system/cpu/cpu%i/online",i);
            fd = open(cpupath, O_WRONLY);
            write (fd, &one, 1);
            close(fd);
            online_cpulist[i] = 1;
        }
        else{
            //printf("skip, already on\n");
        }
    }
    for (i = number_desired; i < num_cpus; i++){
        if (online_cpulist[i] == 1){
            int fd;
            char zero = '0';
            char cpupath[100];
            sprintf(cpupath,"/sys/devices/system/cpu/cpu%i/online",i);
            fd = open(cpupath, O_WRONLY);
            write (fd, &zero, 1);
            close(fd);
            online_cpulist[i] = 0;
        }
        else{
            //printf("skip, already off\n");
        }
    }

    return 0;
}

int parallel_hint(PWR_Obj obj, PWR_Hint hint, int parallel)
{
    if( started == 0 ) {
        //printf("first time hint is run, bootstrapping \n");
        num_cpus = sysconf(_SC_NPROCESSORS_CONF);
        int i;
        for (i = 1; i <= num_cpus - 1; i++){
            int fd;
            char one = '1';
            char cpupath[100];
            sprintf(cpupath,"/sys/devices/system/cpu/cpu%i/online",i);
            fd = open(cpupath, O_WRONLY);
            write (fd, &one, 1);
            close(fd);
            online_cpulist[i] = 1;
        }
        started = 1;
    }

    /* For now we assume that this always operates on the node */
    switch( hint ) {
       case PWR_REGION_PARALLEL:
           online_cpus(parallel);
           break;
       case PWR_REGION_SERIAL:
           online_cpus(1);
           break;
       case PWR_REGION_COMPUTE:
           break;
       case PWR_REGION_COMMUNICATE:
           online_cpus(2);
           break;
       default:
           fprintf(stderr,"INVALID HINT\n");
    }

    return 0;
}

pwr_dev_t oshw_cpudev_open( const char *initstr )
{
    return 0x0;
}

int oshw_cpudev_close( pwr_dev_t dev )
{
    return 0;
}

int oshw_cpudev_read( pwr_dev_t dev, PWR_AttrName attr, void *value, unsigned int len, PWR_Time *timestamp )
{
    return 0;
}

int oshw_cpudev_write( pwr_dev_t dev, PWR_AttrName attr, void *value, unsigned int len )
{
    return 0;
}

int oshw_cpudev_readv( pwr_dev_t dev, unsigned int arraysize,
    const PWR_AttrName attrs[], void *values, PWR_Time timestamp[], int status[] )
{
    return 0;
}

int oshw_cpudev_writev( pwr_dev_t dev, unsigned int arraysize,
    const PWR_AttrName attrs[], void *values, int status[] )
{
    return 0;
}

int oshw_cpudev_time( pwr_dev_t dev, PWR_Time *timestamp )
{
    return 0;
}

int oshw_cpudev_clear( pwr_dev_t dev )
{
    return 0;
}

static plugin_dev_t dev = {
    .open   = oshw_cpudev_open,
    .close  = oshw_cpudev_close,
    .read   = oshw_cpudev_read,
    .write  = oshw_cpudev_write,
    .readv  = oshw_cpudev_readv,
    .writev = oshw_cpudev_writev,
    .time   = oshw_cpudev_time,
    .clear  = oshw_cpudev_clear
};

plugin_dev_t* getDev() {
    return &dev;
}