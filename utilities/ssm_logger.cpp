/****************************************************************/
/**
  @file   ssm_logger.cpp
  @brief  ssm log get
  @author HATTORI Kohei <hattori[at]team-lab.com>
 */
/****************************************************************/



// g++ -o logger ssm-logger.cpp -I../include -lssm
#include <iostream>
#include <string>
#include <stdexcept>

#include <cstdlib>
#include <csignal>
#include <cstring>

#include <getopt.h>

#define __MAIN__
#include "printlog.hpp"
#include "ssm2hat_log.hpp"



using namespace std;
static const ssmTimeT WAITTIME = 60; //![s] wait for stream open
bool gShutOff = false;



/*==============================================================*/
/**
 * @brief The MyParam class
 */
/*==============================================================*/
class MyParam
{
    void init(){
        logName = NULL;
        streamName = NULL;
        streamId = 0;
    }
public:
    char *logName;
    char *streamName;
    int streamId;

    MyParam(){init();}

    void printHelp( int aArgc, char **aArgv )
    {
        cerr
            << "HELP" << endl
            << "\t-l | --log-name <LOGFILE> : set name of logfile to LOGFILE." << endl
            << "\t-n | --stream-name <NAME> : set stream-name of ssm to NAME." << endl
            << "\t-i | --stream-id <ID>     : set stream-id of ssm to ID." << endl
            << "\t-v | --verbose            : verbose." << endl
            << "\t-q | --quiet              : quiet." << endl
            << "\t-h | --help               : print help" << endl
            << "ex)" << endl << "\t$ "<< aArgv[0] << " -l hoge.log -n hogehoge -i 0" << endl
            //<< "\t" << endl
            << endl;
    }

    bool optAnalyze(int aArgc, char **aArgv){
        int opt, optIndex = 0;
        struct option longOpt[] = {
            {"log-name", 1, 0, 'l'},
            {"stream-id", 1, 0, 'i'},
            {"stream-name", 1, 0, 'n'},
            {"verbose", 0, 0, 'v'},
            {"quiet", 0, 0, 'q'},
            {"help", 0, 0, 'h'},
            {0, 0, 0, 0}
        };
        while( ( opt = getopt_long( aArgc, aArgv, "l:i:n:vh", longOpt, &optIndex)) != -1){
            switch(opt){
                case 'l' : logName = optarg; break;
                case 'n' : streamName = optarg; break;
                case 'i' : streamId = atoi(optarg); break;
                case 'v' : printlog::setLogVerbose(  ); break;
                case 'q' : printlog::setLogQuiet(  ); break;
                case 'h' : printHelp( aArgc, aArgv ); return false; break;
                default : {cerr << "help : " << aArgv[0] << " -h" << endl; return false;}break;
            }
        }

        if( ( !logName ) || ( !streamName ) ){
            cerr << "USAGE : this program need <LOGNAME> and <NAME> of stream." << endl;
            cerr << "help : " << aArgv[0] << " -h" << endl;
            return false;
        }

        return true;
    }
};



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
 * @brief ssm log save
 * @param aArgc
 * @param aArgv
 * @return
 */
/*--------------------------------------------------------------*/
int main( int aArgc, char **aArgv )
{
    SSMLogBase log;
    SSMApiBase stream;
    MyParam param;
    double cycle;
    int num;
    void *data = NULL;
    void *property = NULL;
    size_t propertySize = 0;
    size_t dataSize = 0;

    //! option analyze
    if( !param.optAnalyze( aArgc, aArgv ) ) return -1;

    //! SSM initialize
    if( !initSSM(  ) ){
        logError << "ERROR: ssm init error." << endl;
        return -1;
    }

    setSigInt();

    //! connect to SSM
    ssmTimeT startTime = gettimeSSM_real(  );
    while( getSSM_info( param.streamName, param.streamId, &dataSize, &num, &cycle, &propertySize ) <= 0 ){
        if( gShutOff ){
            endSSM();
            return -1;
        }

        if( gettimeSSM_real() - startTime > WAITTIME ){
            logError << "ERROR: cannot get ssm info of '"<< param.streamName << " ["<<  param.streamId<< "]." << endl;
            endSSM();
            return -1;
        }
        usleep( 100000 );
    }

    try{
        //! assure memory
        if( ( data = malloc( dataSize ) ) == NULL )
            throw runtime_error("memory allocation of data");
        if( ( property = malloc( propertySize ) ) == NULL )
            throw runtime_error("memory allocation of property");

        log.setBuffer( data, dataSize, property, propertySize );
        stream.setBuffer( data, dataSize, property, propertySize );

        //! open stream
        if( !stream.open( param.streamName, param.streamId ) )
            throw runtime_error( "stream is not exist." );
        if( propertySize > 0 && ( !stream.getProperty(  ) ) )
            throw runtime_error( "cannot get property" );
        logInfo << "open stream '" << param.streamName << "' [" << param.streamId << "]." << endl;

        stream.readLast(  );
        stream.setBlocking( true );

        //! create log file
        if( !log.create( param.streamName, param.streamId, num, cycle, param.logName ) )
            throw runtime_error( "cannot create logfile" );

        logVerbose << "create log '" << param.logName << "'." << endl;

        //! main loop
        while( !gShutOff ){
            //! read from stream
            if( !stream.readNext(  ) ) continue;

            //! write for file
            if( !log.write( stream.time ) )
                throw runtime_error( "cannot write log." );
        }
    }
    catch(std::exception &exception){
        cerr << endl << "EXCEPTION : " << exception.what() << endl;
    }

    catch(...){
        cerr << endl << "EXCEPTION : unknown exception." << endl;
    }

    log.close(  );

    //! shutout from ssm
    stream.close(  );
    endSSM(  );

    //! release memory
    free( data );
    free( property );

    return 0;
}

