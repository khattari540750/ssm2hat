/****************************************************************/
/**
  @file   libssm2hat_base.c
  @brief  ssm2hat library time
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <time.h>
#include "ssm2hat_lib.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif



int shm_id = -1;			//! Shared memory ID
char *shm_ptr;				//! Shared memory top address
int msq_id = -1;			//! Message queue ID
pid_t my_pid;				//! Own process ID
char err_msg[20];           //! Error message



/*--------------------------------------------------------------*/
/**
 * @brief calcSSM_table
 * @param life
 * @param cycle
 * @return
 */
/*--------------------------------------------------------------*/
int calcSSM_table( ssmTimeT life, ssmTimeT cycle )
{
    return (( double )SSM_BUFFER_MARGIN * ( life / cycle ) ) + SSM_MARGIN;
}



/*--------------------------------------------------------------*/
/**
 * @brief calcSSM_life savetime calcu from table size
 * @param table_num
 * @param cycle
 * @return
 */
/*--------------------------------------------------------------*/
ssmTimeT calcSSM_life( int table_num, ssmTimeT cycle )
{
    return (ssmTimeT)( table_num - SSM_MARGIN ) * cycle / (ssmTimeT)SSM_BUFFER_MARGIN;
}



/*=====================Time ID functions========================*/

/*--------------------------------------------------------------*/
/**
 * @brief  getTID_top
 * @param  sid
 * @return newest TimeID
 */
/*--------------------------------------------------------------*/
SSM_tid getTID_top( SSM_sid sid )
{
    if( sid == 0 )
        return SSM_ERROR_NO_DATA;
    return shm_get_tid_top( shm_get_address( sid ) );
}



/*--------------------------------------------------------------*/
/**
 * @brief  getTID_bottom
 * @param  sid
 * @return oldest Time ID
 */
/*--------------------------------------------------------------*/
SSM_tid getTID_bottom( SSM_sid sid )
{
    if( sid == 0 )
        return SSM_ERROR_NO_DATA;
    return shm_get_tid_bottom( shm_get_address( sid ) );
}



/*--------------------------------------------------------------*/
/**
 * @brief waitTID newest tid is lerger than desighnate tid
 * @param sid
 * @param tid
 * @return 1:succeed 0:false -1:error
 */
/*--------------------------------------------------------------*/
int waitTID( SSM_sid sid, SSM_tid tid )
{
    if( sid == 0 )
        return -1;
    return shm_cond_wait( shm_get_address( sid ), tid );
}



/*--------------------------------------------------------------*/
/**
 * @brief getTID
 * @param sid
 * @param ytime
 * @return most near Time ID before desighnet time
 */
/*--------------------------------------------------------------*/
SSM_tid getTID( SSM_sid sid, ssmTimeT ytime )
{
    ssm_header *shm_p;
    SSM_tid r_tid;
    int top, bottom;

    if( sid == 0 ) { return SSM_ERROR_NO_DATA; }

    //! get shared memory address of assigned sensor
    shm_p = shm_get_address( sid );

    top = shm_get_tid_top( shm_p );
    bottom = shm_get_tid_bottom( shm_p );

    //! time check
    if( ytime > shm_get_time( shm_p, top ) ) {
        return top;//!SSM_ERROR_FUTURE; -1
    }
    if( ytime < shm_get_time( shm_p, bottom) ){
        return SSM_ERROR_PAST; //! -2
    }

    //! search tid
    for( r_tid = top; r_tid >= bottom; r_tid-- ){
        if( shm_get_time( shm_p, r_tid ) < ytime ){ break; }
    }
    return r_tid;
}



/*--------------------------------------------------------------*/
/**
 * @brief getTID_time search TimeID using stream cycle.
 * @param shm_p
 * @param ytime
 * @return TID the nearest to time.
 */
/*--------------------------------------------------------------*/
SSM_tid getTID_time( ssm_header *shm_p, ssmTimeT ytime )
{
    //!int table_pos;
    SSM_tid tid;
    //!SSM_tid *tid_p;
    SSM_tid top, bottom;
    ssmTimeT top_time;
    //!long long table_pos_long;

    top = shm_get_tid_top( shm_p );
    bottom = shm_get_tid_bottom( shm_p );

    //! time check
    if( ytime > shm_get_time( shm_p, top ) )
        return top;//!SSM_ERROR_FUTURE; // -1
    if( ytime < shm_get_time( shm_p, bottom) )
        return SSM_ERROR_PAST; // -2

    //! time quantize
    //! センサ周期を使用して目標時刻の位置を予測してジャンプ */
    top_time = shm_get_time( shm_p, top );
    tid = top + ( SSM_tid )( ( ytime - top_time ) /  ( ssmTimeT )shm_p->cycle * gettimeSSM_speed(  ) );
    //! 予測がはずれていたときのための保険
    if( tid > top )
        tid = top;
    else if( tid < bottom )
        tid = bottom;

    //! tid search
    while( shm_get_time( shm_p, tid ) < ytime ) { tid++; }
    while( shm_get_time( shm_p, tid ) > ytime ) { tid--; }

    return tid;
}



/*=====================message functions========================*/

/*--------------------------------------------------------------*/
/**
 * @brief del_msg
 */
/*--------------------------------------------------------------*/
void del_msg( void )
{
    int len;
    ssm_msg msg;
    while( ( len = msgrcv( msq_id, &msg, SSM_MSG_SIZE, my_pid, IPC_NOWAIT ) ) > 0 );
}



/*--------------------------------------------------------------*/
/**
 * @brief send_msg
 * @param cmd_type command type
 * @param msg message
 * @return 1:suceed 0:false
 */
/*--------------------------------------------------------------*/
int send_msg( int cmd_type, ssm_msg *msg )
{
    ssm_msg msgbuf;
    //! initialize check
    if( msq_id < 0 ){
        strcpy( err_msg, "msq id err" );
        return 0;
    }

    if( msg == NULL ) { msg = &msgbuf; }

    msg->msg_type = MSQ_CMD;
    msg->res_type = my_pid;					/* return pid */
    msg->cmd_type = cmd_type;

    //! send
    if( msgsnd( msq_id, ( void * )( msg ), ( size_t ) SSM_MSG_SIZE, 0 ) < 0 ){
        perror( "msgsnd" );
        sprintf( err_msg, "msq send err" );
        return 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief receive_msg from coordinator reciveve
 * @param msg message
 * @return 1:suceed 0:false
 */
/*--------------------------------------------------------------*/
int receive_msg( ssm_msg *msg )
{
#ifdef __APPLE__
    volatile ssize_t len;
#else
    ssize_t len;
#endif
    //! initialize check
    if( msq_id < 0 ){
        strcpy( err_msg, "msq id err" );
        return 0;
    }

    //! receive
    len = msgrcv( msq_id, msg, SSM_MSG_SIZE, my_pid, 0 );
    if( len <= 0 ){
        strcpy( err_msg, "receive data err" );
        return 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief  communicate_msg communicate coordinator
 * @param  cmd_type
 * @param  msg
 * @return 1:suceed 0:false
 */
/*--------------------------------------------------------------*/
int communicate_msg( int cmd_type, ssm_msg *msg )
{
    //! send
    send_msg( cmd_type, msg );
    //! ...processing on ssm side

    //! return (msgrcv is waiting mode)
    if( !receive_msg( msg ) ) { return 0; }

    return 1;
}



/*=======================API functions==========================*/

/*--------------------------------------------------------------*/
/**
 * @brief errSSM print err message
 */
/*--------------------------------------------------------------*/
void errSSM( void )
{
    fprintf( stderr, "SSM Err: %s\n", err_msg );
}



/*--------------------------------------------------------------*/
/**
 * @brief initSSM initialize shared memory and message queue
 * @return 1:suceed 0:false
 */
/*--------------------------------------------------------------*/
int initSSM( void )
{
    //! Open message queue
    if( ( msq_id = msgget( ( key_t ) MSQ_KEY, 0666 ) ) < 0 ){
        sprintf( err_msg, "msq open err" );
        return 0;
    }
    my_pid = getpid(  );

    //! connect internak time
    if( !opentimeSSM() ){
        sprintf( err_msg, "time open err" );
        return 0;
    }

    //! sign up SSM
    if( !send_msg( MC_INITIALIZE, NULL ) ) { return 0; }

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief  endSSM
 * @return 1:suceed 0:false
 */
/*--------------------------------------------------------------*/
int endSSM( void )
{
    int ret = 1;
    //! sign up SSM
    if( !send_msg( MC_TERMINATE, NULL ) ) { ret = 0; }

    //! release time memory
    closetimeSSM(  );

    return ret;
}



/*--------------------------------------------------------------*/
/**
 * @brief  createSSM Allocate sensor data space with time table on SSM
 * @param  name
 * @param  stream_id
 * @param  ssm_size
 * @param  life
 * @param  cycle
 * @return
 */
/*--------------------------------------------------------------*/
SSM_sid createSSM( const char *name, int stream_id, size_t ssm_size, ssmTimeT life, ssmTimeT cycle )
{
    ssm_msg msg;
    int open_mode = SSM_READ | SSM_WRITE;
    ssm_header *shm_p;
    size_t len;

    //! initialize check
    if( !name ){
        fprintf( stderr, "SSM ERROR : create : stream name is NOT defined, err.\n" );
        strcpy( err_msg, "name undefined" );
        return 0;
    }

    len = strlen( name );
    if( len == 0 || len >= SSM_SNAME_MAX ){
        fprintf( stderr, "SSM ERROR : create : stream name length of '%s' err.\n", name );
        strcpy( err_msg, "name length" );
        return 0;
    }

    if( stream_id < 0 ){
        fprintf( stderr, "SSM ERROR : create : stream id err.\n" );
        return 0;
    }

    if( life <= 0.0 ){
        fprintf( stderr, "SSM ERROR : create : stream life time err.\n" );
    }

    if( cycle <= 0.0 ){
        fprintf( stderr, "SSM ERROR : create : stream cycle err.\n" );
        strcpy( err_msg, "arg error : cycle" );
        return 0;
    }

    if( life < cycle ){
        fprintf( stderr, "SSM ERROR : create : stream saveTime MUST be larger than stream cycle.\n" );
        strcpy( err_msg, "arg err : life, c" );
        return 0;
    }

    //! prepare message packet
    strncpy( msg.name, name, SSM_SNAME_MAX );
    msg.suid = stream_id;
    msg.ssize = ssm_size;
    msg.hsize = calcSSM_table( life, cycle ) ;	//! table size
    msg.time = cycle;

    //! communicate
    if( !communicate_msg( MC_CREATE | open_mode, &msg ) ) { return 0; }

    //! return value: suid=sensors shm_id, ssize=offset
    if( !msg.suid ){
        strcpy( err_msg, "shm address err" );
        return 0;
    }

    //! Attach to shared memory
    if( ( shm_p = ( ssm_header * )shmat( msg.suid, 0, 0 ) ) == ( ssm_header * )-1 ){
        shm_p = 0;
        strcpy( err_msg, "shmat err" );
        return 0;
    }

    //sprintf( err_msg, "%d %p", msg.suid, shm_p );
    shm_p = shm_p + msg.ssize;				//! offset

    shm_init_time( shm_p );
    return ( SSM_sid ) shm_p;
}



/*--------------------------------------------------------------*/
/**
 * @brief releaseSSM release stream
 * @param sid
 * @return
 */
/*--------------------------------------------------------------*/
int releaseSSM( SSM_sid *sid )
{
    if( *sid ) {
        //shmdt( *sid );
        *sid = 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief openSSM open sensor data on ssm
 * @param name
 * @param stream_id
 * @param open_mode
 * @return
 */
/*--------------------------------------------------------------*/
SSM_sid openSSM( const char *name, int stream_id, char open_mode )
{
    ssm_msg msg;
    int len;
    ssm_header *shm_p;

    //! initialize check
    len = strlen( name );
    if( len == 0 || len >= SSM_SNAME_MAX ){
        fprintf( stderr, "SSM ERROR : open : stream name length of '%s' err.\n", name );
        strcpy( err_msg, "name length" );
        return 0;
    }

    if( stream_id < 0 ){
        fprintf( stderr, "SSM ERROR : open : stream id err.\n" );
        return 0;
    }

    //! prepare message packet
    strncpy( msg.name, name, SSM_SNAME_MAX );
    msg.suid = stream_id;
    msg.ssize = 0;
    msg.hsize = 0;
    msg.time = 0;

    //! communicate
    if( !communicate_msg( MC_OPEN | ( int )open_mode, &msg ) ) { return 0; }

    //! attach to shared memory
    if( msg.suid < 0 ) { return 0; }

    if( ( shm_p = ( ssm_header * )shmat( msg.suid, 0, 0 ) ) == ( ssm_header * )-1 ){
        strcpy( err_msg, "shmat err" );
        shm_p = 0;
        return 0;
    }

    shm_p += msg.ssize;				//! offset
    return ( SSM_sid ) shm_p;		//! sensor sid
}



/*--------------------------------------------------------------*/
/**
 * @brief closeSSM close stream
 * @param sid
 * @return
 */
/*--------------------------------------------------------------*/
int closeSSM( SSM_sid *sid )
{
    if( *sid ){
        //shmdt( *sid );
        *sid = 0;
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief write_data copy data to shared mimory
 * @param ssmp
 * @param data
 * @param user_data
 */
/*--------------------------------------------------------------*/
static void write_data( void *ssmp, const void *data, void *user_data )
{
    size_t size = *( size_t *)user_data;
    memcpy( ssmp, data, size );
}



/*--------------------------------------------------------------*/
/**
 * @brief read_data copy data to shared memory
 * @param ssmp
 * @param data
 * @param user_data
 */
/*--------------------------------------------------------------*/
static void read_data( const void *ssmp, void *data, void *user_data )
{
    size_t size = *( size_t *)user_data;
    memcpy( data, ssmp, size );
}



/*--------------------------------------------------------------*/
/**
 * @brief readSSMP
 * @param sid
 * @param adata
 * @param ytime
 * @param tid
 * @param user_data
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid readSSMP( SSM_sid sid, void *adata, ssmTimeT * ytime, SSM_tid tid,
    void (*callback)( const void *ssmp, void *data, void *user_data ), void *user_data )
{
    ssm_header *shm_p;
    SSM_tid top, bottom;
    char *data = ( char * )adata;

    if( sid == 0 ){ return SSM_ERROR_NO_DATA; }

    shm_p = shm_get_address( sid );
    top = shm_get_tid_top( shm_p );
    bottom = shm_get_tid_bottom( shm_p );

    //! if tid is negative then the function returns latest data */
    if( tid < 0 ){ tid = top; }

    //! region check
    if( tid < SSM_TID_SP ){ return SSM_ERROR_NO_DATA; }
    if( tid > top ) { return SSM_ERROR_FUTURE; }
    if( tid < bottom ){ return SSM_ERROR_PAST; }
    //memcpy( data, , shm_p->size );
    callback( shm_get_data_ptr( shm_p, tid ), data, user_data );

    //! get time stamp
    if( ytime ){
        // *ytime = ( ( ssmTimeT * ) ( ( char * )shm_p + shm_p->times_off ) )[r_tid];
        *ytime = shm_get_time( shm_p , tid );
    }
    // printf("%d ",r_tid);
    return tid;
}



/*--------------------------------------------------------------*/
/**
 * @brief readSSM
 * @param sid
 * @param adata
 * @param ytime
 * @param tid
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid readSSM( SSM_sid sid, void *adata, ssmTimeT *ytime, SSM_tid tid )
{
    return readSSMP( sid, adata, ytime, tid, read_data, &shm_get_address( sid )->size );
}



/*--------------------------------------------------------------*/
/**
 * @brief readSSMP_time
 * @param sid
 * @param adata
 * @param ytime
 * @param ret_time
 * @param user_data
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid readSSMP_time( SSM_sid sid, void *adata, ssmTimeT ytime, ssmTimeT *ret_time ,
    void (*callback)( const void *ssmp, void *data, void *user_data ), void *user_data )
{
    SSM_tid tid;

    if( sid == 0 ) { return SSM_ERROR_NO_DATA; }

    //! search timeID in table
    if( ytime <= 0 ){
        tid = -1;
    }
    else{
        tid = getTID_time( shm_get_address( sid ), ytime );
        // sprintf(err_msg,"tid %d",tid);
        if( tid < 0 ) { return tid; }
    }
    return readSSMP( sid, adata, ret_time, tid, callback, &shm_get_address( sid )->size );
}



/*--------------------------------------------------------------*/
/**
 * @brief readSSM_time
 * @param sid
 * @param adata
 * @param ytime
 * @param ret_time
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid readSSM_time( SSM_sid sid, void *adata, ssmTimeT ytime, ssmTimeT *ret_time )
{
    return readSSMP_time( sid, adata, ytime, ret_time, read_data, &shm_get_address( sid )->size );
}



/*--------------------------------------------------------------*/
/**
 * @brief writeSSMP
 * @param sid
 * @param adata
 * @param ytime
 * @param user_data
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid writeSSMP( SSM_sid sid, const void *adata, ssmTimeT ytime ,
    void (*callback)( void *ssmp, const void *data, void *user_data ), void *user_data )
{
    ssm_header *shm_p;
    SSM_tid tid;
    //int i;
    const char *data = ( const char * )adata;

    if( sid == 0 ) { return SSM_ERROR_NO_DATA; }

    shm_p = shm_get_address( sid );
    //i = 0;

    shm_lock( shm_p );
    {
        tid = shm_get_tid_top( shm_p ) + 1;

        //i = setTID_time( shm_p, ytime, tid );
        //memcpy( shm_get_data_ptr( shm_p, tid ), data, shm_p->size );
        callback( shm_get_data_ptr( shm_p, tid ), data, user_data );


        shm_set_time( shm_p, tid, ytime );

        //! update
        ( shm_p->tid_top )++;
    }
    shm_cond_broadcast( shm_p );
    shm_unlock( shm_p );

    return tid;
}



/*--------------------------------------------------------------*/
/**
 * @brief writeSSM
 * @param sid
 * @param adata
 * @param ytime
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid writeSSM( SSM_sid sid, const void *adata, ssmTimeT ytime )
{
    return writeSSMP( sid, adata, ytime, write_data, &shm_get_address( sid )->size );
}



/*--------------------------------------------------------------*/
/**
 * @brief writeSSM_time
 * @param sid
 * @param adata
 * @param ytime
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid writeSSM_time( SSM_sid sid, const void *adata, ssmTimeT ytime )
{
    return writeSSMP( sid, adata, ytime, write_data, &shm_get_address( sid )->size );
}



/*--------------------------------------------------------------*/
/**
 * @brief writeSSMP_time
 * @param sid
 * @param adata
 * @param ytime
 * @param user_data
 * @return
 */
/*--------------------------------------------------------------*/
SSM_tid writeSSMP_time( SSM_sid sid, const void *adata, ssmTimeT ytime ,
    void (*callback)( void *ssmp, const void *data, void *user_data ), void *user_data )
{
    return writeSSMP( sid, adata, ytime, callback, user_data );
}



/*--------------------------------------------------------------*/
/**
 * @brief set_propertySSM
 * @param name
 * @param sensor_uid
 * @param adata
 * @param size
 * @return
 */
/*--------------------------------------------------------------*/
int set_propertySSM( const char *name, int sensor_uid, const void *adata, size_t size )
{
    ssm_msg msg;
    char *ptr;
    const char *data = ( char * )adata;

    if( strlen( name ) > SSM_SNAME_MAX ){
        strcpy( err_msg, "name length" );
        return 0;
    }

    //! set message
    strncpy( msg.name, name, SSM_SNAME_MAX );
    msg.suid = sensor_uid;
    msg.ssize = size;
    msg.hsize = 0;
    msg.time = 0;

    //! communicate
    if( !communicate_msg( MC_STREAM_PROPERTY_SET, &msg ) ) { return 0; }

    //! error
    if( !msg.ssize ) { return 0; }

    ptr = malloc( size + sizeof ( long ) );
    memcpy( ptr + sizeof ( long ), data, size );
    *( ( long * )ptr ) = msg.res_type;

    //! send
    msgsnd( msq_id, ptr, size, 0 );
    free( ptr );
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief get_propertySSM
 * @param name
 * @param sensor_uid
 * @param adata
 * @return
 */
/*--------------------------------------------------------------*/
int get_propertySSM( const char *name, int sensor_uid, void *adata )
{
    ssm_msg msg;
    int size;
    char *ptr;
    char *data = ( char * )adata;

    if( strlen( name ) > SSM_SNAME_MAX ){
        strcpy( err_msg, "name length" );
        return 0;
    }

    //! message set
    strncpy( msg.name, name, SSM_SNAME_MAX );
    msg.suid = sensor_uid;
    msg.ssize = 0;
    msg.hsize = 0;
    msg.time = 0;

    //! communicate
    if( !communicate_msg( MC_STREAM_PROPERTY_GET, &msg ) )
        return 0;

    //! error
    if( !msg.ssize ) { return 0; }
    size = msg.ssize;

    ptr = malloc( size + sizeof ( long ) );

    //! recive
    msgrcv( msq_id, ptr, size, my_pid, 0 );

    memcpy( data, ptr + sizeof ( long ), size );
    free( ptr );
    return 1;
}



/*==============================================================*/

/*--------------------------------------------------------------*/
/**
 * @brief damp
 * @param sid
 * @param mode
 * @param num
 * @return
 */
/*--------------------------------------------------------------*/
int damp( SSM_sid sid, int mode, int num )
{
#if 0
    //int i;
    //SSM_tid *p;
    ssm_header *shm_p;

    //! shared memory get
    shm_p = shm_get_address( sid );

    switch ( mode )
    {
    case 0:

        break;
    case 1:

        break;
    case 2:			//! time table damp
#if 0
        p = shm_get_timetable_address( shm_p );
        for ( i = 0; i < num; i++ ){
            printf( "table[%d] = %d \n ", i, (int)p[i] );
        }
#endif
        break;
    default:
        break;
    }
    return num;
#else
    return 0;
#endif
}



/*--------------------------------------------------------------*/
/**
 * @brief  getSSM_num
 * @return error: minus value
 */
/*--------------------------------------------------------------*/
int getSSM_num( void )
{
    ssm_msg msg;

    //! communicate
    if( !communicate_msg( MC_STREAM_LIST_NUM, &msg ) ) { return -1; }

    return msg.suid;
}



/*--------------------------------------------------------------*/
/**
 * @brief getSSM_name
 * @param n
 * @param name
 * @param suid
 * @param size
 * @return
 */
/*--------------------------------------------------------------*/
int getSSM_name( int n, char *name, int *suid, size_t *size )
{
    ssm_msg msg;

    msg.suid = n;
    //! communicate
    if( !communicate_msg( MC_STREAM_LIST_NAME, &msg ) ) { return -1; }

    if( suid ) { *suid = msg.suid; }                         //! ID
    if( size ) { *size = msg.ssize; }	                     //! data size
    if( name ) { strncpy( name, msg.name, SSM_SNAME_MAX ); } //! name

    return msg.ssize;
}



/*--------------------------------------------------------------*/
/**
 * @brief getSSM_info
 * @param name
 * @param suid
 * @param size
 * @param num
 * @param cycle
 * @param property_size
 * @return
 */
/*--------------------------------------------------------------*/
int getSSM_info( const char *name, int suid, size_t *size, int *num, double *cycle, size_t *property_size )
{
    ssm_msg msg;

    if( strlen( name ) > SSM_SNAME_MAX ){
        strcpy( err_msg, "name length" );
        return 0;
    }

    //! set message
    msg.suid = suid;
    strncpy( msg.name, name, SSM_SNAME_MAX );
    //! communicate
    if( !communicate_msg( MC_STREAM_LIST_INFO, &msg ) ) { return -1; }

    if( size ) { *size = msg.ssize; }		/* data size */
    if( num ) { *num = msg.hsize; }			/* history number */
    if( cycle ) *cycle = msg.time;			/* cycle */
    if( property_size ) { *property_size = msg.suid; }

    return msg.ssize;
}



/*--------------------------------------------------------------*/
/**
 * @brief getSSM_node_num
 * @return
 */
/*--------------------------------------------------------------*/
int getSSM_node_num( void )
{
    ssm_msg msg;

    //! communicate
    if( !communicate_msg( MC_NODE_LIST_NUM, &msg ) ) { return -1; }

    return msg.suid;
}



/*--------------------------------------------------------------*/
/**
 * @brief getSSM_node_info
 * @param n
 * @param node_num
 * @return
 */
/*--------------------------------------------------------------*/
int getSSM_node_info( int n, int *node_num )
{
    ssm_msg msg;

    msg.suid = n;
    if( !communicate_msg( MC_NODE_LIST_INFO, &msg ) ) { return 0; }
    if( msg.suid < 0 ) { return 0;}

    *node_num = msg.suid;

    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief getSSM_edge_num get node connect edge number
 * @return
 */
/*--------------------------------------------------------------*/
int getSSM_edge_num( void )
{
    ssm_msg msg;

    //! communicate
    if( !communicate_msg( MC_EDGE_LIST_NUM, &msg ) ) { return -1; }

    return msg.suid;
}



/*--------------------------------------------------------------*/
/**
 * @brief getSSM_edge_info get n edge info
 * @param n
 * @param name
 * @param name_size
 * @param id
 * @param node1
 * @param node2
 * @param dir
 * @return
 */
/*--------------------------------------------------------------*/
int getSSM_edge_info( int n, char *name, size_t name_size, int *id, int *node1, int *node2, int *dir )
{
    ssize_t len;
    ssm_msg msg;
    ssm_msg_edge edge;

    msg.suid = n;
    if( !send_msg( MC_EDGE_LIST_INFO, &msg ) ) { return 0; }

    //! receive
    len = msgrcv( msq_id, &edge, sizeof(ssm_msg_edge) - sizeof(long), my_pid, 0 );
    if( len <= 0 ){
        strcpy( err_msg, "receive data err" );
        return 0;
    }

    if( !strlen( edge.name ) ) { return 0; }

    strncpy( name, edge.name, ( SSM_MSG_SIZE < name_size ? SSM_MSG_SIZE : name_size ) );
    *id = edge.suid;
    *node1 = edge.node1;
    *node2 = edge.node2;
    *dir = edge.cmd_type & SSM_MODE_MASK;

    return 1;
}
