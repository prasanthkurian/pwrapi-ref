/*
 * Copyright 2014-2016 Sandia Corporation. Under the terms of Contract
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this work
 * by or on behalf of the U.S. Government. Export of this program may require
 * a license from the United States Government.
 *
 * This file is part of the Power API Prototype software package. For license
 * information, see the LICENSE file in the top level directory of the
 * distribution.
*/

#include "pwr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

int section_4_2_test( void )
{
    int rc;
    PWR_Cntxt cntxt;
    PWR_Obj self, parent, obj;
	PWR_ObjType type;
    char name[PWR_MAX_STRING_LEN] = "";
    PWR_Grp children;

    printf( "\tPWR_CntxtInit - application context\n" );
	rc = PWR_CntxtInit( PWR_CNTXT_DEFAULT, PWR_ROLE_APP, "Application", &cntxt );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: initialization of PowerAPI context failed\n" );
        return -1;
    }

    printf( "\tPWR_GetEntryPoint\n" );
	rc = PWR_CntxtGetEntryPoint( cntxt, &self );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: getting self from PowerAPI context failed\n" );
        return -1;
    }

    printf( "\tPWR_ObjGetType\n" );
	rc = PWR_ObjGetType( self, &type );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: getting object type failed\n" );
        return -1;
    }

    printf( "\tPWR_ObjGetName\n" );
	rc = PWR_ObjGetName( self, name, PWR_MAX_STRING_LEN );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: getting object name failed\n" );
        return -1;
    }

    printf( "\tPWR_ObjGetParent\n" );
	rc = PWR_ObjGetParent( self, &parent );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: getting object parent failed\n" );
        return -1;
    }

    printf( "\tPWR_ObjGetChildren\n" );
	rc = PWR_ObjGetChildren( self, &children );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: getting object parent failed\n" );
        return -1;
    }

    printf( "\tPWR_ObjGrpDestroy - children group\n" );
    rc = PWR_GrpDestroy( children );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: destroying children failed\n" );
        return -1;
    }

    printf( "\tPWR_CntxtGetObjByName - \"plat\" name\n" );
	rc = PWR_CntxtGetObjByName( cntxt, "plat", &obj );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: getting object by name from PowerAPI context failed\n" );
        return -1;
    }

    printf( "\tPWR_CntxtDestroy - application context\n" );
    rc = PWR_CntxtDestroy( cntxt );
    if( rc != PWR_RET_SUCCESS ) {
        printf( "\t\tError: destruction of PowerAPI context failed\n" );
        return -1;
    }

    return 0;
}
