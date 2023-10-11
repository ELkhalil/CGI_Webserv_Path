#ifndef CGI_HPP
#define CGI_HPP

# include <iostream>
# include <vector>
# include <map>
# include <cstring>
# include "request.hpp"
# include <sys/wait.h>
# include <unistd.h>
# include <fcntl.h>
# include <signal.h>

class   cgi
{

public:

    cgi             ( server_data const& , int ); // int: the location index inside the calling server
    ~cgi            ( void );
    bool            executeCgi( request const& );
    int             pipeFds[2];
    std::string     errorMsg;

private:

    cgi                                 ( void );
    std::string                         _cgiBinPath;
    std::string                         _script;
    int                                 _pid;
    std::map<std::string, std::string>  _cgiEnvVars;
    std::vector<std::string>            _tmpEnvs;
    char**                              _envp;

    /*  Helper Private Functions */
    void            _initServerEnvVariables( server_data const& );
    void            _initRequestEnvVariables( request const& );
    bool            _initPipeFds( void );
    bool            _setupCgiEnvs( request const& );
    bool            _executeCgiScript( FILE *body );
    std::string     _intToString( int );

};
#endif
