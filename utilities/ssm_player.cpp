/****************************************************************/
/**
  @file   ssm_player.c
  @brief  ssm log player
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



// g++ -o player ssm-player.cpp -I../include -lssm -Wall
#include <iostream>
#include <iomanip>
#include <vector>
#include <list>
#include <string>
#include <stdexcept>

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cstring>
#include <cmath>

#include <fcntl.h>
#include <getopt.h>

#define __MAIN__

#include "ssm2hat.hpp"
#include "ssm2hat_lib.h"
#include "ssm2hat_log.hpp"
#include "printlog.hpp"

using namespace std;
bool gShutOff = false;



/*--------------------------------------------------------------*/
/**
 * @brief ctrlC
 * @param aStatus
 */
/*--------------------------------------------------------------*/
void ctrlC( int aStatus )
{
    // exit(aStatus);
    gShutOff = true;
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
 * @brief getUnit
 * @param data
 * @param unit
 * @return
 */
/*--------------------------------------------------------------*/
int getUnit( double *data, char *unit )
{
    int i = 0;
    static const char table[] = { " kMGTPEZY" };
    static const double base = 1024;

    while( *data >= base && i < 8 ){
        *data /= base;
        i++;
    }
    if( i < 0 || i > 8 ) i = 0;
    *unit = table[i];

    if( i > 0 ) return 1;
    return 0;
}



/*==============================================================*/
/**
 * @brief The LogPlayer class
 */
/*==============================================================*/
class LogPlayer
{
    const char *logName;
    void *data;
    void *property;
    size_t propertySize;
    size_t dataSize;
    SSMApiBase stream;
    SSMLogBase log;

    int readCnt;  //! read counter
    int writeCnt; //! wirte counter

    bool mIsPlaying;

    void init(  )
    {
        data = NULL;
        property = NULL;
        dataSize = 0;
        propertySize = 0;
        readCnt = 0;
        writeCnt = 0;
        mIsPlaying = false;
    }

public:
    bool isPlaying(  )
    {
        return mIsPlaying;
    }

    void setLog( const char *logName )
    {
        this->logName = logName;
    }

    const char *getLogName() const
    {
        return logName;
    }

    LogPlayer(  )
    {
        init(  );
    }

    LogPlayer( const char *logName )
    {
        init(  );
        setLog( logName );
    }

    ~LogPlayer(  )
    {
        free( data );
        free( property );
        data = NULL;
        property = NULL;
    }

    bool open(  )
    {
        logVerbose << "open : " << logName << endl;
        if( !log.open( logName ) )
            return false;
        dataSize = log.dataSize(  );
        propertySize = log.propertySize(  );
        data = malloc( dataSize );
        property = malloc( propertySize );

        if( ( data == NULL ) || ( propertySize && property == NULL )){
            log.close();
            free( data );
            free( property );
            data = property = NULL;
            return false;
        }
        log.setBuffer( data, dataSize, property, propertySize );
        stream.setBuffer( data, dataSize, property, propertySize );

        log.readProperty(  );
        log.readNext(  );

        ssmTimeT saveTime;
        saveTime = calcSSM_life( log.getBufferNum(  ), log.getCycle(  ) );
        logVerbose << "> "<< log.getStreamName() << ", " << log.getStreamId() << ", " << saveTime << ", " << log.getCycle() << endl;
        if( !stream.create( log.getStreamName(), log.getStreamId(), saveTime, log.getCycle() ) )
            return false;

        if( propertySize && !stream.setProperty() ) return false;
        return true;
    }

    void close(  )
    {
        stream.release(  );
        log.close(  );
    }

    bool seek( ssmTimeT time )
    {
        //! read log
        if( !log.readTime( time ) ) return false;
        if( !stream.write( log.time() ) ) return false;
        writeCnt = ++readCnt;
        return true;
    }

    bool play( ssmTimeT time = gettimeSSM(  ) )
    {
        if( readCnt == writeCnt ){
            if( ( mIsPlaying = log.readNext(  ) )  ) readCnt++;
        }

        if( log.time(  ) <= time && writeCnt < readCnt ){
            stream.write( log.time(  ) );
            writeCnt = readCnt;
            return true;
        }
        return false;
    }

    bool playBack( ssmTimeT time = gettimeSSM(  ) )
    {
        if( readCnt == writeCnt ){
            if( ( mIsPlaying = log.readBack(  ) )  ) readCnt++;
        }

        if( log.time(  ) >= time && writeCnt < readCnt ){
            stream.write( log.time(  ) );
            writeCnt = readCnt;
            return true;
        }
        return false;
    }

    const ssmTimeT &getStartTime() const
    {
        return log.getStartTime();
    }
};



typedef std::list<LogPlayer> LogPlayerArray;



class MyParam
{
    void init()
    {
        startTime = -1;
        endTime = -1;
        startOffsetTime = 0.0;
        endOffsetTime = 0.0;
        speed = 1.0;
        isLoop = false;
    }
public:

    LogPlayerArray logArray;
    ssmTimeT startTime, endTime, startOffsetTime, endOffsetTime;
    bool isLoop;
    double speed;

    MyParam(){init();}

    void printHelp( int aArgc, char **aArgv )
    {
        cerr
            << "HELP" << endl
            << "\t-x | --speed <SPEED>            : set playing speed to SPEED." << endl
            << "\t-s | --start-time <OFFSET TIME> : start playing log at TIME." << endl
            << "\t-e | --end-time <OFFSET TIME>   : end playing log at TIME." << endl
            << "\t-l | --loop                     : looping mode." << endl
            << "\t-S | --start-ssmtime <SSMTIME>  : start playing log at SSMTIME." << endl
            << "\t-E | --end-ssmtime <SSMTIME>    : end playing log at SSMTIME." << endl
            << "\t-v | --verbose                  : verbose." << endl
            << "\t-q | --quiet                    : quiet." << endl
            << "\t-h | --help                     : print help" << endl
            << "ex)" << endl << "\t$ "<< aArgv[0] << " hoge1.log hoge2.log" << endl
            //<< "\t" << endl
            << endl;
    }

    bool optAnalyze(int aArgc, char **aArgv)
    {
        int opt, optIndex = 0;
        struct option longOpt[] = {
            {"speed", 1, 0, 'x'},
            {"start-time", 1, 0, 's'},
            {"end-time", 1, 0, 'e'},
            {"loop", 0, 0, 'l'},
            {"start-ssmtime", 1, 0, 'S'},
            {"end-ssmtime", 1, 0, 'E'},
            {"verbose", 0, 0, 'v'},
            {"quiet", 0, 0, 'q'},
            {"help", 0, 0, 'h'},
            {0, 0, 0, 0}
        };
        while( ( opt = getopt_long( aArgc, aArgv, "x:s:lS:e:E:vqh", longOpt, &optIndex)) != -1){
            switch(opt){
                case 'x' : speed = atof( optarg ); break;
                case 's' : startOffsetTime = atof(optarg); break;
                case 'e' : endOffsetTime = atof(optarg); break;
                case 'l' : isLoop = true; break;
                case 'S' : startTime = atof(optarg); break;
                case 'E' : endTime = atof(optarg); break;
                case 'v' : printlog::setLogVerbose(  ); break;
                case 'q' : printlog::setLogQuiet(  ); break;
                case 'h' : printHelp( aArgc, aArgv ); return false; break;
                default : { logError << "help : " << aArgv[0] << " -h" << endl; return false;}break;
            }
        }

        //! remainder is all log file
        for( ; optind < aArgc; optind++ ){
            logArray.push_back( LogPlayer( aArgv[optind] ) );
        }

        if( !logArray.size() ){
            logError << "USAGE : this program needs <LOGFILE> of ssm." << endl;
            logError << "help : " << aArgv[0] << " -h" << endl;
            return false;
        }
        return true;
    }

    bool logOpen()
    {
        LogPlayerArray::iterator log;
        ssmTimeT time = INFINITY;

        settimeSSM_is_pause( 1 ); //! pose during setting
        settimeSSM_speed( speed ); //! setting

        //! deployment logfile and create stream
        log = logArray.begin(  );
        while( log != logArray.end(  ) ){
            if( !log->open(  ) ){
                log = logArray.erase( log );
                continue;
            }
            if( log->getStartTime(  ) < time )
                time = log->getStartTime(  );
            log++;
        }

        //! ssm time setting
        if( startTime > 0 || startOffsetTime > 0 ){
            if( startTime <= 0 ) startTime = time;
            startTime += startOffsetTime;
            seek( startTime );
        }
        else{
            startTime = time;
        }

        if( endTime > 0 || endOffsetTime > 0 ){
            if( endTime <= 0 ) endTime = time;
            endTime += endOffsetTime;
        }
        else{
            endTime = INFINITY;
        }

        logVerbose << "startTime : " << startTime << endl;
        settimeSSM( startTime );

        settimeSSM_is_pause( 0 );

        return true;
    }

    bool seek( ssmTimeT time )
    {
        int isPause = gettimeSSM_is_pause();
        settimeSSM_is_pause( 1 );
        LogPlayerArray::iterator log;

        //! seek
        log = logArray.begin(  );
        while( log != logArray.end(  ) ){
            log->seek( time );
            log++;
        }
        settimeSSM( time );
        settimeSSM_is_pause( isPause );
        return true;
    }

    bool printProgressInit()
    {
        for( int i=0; i< 5; i++ ) logInfo << endl;

        return true;
    }

    bool printProgress( ssmTimeT time )
    {
        char unit;
        double total = 0;
        char day[64];
        time_t t = time;
        LogPlayerArray::iterator log;

        strftime( day, sizeof ( day ), "%Y/%m/%d(%a) %H:%M:%S %Z", localtime( &t ) );
        getUnit( &total, &unit );

        logInfo
            << "\033[s"							// 現在位置を記憶
            << "\033[" << 6 << "A"				// 移動
            << "\r\033[K"						// 左端へ移動後、右を全消去
            ;
        logInfo
            << "\033[K------[ "<< day <<" ]-------------" << endl
            << "\033[Kssm time  : " << time << endl
            << "\033[Klog time  : " << time - startTime + startOffsetTime << endl
            //<< "\033[Kdata rate : " << total << unit << endl
            << "\033[Kspeed rate: " << gettimeSSM_speed(  ) << endl
            << "\033[K" << endl;

        logInfo << "\033[u" << flush;						// 保存した位置に戻る
        return true;
    }

    bool commandAnalyze( const char *aCommand )
    {
        std::istringstream data( aCommand );
        std::string command;

        data >> command;
        if( !data.fail() )
        {
            if( command.compare( "speed" ) == 0 || command.compare( "x" ) == 0 )
            {
                double speed = 1.0;
                data >> speed;
                settimeSSM_speed( speed );
            }
            else if( command.compare( "skip" ) == 0 )
            {
                double skip = 0.1;
                data >> skip;
                settimeSSM( gettimeSSM(  ) + skip );
            }
            else if( command.compare( "seek" ) == 0 )
            {
                ssmTimeT offset = 0.0;
                data >> offset;
                seek( gettimeSSM(  ) + offset );
            }
            else if( command.compare( "pause") == 0 || command.compare( "p" ) == 0  )
            {
                settimeSSM_is_pause( 1 );
            }
            else if( command.compare( "start") == 0 || command.compare( "s" ) == 0 )
            {
                settimeSSM_is_pause( 0 );
            }
            else if( command.compare( "reverse") == 0 || command.compare( "r" ) == 0  )
            {
                settimeSSM_speed( -gettimeSSM_speed(  ) );
            }
            else if( command.compare( "quit") == 0 || command.compare( "q" ) == 0 )
            {
                gShutOff = true;
            }
            else
            {
                int c;
                for ( unsigned int i = 0; i < command.length(); i++ )
                {
                    c = command[i];
                    if( c == '+' )
                    {
                        settimeSSM_speed( gettimeSSM_speed(  ) * 1.2 );
                    }
                    else if( c == '-' )
                    {
                        settimeSSM_speed( gettimeSSM_speed(  ) / 1.2 );
                    }
                    else if( c == '>' )
                    {
                        settimeSSM_is_pause( 1 );
                        settimeSSM( gettimeSSM(  ) + 0.1 );
                    }
                }
            }
        }
        logInfo << "\033[2A\033[1M\r \n\r\033[K> " << flush;	// 2つ上へ移動して下の行を削除して下の行に移動

        return true;
    }
};



/*--------------------------------------------------------------*/
/**
 * @brief main ssm log player
 * @param aArgc
 * @param aArgv
 * @return
 */
/*--------------------------------------------------------------*/
int main( int aArgc, char **aArgv )
{
    MyParam param;
    LogPlayerArray::iterator log;
    char command[128];
    ssmTimeT time, printTime;
    bool isWorking;
    int playCnt; //! number of log until playing

    try{
        std::cout.setf(std::ios::fixed);
        std::cout << setprecision(3);

        //! keyboard nonblocking
        fcntl( fileno( stdin ), F_SETFL, O_NONBLOCK );

        //! option analyze
        if( !param.optAnalyze( aArgc, aArgv ) ) return -1;

        //! SSM initialize
        if( !initSSM(  ) ){
            logError << "ssm init error." << endl;
            return -1;
        }

        setSigInt();

        //! logfile and create stream
        log = param.logArray.begin(  );

        param.logOpen();
        param.printProgressInit();

        logInfo << "  start" << endl << endl;
        logInfo << "\033[1A" << "> " << flush;

        //! main loop
        printTime = gettimeSSM_real(  );
        while( !gShutOff )
        {
            isWorking = false;
            playCnt = 0;

            //! get now time
            time = gettimeSSM(  );

            //! play log
            log = param.logArray.begin(  );
            while( log != param.logArray.end(  ) ){
                isWorking |= ( gettimeSSM_speed(  ) >= 0 ? log->play( time ) : log->playBack( time ) );
                if( log->isPlaying(  ) ) playCnt++;
                log++;
            }
            // 終了判定
            if( !playCnt || time > param.endTime )
            {
                // ループするかどうか
                if( param.isLoop ){
                    param.seek( param.startTime );
                }
                else{
                    gShutOff = true;
                    break;
                }
            }
            // status display
            if( gettimeSSM_real(  ) >= printTime ){
                printTime += 1.0;
                param.printProgress( time );
            }

            //! command analyize
            {
                // cinがノンブロッキングできないのでしょうがなくstdinを使うことにする。
                // 時間があればマルチスレッドとかにしても良いけど、同期が面倒
                if( fgets(command, sizeof(command), stdin ) )
                    param.commandAnalyze( command );

            }
            //! wait
            if( !isWorking ) usleepSSM( 1000 ); // 1ms sleep
        }
    }
    catch(std::exception &exception){
        cerr << endl << "EXCEPTION : " << exception.what() << endl;
    }
    catch(...){
        cerr << endl << "EXCEPTION : unknown exception." << endl;
    }

    logInfo << endl;

    //! close log
    logInfo << "finalize log data" << endl;
    log = param.logArray.begin(  );
    while( log != param.logArray.end(  ) ){
        log->close(  );
        log++;
    }

    //! time initialize
    logInfo << "ssm time init" << endl;
    inittimeSSM(  );

    logInfo << "end" << endl;
    endSSM(  );

    return 0;
}
