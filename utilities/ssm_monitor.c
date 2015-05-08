/****************************************************************/
/**
  @file   ssm_monitor.c
  @brief  ssm monitoring
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "ssm2hat.h"
#include "ssm2hat_time.h"



int gShutOff = 0;



typedef struct
{
    char name[SSM_SNAME_MAX];
    int suid;
    size_t size;
    SSM_sid ssmId;
    int tid;
} info_t;



/*--------------------------------------------------------------*/
/**
 * @brief ctrlC
 * @param aStatus
 */
/*--------------------------------------------------------------*/
void ctrlC( int aStatus )
{
    // exit(aStatus);
    gShutOff = 1;
    signal( SIGINT, NULL );
}



/*--------------------------------------------------------------*/
/**
 * @brief setSigInt
 */
/*--------------------------------------------------------------*/
void setSigInt(  )
{
    signal(SIGINT, ctrlC);
}



/*--------------------------------------------------------------*/
/**
 * @brief get_unit
 * @param data
 * @param unit
 * @param base
 * @return
 */
/*--------------------------------------------------------------*/
int get_unit( double *data, char *unit, double base )
{
    int i = 0;
    static const char table[] = { " kMGTPEZY" };
    while( *data >= base && i < 8 ){
        *data /= base;
        i++;
    }
    if( i < 0 || i > 8 ) i = 0;
    *unit = table[i];

    if( i > 0 ) return 1;
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief main
 * @param aArgc
 * @param aArgv
 * @return
 */
/*--------------------------------------------------------------*/
int main( int aArgc, char **aArgv )
{
    int i;
    int sensorNum = 0;

    //! ssm initialize
    if( !initSSM(  ) ){
        fprintf( stderr, "ERROR : cannot init ssm.\n" );
        return 0;
    }

    setSigInt(  );

    while( !gShutOff ){
        sensorNum = getSSM_num(  );

        if( sensorNum < 0 ){
            errSSM();
            return 1;
        }

        //! data save array
        info_t info[sensorNum];

        //! open all sensors
        for ( i = 0; i < sensorNum; i++ ){
            if( getSSM_name( i, info[i].name, &info[i].suid, &info[i].size ) >= 0 ){
                info[i].ssmId = openSSM( info[i].name, info[i].suid, SSM_READ );
                if( info[i].ssmId == 0 ){
                    fprintf( stderr, "ERROR : cannot open ssm '%s' id %d\n", info[i].name, info[i].suid );
                    return 1;
                }
                info[i].tid = getTID_top( info[i].ssmId );
            }
            else{
                fprintf( stderr, "ERROR : cannot get sensor name of No.%d\n", i );
                return 1;
            }
        }

        printf( "\033[2J" );					//! crear display
        while( ( !gShutOff ) && sensorNum == getSSM_num(  ) ){
            int tid;
            double size, total = 0;
            char unit;

            printf( "\033[%d;%dH", 0, 0 );
            printf( "No. |           sensor name            | id | count |    tid      | data[B/s] \n" );
            printf( "----+----------------------------------+----+-------+-------------+----------|\n" );
            for ( i = 0; i < sensorNum; i++ ){
                tid = getTID_top( info[i].ssmId );
                size = info[i].size * ( tid - info[i].tid );
                total += size;
                get_unit( &size, &unit, 1024 );
                printf( "\033[K" );				// 右を全消去
                printf( "%3d | %32.32s |%3d | %5d | %11d | %8.3f%c\n", i, info[i].name, info[i].suid, tid - info[i].tid, tid, size, unit );
                info[i].tid = tid;
            }
            printf( "----+----------------------------------+----+-------+-------------+----------|\n" );
            get_unit( &total, &unit, 1024 );
            printf( "\nssm total wrote size : %7.3f %cB/s\n", total, unit );

            sleep( 1 );
        }
    }

    endSSM(  );

    return 0;
}
