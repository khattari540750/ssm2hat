/****************************************************************/
/**
  @file   libssm2hat_time.c
  @brief  ssm2hat library time
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



#include <stdio.h>
//#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#include "ssm2hat.h"
#include "ssm2hat_lib.h"



struct ssmtime *timecontrol = NULL;
int shm_timeid;



#if _POSIX_C_SOURCE >= 199309L

/*--------------------------------------------------------------*/
/**
 * @brief timetof
 * @param t
 * @return
 */
/*--------------------------------------------------------------*/
static double timetof( struct timespec t )
{
    return ( double )t.tv_sec + ( double )t.tv_nsec / 1000000000.0;
}



/*--------------------------------------------------------------*/
/**
 * @brief  gettimeSSM_real
 * @return
 */
/*--------------------------------------------------------------*/
ssmTimeT gettimeSSM_real( void )
{
    struct timespec current;

    clock_gettime( CLOCK_REALTIME, &current );
    return timetof( current );
}



/*--------------------------------------------------------------*/
/**
 * @brief nanosleepSSM
 * @param req
 * @param rem
 * @return
 */
/*--------------------------------------------------------------*/
int nanosleepSSM(const struct timespec *req, struct timespec *rem)
{
    if(timecontrol != NULL && timecontrol->speed != 0.0){
        struct timespec t;
        double sec, speed = timecontrol->speed;
        if(speed < 0.0)speed = -speed;
        sec = req->tv_sec / timecontrol->speed;

        t.tv_sec = sec;
        t.tv_nsec = (sec - t.tv_sec) * 1000000000.0;
        t.tv_nsec += (req->tv_nsec / speed);
        t.tv_sec += t.tv_nsec / 1000000000;
        t.tv_nsec %= 1000000000;

        return nanosleep(&t, rem);
    }
    return nanosleep(req, rem);;
}



/*--------------------------------------------------------------*/
/**
 * @brief sleepSSM
 * @param sec
 * @return
 */
/*--------------------------------------------------------------*/
unsigned int sleepSSM( double sec )
{
    struct timespec time;
    time.tv_sec = (int)sec;
    time.tv_nsec = (int)((sec - time.tv_sec) * 1000000000.0);

    return 	nanosleepSSM(&time, NULL);
}



/*--------------------------------------------------------------*/
/**
 * @brief usleepSSM use nanosleep
 * @param usec
 * @return
 */
/*--------------------------------------------------------------*/
int usleepSSM( useconds_t usec )
{
    struct timespec time;
    time.tv_sec = usec / 1000000;
    time.tv_nsec = (usec % 1000000) * 1000;
    return nanosleepSSM(&time ,NULL);
}

#else /** _POSIX_C_SOURCE < 199309L */



/*--------------------------------------------------------------*/
/**
 * @brief timetof
 * @param t
 * @return time
 */
/*--------------------------------------------------------------*/
static double timetof( struct timeval t )
{
    return t.tv_sec + t.tv_usec / 1000000.0;
}



/*--------------------------------------------------------------*/
/**
 * @brief gettimeSSM_real
 * @return time
 */
/*--------------------------------------------------------------*/
ssmTimeT gettimeSSM_real( void )
{
    struct timeval current;

    gettimeofday( &current, NULL );
    return timetof( current );
}



/*--------------------------------------------------------------*/
/**
 * @brief sleepSSM
 * @param sec
 * @return
 */
/*--------------------------------------------------------------*/
unsigned int sleepSSM( double sec )
{
    return usleepSSM( sec * 1000000.0 );
}



/*--------------------------------------------------------------*/
/**
 * @brief  usleepSSM
 * @param  usec
 * @return
 */
/*--------------------------------------------------------------*/
int usleepSSM( useconds_t usec )
{
    if(timecontrol != NULL && timecontrol->speed != 0.0){
        double t, speed = timecontrol->speed;
        if(speed < 0.0)speed = -speed;
        t = (double)usec / timecontrol->speed;
        return usleep( (int)t );
    }
    return usleep( usec );
}


#endif	/** _POSIX_C_SOURCE  199309L */



/*--------------------------------------------------------------*/
/**
 * @brief  gettimeSSM
 * @return
 */
/*--------------------------------------------------------------*/
ssmTimeT gettimeSSM( void )
{
    if(timecontrol != NULL){
        if( timecontrol->is_pause ){ return timecontrol->pausetime; }
        return timecontrol->speed * gettimeSSM_real() + timecontrol->offset;
    }
    return gettimeSSM_real();
}



/*--------------------------------------------------------------*/
/**
 * @brief  settimeSSM
 * @param  time
 * @return
 */
/*--------------------------------------------------------------*/
int settimeSSM( ssmTimeT time )
{
    if(timecontrol != NULL){
        timecontrol->offset = time - timecontrol->speed * gettimeSSM_real();
        timecontrol->pausetime = time;
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief  settimeSSM_speed  rate setting
 * @param  speed
 * @return
 */
/*--------------------------------------------------------------*/
int settimeSSM_speed( double speed )
{
    if(timecontrol != NULL){
        double time = gettimeSSM();
        timecontrol->speed = speed;
        settimeSSM( time );
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief  gettimeSSM_speed
 * @return
 */
/*--------------------------------------------------------------*/
double gettimeSSM_speed( void )
{
    if(timecontrol != NULL){
        return timecontrol->speed;
    }
    return 1.0;
}



/*--------------------------------------------------------------*/
/**
 * @brief  settimeSSM_is_pause
 * @param  is_pause
 * @return
 */
/*--------------------------------------------------------------*/
int settimeSSM_is_pause( int is_pause )
{
    if(timecontrol != NULL){
        ssmTimeT time = gettimeSSM(  );
        timecontrol->pausetime = time;
        settimeSSM( time );
        timecontrol->is_pause = is_pause;
        return 1;
    }
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief  gettimeSSM_is_pause
 * @return
 */
/*--------------------------------------------------------------*/
int gettimeSSM_is_pause( void )
{
    if(timecontrol != NULL) return timecontrol->is_pause;
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief  settimeSSM_is_reverse
 * @param  is_reverse
 * @return 1
 */
/*--------------------------------------------------------------*/
int settimeSSM_is_reverse( int is_reverse )
{
    double speed = gettimeSSM_speed();
    if( (speed  < 0.0 && !is_reverse) || ( speed >= 0.0 && is_reverse ) )
        settimeSSM_speed( -speed );

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief  gettimeSSM_is_reverse  check reverse
 * @return speed is.. +:0 -:1
 */
/*--------------------------------------------------------------*/
int gettimeSSM_is_reverse( void )
{
    if(gettimeSSM_speed() < 0.0) return 1;
    return 0;
}



/*================user prohibition function=====================*/

/*--------------------------------------------------------------*/
/**
 * @brief  opentimeSSM
 * @return false:0 ture:1
 */
/*--------------------------------------------------------------*/
int opentimeSSM( void )
{
    if( (shm_timeid = shmget( SHM_TIME_KEY, sizeof(struct ssmtime), IPC_CREAT | 0666 )) < 0 ){
        perror("libssm : opentimeSSM : shmget ");
        return 0;
    }
    if( (timecontrol = (struct ssmtime *)shmat(shm_timeid, 0, 0)) == (struct ssmtime *)-1 ){
        timecontrol = NULL;
        perror("libssm : opentimeSSM : shmat ");
        return 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief closetimeSSM
 */
/*--------------------------------------------------------------*/
void closetimeSSM( void )
{
    shmdt( timecontrol );
    timecontrol = NULL;
}



/*--------------------------------------------------------------*/
/**
 * @brief inittimeSSM struct initialize
 */
/*--------------------------------------------------------------*/
void inittimeSSM( void )
{
    if(timecontrol != NULL){
        timecontrol->offset = 0.0;
        timecontrol->speed = 1.0;
        timecontrol->is_pause = 0;
    }
}



/*=========================coordinator==========================*/

/*--------------------------------------------------------------*/
/**
 * @brief  createtimeSSM using coordinator
 * @return
 */
/*--------------------------------------------------------------*/
int createtimeSSM( void )
{
    return opentimeSSM(  );
}



/*--------------------------------------------------------------*/
/**
 * @brief  destroytimeSSM
 * @return 1
 */
/*--------------------------------------------------------------*/
int destroytimeSSM( void )
{
    closetimeSSM(  );
    if(shm_timeid >= 0 ) shmctl( shm_timeid, IPC_RMID, 0 );

    return 1;
}
