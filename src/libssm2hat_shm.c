/****************************************************************/
/**
  @file   libssm2hat_shm.c
  @brief  ssm2hat library shm
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

#include "libssm.h"



/*--------------------------------------------------------------*/
/**
  @fn
  @brief allock shm memory
  @param key_t shmget key
  @param data_size ssm stream data size
  @param history_num
  @param cycle
  @return false: failed, suceed: shm id
 */
/*--------------------------------------------------------------*/
int shm_create_ssm( key_t key, int data_size, int history_num, double cycle )
{
    int shm_id;

    //! get shared memory,  size = ssm header + data + timestamp
    shm_id = shmget( key, sizeof( ssm_header ) + ( data_size + sizeof ( ssmTimeT ) ) * history_num, IPC_CREAT | 0666 );

    if( shm_id < 0 ){
        return -1;
    }
    return shm_id;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief  open shm_memory
  @param  shm_id
  @return shared memory header name
 */
/*--------------------------------------------------------------*/
ssm_header *shm_open_ssm( int shm_id )
{
    ssm_header *header;

    //! attach memory
    header = shmat( shm_id, 0, 0 );

    if( header == ( ssm_header * )-1 ){
        return 0;
    }
    return header;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief  init shm header
  @param  *header
  @param  data_size
  @param  history_num
  @param  cycle
 */
/*--------------------------------------------------------------*/
void shm_init_header( ssm_header *header, int data_size, int history_num, ssmTimeT cycle )
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    header->tid_top = SSM_TID_SP - 1;	//! initial position
    header->size = data_size;	//! data size
    header->num = history_num;	//! history number
    //header->table_size = hsize;	//! table size
    header->cycle = cycle;	//! data mimimum size
    header->data_off = sizeof( ssm_header );	//! data beginning address
    header->times_off = header->data_off + ( data_size * history_num );	//! time beginning address
    //header->table_off = header->times_off + sizeof( ssmTimeT ) * hsize ;	//! time table's beginng address

    //! initialize synchronize mutex
    pthread_mutexattr_init(&mattr);
    pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&header->mutex, &mattr);

    pthread_condattr_init(&cattr);
    pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&header->cond, &cattr);

    pthread_mutexattr_destroy( &mattr );
    pthread_condattr_destroy( &cattr );
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief  destroy shared memory headder
  @param  *header
 */
/*--------------------------------------------------------------*/
void shm_dest_header( ssm_header *header )
{
    pthread_mutex_destroy( &header->mutex );
    pthread_cond_destroy( &header->cond );
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief  get shared memory sid address
  @param  sid
  @return sid
 */
/*--------------------------------------------------------------*/
ssm_header *shm_get_address( SSM_sid sid )
{
    return ( ssm_header * )sid;
}



/*--------------------------------------------------------------*/
/**
  @fn
  @brief  get shared memory data address
  @param  *shm_p
 */
/*--------------------------------------------------------------*/
void *shm_get_data_address( ssm_header *shm_p )
{
    return ( void * )( (char *)shm_p + shm_p->data_off );
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief shm_get_data_ptr
 * @param shm_p
 * @param tid
 */
/*--------------------------------------------------------------*/
void *shm_get_data_ptr( ssm_header *shm_p, SSM_tid tid )
{
    return ( void * )( (char *)shm_p + shm_p->data_off + (shm_p->size * ( tid % shm_p->num )));
}



/*--------------------------------------------------------------*/
/**
 * @brief  shm_get_data_size
 * @param  shm_p
 * @return shared memory size
 */
/*--------------------------------------------------------------*/
size_t shm_get_data_size( ssm_header *shm_p )
{
    return shm_p->size;
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  shm_get_time_address
 * @param  shm_p
 * @return ssm time
 */
/*--------------------------------------------------------------*/
ssmTimeT *shm_get_time_address( ssm_header *shm_p )
{
    return ( ssmTimeT * ) ( (char *)shm_p + shm_p->times_off );
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief shm_init_time
 * @param shm_p
 */
/*--------------------------------------------------------------*/
void shm_init_time( ssm_header *shm_p )
{
    int i;
    ssmTimeT *time = shm_get_time_address( shm_p );
    for(i = 0; i < shm_p->num; i++)
        time[i] = 0;
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  shm_get_time
 * @param  shm_p
 * @param  tid
 * @return ssmtime
 */
/*--------------------------------------------------------------*/
ssmTimeT shm_get_time( ssm_header *shm_p, SSM_tid tid )
{
    return shm_get_time_address( shm_p )[tid % shm_p->num];
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief shm_set_time
 * @param shm_p
 * @param tid
 * @param time
 */
/*--------------------------------------------------------------*/
void shm_set_time( ssm_header *shm_p, SSM_tid tid, ssmTimeT time )
{
    shm_get_time_address( shm_p )[tid % shm_p->num] = time;
}



#if 0
/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  shm_get_timetable_address
 * @param  shm_p
 * @return ssm tid
 */
/*--------------------------------------------------------------*/
SSM_tid *shm_get_timetable_address( ssm_header *shm_p )
{
    return ( SSM_tid *)((char *)shm_p + shm_p->table_off);
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief shm_init_timetable
 * @param shm_p
 */
/*--------------------------------------------------------------*/
void shm_init_timetable( ssm_header *shm_p )
{
    int i;
    SSM_tid *timetable = shm_get_timetable_address( shm_p );
    for ( i = 0; i < shm_p->table_size; i++ )
        timetable[i] = 0;
    timetable[0] = -1000000;

}
#endif



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief shm_get_tid_top
 * @param shm_p
 * @return ssm tid
 */
/*--------------------------------------------------------------*/
SSM_tid shm_get_tid_top( ssm_header *shm_p )
{
    //! return tid
    return shm_p->tid_top;
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  shm_get_tid_bottom
 * @param  shm_p
 * @return ssm tid
 */
/*--------------------------------------------------------------*/
SSM_tid shm_get_tid_bottom( ssm_header *shm_p )
{
    int tid;
    if( shm_p->tid_top < 0 )
        return shm_p->tid_top;

    //! return tid
    tid = shm_p->tid_top - shm_p->num + SSM_MARGIN + 1;

    if( tid < 0 )
        return 0;

    return tid;
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  lock shm
 * @param
 * @return 1:success, 0: failed
 */
/*--------------------------------------------------------------*/
int shm_lock( ssm_header *shm_p )
{
    if( pthread_mutex_lock( &shm_p->mutex ) )
        return 0;
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  unlock shm
 * @param  *shm_p
 * @return 1:success, 0: failed
 */
/*--------------------------------------------------------------*/
int shm_unlock( ssm_header *shm_p )
{
    if( pthread_mutex_unlock( &shm_p->mutex ) )
        return 0;
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  wait wrote TID is tid
 * @retval 1:suceed  0:false  -1:error
 */
/*--------------------------------------------------------------*/
int shm_cond_wait( ssm_header *shm_p, SSM_tid tid )
{
    int ret = 0;
    struct timeval now;
    struct timespec tout;

    if( tid <= shm_get_tid_top( shm_p ) )
        return 1;
    gettimeofday( &now, NULL );
    tout.tv_sec = now.tv_sec + 1;
    tout.tv_nsec = now.tv_usec * 1000;


    if( !shm_lock( shm_p ) )
        return -1;

    while( tid > shm_get_tid_top( shm_p ) && (ret == 0) ){
        ret = pthread_cond_timedwait( &shm_p->cond, &shm_p->mutex, &tout );
    }

    if( !shm_unlock( shm_p ) )
        return -1;
    return ( ret == 0 );
}



/*--------------------------------------------------------------*/
/**
 * @fn
 * @brief  shm_cond_broadcast
 * @param  shm_p
 * @return 1:succed
 */
/*--------------------------------------------------------------*/
int shm_cond_broadcast( ssm_header *shm_p )
{
    pthread_cond_broadcast( &shm_p->cond );
    return 1;
}
