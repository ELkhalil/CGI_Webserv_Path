#ifndef CGI_HPP
#define CGI_HPP

# include <iostream>
# include <vector>
# include <map>
# include <cstring>
# include <sys/wait.h>
# include <sys/time.h>
# include <unistd.h>
# include <fcntl.h>
# include <signal.h>
# include <stdio.h>
# include "request.hpp"

#define MAX_EXEC_TIME 10

class   cgi
{

public:

    cgi             ( server_data const& , location_data const& ); // int: the location index inside the calling serve
    ~cgi            ( void );
    bool            executeCgi( request const& );
    std::string     errorMsg;
    int             _pid;
    std::string     _cgiOutputPath;

private:

    cgi                                 ( void );
    std::string                         _cgiBinPath;
    std::string                         _script;
    std::map<std::string, std::string>  _cgiEnvVars;
    std::vector<std::string>            _tmpEnvs;
    char**                              _envp;
    int                                 outputCGIFile;

    /*  Helper Private Functions */
    void            _initServerEnvVariables( server_data const& );
    void            _initRequestEnvVariables( request const& );
    bool            _initOutputFile( void );
    bool            _setupCgiEnvs( request const& );
    bool            _executeCgiScript( FILE *body, request const& );
    std::string     _intToString( int );
    unsigned long   _initTime( void );

};
#endif
