/****************************************************************/
/**
  @file   ssm_coodinator.c
  @brief  shared memory program
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
#include <signal.h>
#include <sys/msg.h>
#include <getopt.h>
#include <errno.h>

#include <list>

#include "ssm2hat.h"
#include "ssm2hat_lib.h"
//#include "ssm2hat_time.h"
//#include "ssm_coordinator.h"


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

using namespace std;


/** typedefs  */
typedef struct ssm_list *SSM_ListPtr;

typedef struct ssm_list
{
    int node;									/**< ストリームを作成したノード */
    char name[SSM_SNAME_MAX];					/**< ストリームの名前 */
    int suid;									/**< ストリーム固有のID */
    int shm_id;									/**< ブロックのある共有メモリのID */
    char *shm_ptr;								/**< ブロックのある共有メモリのアドレス(Manager内のみ有効) */
    int shm_offset;								/**< 共有メモリ内でブロックのある場所 */
    int size;									/**< ブロックのサイズ */
    ssm_header *header;							/**< ブロックのヘッダ */
    SSM_ListPtr next;							/**< 次のリストへ */
    char *property;								/**< プロパティデータへのポインタ */
    int property_size;							/**< プロパティデータの大きさ */
} SSM_List;


class Node
{
public:
    Node( int node ){
        this->node = node;
    }
    int node;
};

class Edge
{
public:
    Edge( const char *name, int id, int node1, int node2, int dir ){
        strncpy( this->name, name, SSM_SNAME_MAX );
        this->id = id;
        this->node1 = node1;
        this->node2 = node2;
        this->dir = dir;
    }
    int node1,node2; // node1 -> node2
    int dir;
    char name[SSM_SNAME_MAX];
    int id;
};

typedef list<Node> nodeArrayT;
typedef list<Edge> edgeArrayT;

int msq_id = -1;								/* メッセージキューのID */
key_t shm_key_num = 0;							/* 共有メモリの数 */
SSM_List *ssm_top = 0;
pid_t my_pid;									/* 自分のプロセスID */
int verbosity_mode = 1;							/* メッセージ表示 */
int is_check_msgque = 1;						/* メッセージキューがすでに存在しているかを確認しない */

void escape_road( void );
static void emergency( int );

nodeArrayT node;								/** ノード（プロセス） */
edgeArrayT edge;								/** エッジ保存用 */
//----------------------------------------------
int emergencyCnt = 0;



/** function prototypes */
int alloc_ssm_block( int ssize, int hsize, ssmTimeT cycle, char **shm_h, int *offset );
SSM_List *add_ssm_list( char *name, int suid, int ssize, int hsize, ssmTimeT cycle );
void free_ssm_list( SSM_List * ssmp );
int ssm_init( void );
SSM_List *serch_SSM_List( char *name, int suid );
int msq_loop( void );
SSM_List *get_nth_SSM_List( int n );
int get_num_SSM_List( void );
long get_receive_id( void );



/*--------------------------------------------------------------*/
/**
 * @brief emergency
 * @param sig
 */
/*--------------------------------------------------------------*/
static void emergency( int sig )
{
    if( emergencyCnt < 1 ){
        emergencyCnt++;
        if( node.size( ) > 0 ){
            fprintf( stderr, "maybe other process using ssm is alive.\n");
            fprintf( stderr, "or do you forget to call 'endSSM()' at the end of programs ?\n");
            fprintf( stderr, "if you want to shutoff 'ssm-coordinator', please hit Ctrl-C.");
            return;
        }
    }

    if( verbosity_mode >= 1 ){
        fprintf( stderr, "program stop [%d]\n", sig );
        fprintf( stderr, "finalize...\n" );
    }

    //! final
    free_ssm_list( ssm_top );
    ssm_top = 0;
    msgctl( msq_id, IPC_RMID, NULL );			/* メッセージキューの削除 */
    destroytimeSSM(  );							/* 時刻同期用バッファの削除 */
    if( verbosity_mode >= 1 ){
        fprintf( stderr, "- all allocated shared memory released\n" );
    }

    if( verbosity_mode >= 1 ){
        fprintf( stderr, "exit\n" );
    }
}



/*--------------------------------------------------------------*/
/**
 * @brief escape_road
 */
/*--------------------------------------------------------------*/
void escape_road( void )
{
#if 0
    struct sigaction sa_sigint;
    sa_sigint.sa_handler = emergency;
    sa_sigint.sa_flags = SA_RESETHAND | SA_NODEFER;
    sa_sigint.sa_restorer = 0;

    if( sigaction( SIGINT, &sa_sigint, NULL ) < 0 ){
        perror( "sigaction" );
        exit( 1 );
    }
#else
    signal( SIGINT, emergency );
#endif
}



/*--------------------------------------------------------------*/
/**
 * @brief print_list
 * @param p
 */
/*--------------------------------------------------------------*/
void print_list( SSM_List * p )
{
    while( p ){
        printf( "   |   :name : %s\n", p->name );
        printf( "   |   :ID: %d  offset: %d  size: %d  address: %ld \n   |\n", p->shm_id, p->shm_offset, p->size,
                ( long )p->shm_ptr );
        p = p->next;
    }
}



/*--------------------------------------------------------------*/
/**
 * @brief get_receive_id
 * @return
 */
/*--------------------------------------------------------------*/
long get_receive_id( void )
{
    static long id;
    if( id < MSQ_RES ) { id = MSQ_RES; }

    id++;

    if( id > MSQ_RES_MAX ) { id = MSQ_RES; }
    return id;
}



/*--------------------------------------------------------------*/
/**
 * @brief alloc_ssm_block
 * @param ssize
 * @param hsize
 * @param cycle
 * @param shm_h
 * @param offset
 * @return
 */
/*--------------------------------------------------------------*/
int alloc_ssm_block( int ssize, int hsize, ssmTimeT cycle, char **shm_h, int *offset )
{
    int s_id;

    //! get shared memory
    if( ( s_id =
          shmget( SHM_KEY + shm_key_num, sizeof ( ssm_header ) + ( ssize + sizeof ( ssmTimeT ) ) * hsize,
                  IPC_CREAT | 0666 ) ) < 0 ){
        if( verbosity_mode >= 0 ){
            fprintf( stderr, "ERROR  : cannot allock shared memory.\n" );
        }
        return 0;
    }
    shm_key_num++;

    //! atache
    if( ( *shm_h = static_cast< char* >( shmat( s_id, 0, 0 ) ) ) == ( void * )-1 ) { return 0; }
#if 0
    //! 0:init?
    for ( i = 0; i < ( int )( sizeof ( ssm_header ) + ( ssize + sizeof ( ssmTimeT ) ) * hsize ); i++ ){
        ( *shm_h )[i] = 0;
    }
#endif

    //! offset 0
    *offset = 0;
    if( cycle <= 0 ) { cycle = 1; }

    //! ssm_header initialize
    shm_init_header( ( ssm_header * ) * shm_h, ssize, hsize, cycle );

    return s_id;
}



/*--------------------------------------------------------------*/
/**
 * @brief add_ssm_list
 * @param node
 * @param name
 * @param suid
 * @param ssize
 * @param hsize
 * @param cycle
 * @return 0:false
 */
/*--------------------------------------------------------------*/
SSM_List *add_ssm_list( int node, char *name, int suid, int ssize, int hsize, ssmTimeT cycle )
{
    SSM_List *p, *q;

    p = ( SSM_List * ) malloc( sizeof ( SSM_List ) );
    if( !p ){
        if( verbosity_mode >= 0 ){
            fprintf( stderr, "ERROR  : cannot allock memory of local list\n" );
        }
        return 0;
    }
    p->shm_id = alloc_ssm_block( ssize, hsize, cycle, &( p->shm_ptr ), &( p->shm_offset ) );
    if( !p->shm_id ) { return 0; }

    p->node = node;
    strcpy( p->name, name );
    p->suid = suid;
    p->size = sizeof ( ssm_header ) + ( ssize + sizeof ( ssmTimeT ) ) * hsize;
    p->header = ( ssm_header * ) ( p->shm_ptr );
    p->next = 0;
    p->property = 0;
    p->property_size = 0;

    //! add list final p
    if( !ssm_top ) { ssm_top = p; }
    else{
        q = ssm_top;
        while( q->next ){ q = q->next; }
        q->next = p;
    }
    return p;
}



/*--------------------------------------------------------------*/
/**
 * @brief free_ssm_list
 * @param ssmp
 */
/*--------------------------------------------------------------*/
void free_ssm_list( SSM_List * ssmp )
{
    if( ssmp ){
        if( ssmp->next ) { free_ssm_list( ssmp->next ); }
        if( ssmp->shm_ptr ) {
            shm_dest_header( ssmp->header );	  //! release
            shmdt( ssmp->shm_ptr );				  //! detauch
            shmctl( ssmp->shm_id, IPC_RMID, 0 );  //! delete
            if( verbosity_mode >= 1 ){
                printf( "%s detached\n", ssmp->name );
            }
        }
#if 0
        ssmp->next = 0;
        if( ssmp->header ) { free( ssmp->header ); }
        if( ssmp->property ) { free( ssmp->property ); }
        free( ssmp );
#endif
    }
}



/*--------------------------------------------------------------*/
/**
 * @brief ssm_init
 * @return
 */
/*--------------------------------------------------------------*/
int ssm_init( void )
{
    if( is_check_msgque ){
        msq_id = msgget( MSQ_KEY, IPC_CREAT | IPC_EXCL | 0666 );
    }
    else{
        msq_id = msgget( MSQ_KEY, IPC_CREAT | 0666 );
    }

    //! message que open
    if( msq_id < 0 ){
        if( errno == EEXIST ){
            fprintf( stderr, "ERROR : SSM message queue is already exist.\n" );
            fprintf( stderr, "maybe ssm-coordinator has started.\n" );
            fprintf( stderr,
                     "if you didn't started ssm-coordinator, please try\n\t$ ssm-coordinator --without-check-msgque\n" );
        }
        return 0;
    }

    my_pid = getpid(  );
    ssm_top = 0;

    if( !opentimeSSM(  ) ) { return -1; }
    inittimeSSM(  );

    if( verbosity_mode >= 1 ){
        printf( "Message queue ready.\n" );
        printf( "Msg queue ID = %d \n", msq_id );
        // printf("PID = %d\n",my_pid);
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief search_SSM_List
 * @param name
 * @param suid
 * @return
 */
/*--------------------------------------------------------------*/
SSM_List *search_SSM_List( char *name, int suid )
{
    SSM_List *p, *pn, *pni, *pi;

    p = ssm_top;

    pn = 0;
    pni = 0;
    pi = 0;
    while( p ){
        if( strcmp( p->name, name ) == 0 ) {
            //! same name
            pn = p;
            if( p->suid == suid ){
                //! same name
                pni = p;
            }
        }
        if( p->suid == suid ){
            //! same suid
            pi = p;
            // break;
        }
        p = p->next;
    }

    if( pni ) { return pni; }		//! same name and ID
    // if(pn)return pn; // same name
    // if(pi)return pi; // same ID
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief get_num_SSM_List
 * @return
 */
/*--------------------------------------------------------------*/
int get_num_SSM_List( void )
{
    SSM_List *p;
    int num;

    p = ssm_top;
    num = 0;
    while( p ){
        num++;
        p = p->next;
    }

    return num;
}



/*--------------------------------------------------------------*/
/**
 * @brief get_nth_SSM_List
 * @param n
 * @return
 */
/*--------------------------------------------------------------*/
SSM_List *get_nth_SSM_List( int n )
{
    SSM_List *p;
    p = ssm_top;

    while( p ){
        n--;
        if( n < 0 ) { return p; }
        p = p->next;
    }

    p = 0;
    return p;
}



/*--------------------------------------------------------------*/
/**
 * @brief msq_loop
 * @return
 */
/*--------------------------------------------------------------*/
int msq_loop( void )
{
    ssm_msg msg;
    SSM_List *slist;
    int len, num;

    //! loop
    while( 1 ) {
        //! recieve (wait)
        if( verbosity_mode >= 2 ) { printf( "msg_ready\n" ); }
        len = msgrcv( msq_id, &msg, SSM_MSG_SIZE, MSQ_CMD, 0 );
        if( len < 0 ){
            if(errno == EINTR) { continue; }
            //perror( "ERROR : cannot get msg " );
            return 0;
        }

        if( verbosity_mode >= 2 ) { printf( "msg_get\n" ); }

        // answer command
        switch ( ( msg.cmd_type ) & 0x1f )
        {
        case MC_NULL:
            if( verbosity_mode >= 2 ) { printf( "message:null\n" ); }
            break;
        case MC_INITIALIZE:
            if( verbosity_mode >= 2 ){
                printf( "message:initialize\n" );
                printf( "   |   :process %ld attach\n", msg.res_type );
            }
            /* プロセスをSSMに登録 */
            node.push_back( Node( msg.res_type ) );
            break;
        case MC_TERMINATE:
            if( verbosity_mode >= 2 ){
                printf( "message:terminate\n" );
                printf( "   |   :process %ld detach\n", msg.res_type );
            }
            /* プロセスをSSMから削除 */
        {
            nodeArrayT::iterator it = node.begin();
            while( it != node.end(  ) ){
                if( it->node == msg.res_type ){
                    it = node.erase( it );
                }
                else { it++; }
            }

            edgeArrayT::iterator jt = edge.begin();
            while( jt != edge.end(  ) )
            {
                if( jt->node1 == msg.res_type || jt->node2 == msg.res_type ){
                    jt = edge.erase( jt );
                }
                else{ jt++; }
            }
        }
            break;

        case MC_VERSION_GET:
            if( verbosity_mode >= 2 ) { printf( "message:get version\n" ); }
            break;

        case MC_CREATE:						//! keep memory
            if( verbosity_mode >= 2 ){
                printf( "message:create!\n" );
                printf( "   |   :name=%s id=%d\n", msg.name, msg.suid );
            }

            //! search list
            slist = search_SSM_List( msg.name, msg.suid );

            /* 同じ物が無かったら */
            if( !slist ){
                if( verbosity_mode >= 2 ) { printf( "   |   :add\n" ); }
                /* リストに追加 */
                slist = add_ssm_list( msg.res_type, msg.name, msg.suid, msg.ssize, msg.hsize, msg.time );
                if( !slist && verbosity_mode >= 2 )
                    printf( "   |   :cannot alock\n" );
            }
            else{
                if( verbosity_mode >= 2 ) { printf( "   |   :exist\n" ); }
            }
            msg.msg_type = msg.res_type;		/* 返信 */
            msg.cmd_type = MC_RES;

            if( slist ){
                msg.suid = slist->shm_id;
                msg.ssize = slist->shm_offset;
            }
            else{
                msg.suid = 0;
                msg.ssize = 0;
            }
            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;

            if( verbosity_mode >= 2 )
                printf( "   |   :received shm_id:%d offset:%lu\n", msg.suid, msg.ssize );
            break;

        case MC_OPEN:							/* センサのオープン */
            if( verbosity_mode >= 2 ){
                printf( "message:open!\n" );
                printf( "   |   :name=%s id=%d\n", msg.name, msg.suid );
            }
            slist = search_SSM_List( msg.name, msg.suid );
            if( slist ){
                if( verbosity_mode >= 2 )
                    printf( "   |   :exist\n" );
                /* エッジ情報の登録 */
                edge.push_back( Edge( msg.name, msg.suid, slist->node, msg.res_type, msg.cmd_type & SSM_MODE_MASK ) );
                /* 返信データの作成 */
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.suid = slist->shm_id;
                msg.ssize = slist->shm_offset;
            }
            else
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :not found\n" );
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.suid = -1;
                msg.ssize = 0;
            }
            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            if( verbosity_mode >= 2 )
                printf( "   |   :received shm_id%d offset%lu\n", msg.suid, msg.ssize );
            break;

        case MC_CLOSE:
            if( verbosity_mode >= 2 )
                printf( "message:stream close\n" );
            break;

        case MC_GET_TID:
            if( verbosity_mode >= 2 )
                printf( "message:get tid\n" );
            break;

        case MC_STREAM_LIST_NUM:
            if( verbosity_mode >= 2 )
                printf( "message:get list num\n" );
            /* センサのリストに登録されている数を取得する */
            num = get_num_SSM_List(  );
            msg.msg_type = msg.res_type;		/* 返信 */
            msg.cmd_type = MC_RES;
            msg.suid = num;

            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            if( verbosity_mode >= 2 )
            {
                printf( "   |   :sensor num %d\n", msg.suid );
                print_list( ssm_top );
            }

            break;

        case MC_STREAM_LIST_NAME:
            if( verbosity_mode >= 2 )
                printf( "message:get list name\n" );
            /* センサリストからn番めのものの情報を取得する */
            num = msg.suid;
            slist = get_nth_SSM_List( num );
            if( slist )
            {
                if( verbosity_mode >= 2 )
                {
                    printf( "   |   :exist\n" );
                    printf( "   |   :received num%d = %s[%d]\n", num, slist->name, slist->suid );
                }
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.suid = slist->suid;
                /*ID*/ msg.ssize = slist->header->size;	/* データサイズ */
                // msg.hsize = slist->table_size; /*記憶数*/
                // msg.time = slist->cycle; /*平均周期*/
                strcpy( msg.name, slist->name );	/* 名前 */
            }
            else
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :not found\n" );
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.suid = -1;
                msg.ssize = -1;
            }
            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            break;

        case MC_STREAM_LIST_INFO:
            /* 名前で指定したセンサの情報を取得する */
            if( verbosity_mode >= 2 )
            {
                printf( "message:get_list_info\n" );
                printf( "   |   :name=%s id=%d\n", msg.name, msg.suid );
            }

            slist = search_SSM_List( msg.name, msg.suid );
            if( slist )
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :exist\n" );
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.ssize = slist->header->size;	/* データサイズ */
                msg.suid = slist->property_size;	/* property size */
                msg.time = slist->header->cycle;	/* 平均周期 */
                msg.hsize = slist->header->num;	/* 履歴数 */
            }
            else
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :not found\n" );
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.suid = -1;
                msg.ssize = -1;
            }
            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            if( verbosity_mode >= 2 )
                printf( "   |   :received \n" );
            break;

        case MC_STREAM_PROPERTY_SET:
            if( verbosity_mode >= 2 )
            {
                printf( "message:property_set\n" );
                printf( "   |   :name=%s id=%d\n", msg.name, msg.suid );
            }

            slist = search_SSM_List( msg.name, msg.suid );
            if( slist )
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :exist\n" );
                if( !slist->property )
                {								/* 場所がなければ作る */
                    slist->property = static_cast< char *>( malloc( msg.ssize + sizeof ( long ) ) );
                    slist->property_size = msg.ssize;
                }
                if( slist->property )
                {								/* 場所がとれてれば書き込む */
                    /* 受信準備OKの返信 */
                    msg.msg_type = msg.res_type;	/* 返信 */
                    msg.res_type = get_receive_id(  );	/* 宛先はこちら */
                    msg.cmd_type = MC_RES;
                    msg.ssize = slist->property_size;	/* データサイズ */
                    if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                        return 0;
                    /* データを読み込む */
                    msgrcv( msq_id, ( char * )slist->property, slist->property_size, msg.res_type, 0 );
                }
                else
                {
                    if( verbosity_mode >= 2 )
                        printf( "   |   :mem error\n" );
                    msg.msg_type = msg.res_type;	/* 返信 */
                    msg.cmd_type = MC_RES;
                    msg.ssize = 0;
                    if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                        return 0;
                }
            }
            else
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :not found\n" );
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.ssize = 0;
                if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                    return 0;
            }
            break;

        case MC_STREAM_PROPERTY_GET:
            if( verbosity_mode >= 2 )
            {
                printf( "message:propeerty_get\n" );
                printf( "   |   :name=%s id=%d\n", msg.name, msg.suid );
            }

            slist = search_SSM_List( msg.name, msg.suid );
            if( slist )
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :exist\n" );
                if( slist->property && slist->property_size )
                {								/* 場所がとれてれば書き込む */
                    /* 受信準備OKの返信 */
                    msg.msg_type = msg.res_type;	/* 返信 */
                    msg.cmd_type = MC_RES;
                    msg.ssize = slist->property_size;	/* データサイズ */
                    if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                        return 0;

                    /* データを送る */
                    *( ( long * )slist->property ) = msg.res_type;	/* 返信 */
                    msgsnd( msq_id, ( char * )slist->property, slist->property_size, 0 );
                }
                else
                {
                    if( verbosity_mode >= 2 )
                        printf( "   |   :mem error\n" );
                    msg.msg_type = msg.res_type;	/* 返信 */
                    msg.cmd_type = MC_RES;
                    msg.ssize = 0;
                    if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                        return 0;
                }
            }
            else
            {
                if( verbosity_mode >= 2 )
                    printf( "   |   :not found\n" );
                msg.msg_type = msg.res_type;	/* 返信 */
                msg.cmd_type = MC_RES;
                msg.ssize = 0;
                if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                    return 0;
            }
            break;


        case MC_NODE_LIST_NUM:
            if( verbosity_mode >= 2 )
                printf( "message:get node list num\n" );
            num = node.size(  );
            msg.msg_type = msg.res_type;		/* 返信 */
            msg.cmd_type = MC_RES;
            msg.suid = num;

            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            if( verbosity_mode >= 2 )
                printf( "   |   :node num %d\n", msg.suid );
            break;
        case MC_NODE_LIST_INFO:
            if( verbosity_mode >= 2 )
                printf( "message:get node list info\n" );
        {
            num = 0;
            nodeArrayT::iterator it = node.begin(  );
            while( it != node.end(  ) )
            {
                if( num == msg.suid )
                    break;
                num++;
                it++;
            }
            if( num < node.size(  ) )
                msg.suid = it->node;		/* 成功 */
            else
                msg.suid = -1;				/* 失敗 */
        }
            msg.msg_type = msg.res_type;		/* 返信 */
            msg.cmd_type = MC_RES;
            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            break;
        case MC_EDGE_LIST_NUM:
            if( verbosity_mode >= 2 )
                printf( "message:get edge list num\n" );
            num = edge.size(  );
            msg.msg_type = msg.res_type;		/* 返信 */
            msg.cmd_type = MC_RES;
            msg.suid = num;

            /* 送信 */
            if( ( msgsnd( msq_id, &msg, SSM_MSG_SIZE, 0 ) ) < 0 )
                return 0;
            if( verbosity_mode >= 2 )
                printf( "   |   :node num %d\n", msg.suid );
            break;
        case MC_EDGE_LIST_INFO:
        {
            ssm_msg_edge msge;
            if( verbosity_mode >= 2 ) {
                printf( "message:get edge list info\n" );
            }
            num = 0;
            edgeArrayT::iterator it = edge.begin(  );
            while( it != edge.end(  ) ){
                if( num == msg.suid ) { break; }
                num++;
                it++;
            }
            if( num < edge.size(  ) ) {
                strncpy( msge.name, it->name, SSM_SNAME_MAX );				/* 成功 */
                msge.suid = it->id;
                msge.cmd_type = MC_RES | it->dir;
                msge.node1 = it->node1;
                msge.node2 = it->node2;
            }
            else{
                msge.name[0] = '\0';		    //! false
                msge.cmd_type = MC_RES;
            }
            msge.msg_type = msg.res_type;		//! return
            //! send
            if( ( msgsnd( msq_id, &msge, sizeof(ssm_msg_edge) - sizeof(long), 0 ) ) < 0 )
                return 0;
        }
            break;
        default:
            if( verbosity_mode >= 1 ){
                fprintf( stderr, "NOTICE : unknown msg %d", msg.cmd_type );
            }
            break;
        }
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief print_help
 * @param name
 * @return
 */
/*--------------------------------------------------------------*/
int print_help( char *name )
{
    fprintf( stderr, "HELP\n" );
    fprintf( stderr, "\t-v | --verbose              : print verbose.\n" );
    fprintf( stderr, "\t-q | --quiet                : print quiet.\n" );
    fprintf( stderr, "\t   | --version              : print version.\n" );
    fprintf( stderr, "\t   | --without-check-msgque : run ssm without checking msgque.\n");
    fprintf( stderr, "\t-h | --help                 : print this help.\n" );

    fprintf( stderr, "ex)\n\t%s\n", name );
    return 0;
}



/*--------------------------------------------------------------*/
/**
 * @brief arg_analyze
 * @param argc
 * @param argv
 * @return
 */
/*--------------------------------------------------------------*/
int arg_analyze( int argc, char **argv )
{
    int opt, optIndex = 0, optFlag = 0;
    struct option longOpt[] = {
    {"version", 0, &optFlag, 'V'},
    {"without-check-msgque", 0, &optFlag, 'm'},
    {"quiet", 0, 0, 'q'},
    {"verbose", 0, 0, 'v'},
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
};

    while( ( opt = getopt_long( argc, argv, "vqh", longOpt, &optIndex ) ) != -1 ){
        switch ( opt )
        {
        case 'v':
            verbosity_mode = 2;
            break;
        case 'q':
            verbosity_mode = 0;
            break;
        case 'h':
            print_help( argv[0] );
            return 0;
            break;
        case 0:
        {
            switch ( optFlag )
            {
            case 'V':
                //printf( " Ver. %s\n", PACKAGE_VERSION );
                return 0;
                break;
            case 'm':
                is_check_msgque = 0;
                break;
            default:
                break;
            }
        }
            break;
        default:
        {
            fprintf( stderr, "help : %s -h\n", argv[0] );
            return 0;
        }
            break;
        }
    }
    return 1;
}



/*--------------------------------------------------------------*/
/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */
/*--------------------------------------------------------------*/
int main( int argc, char **argv )
{
    if( !arg_analyze( argc, argv ) ) { return 1; }

    //! display start
    if( verbosity_mode >= 1 ){
        printf( "\n" );
        printf( " --------------------------------------------------\n" );
        printf( " SSM ( Streaming data Sharing Manager )\n" );
        //printf( " Ver. %s\n", PACKAGE_VERSION );
        //printf( " Ver. %s\n", Tutorial_VERSION_MINOR );
        printf( " --------------------------------------------------\n\n" );
    }

    //! prepare evacuation gate
    escape_road(  );

    ssm_top = 0;

    if( !ssm_init(  ) ) { return -1;}

    //! message loop
    msq_loop(  );

    return 0;
}
