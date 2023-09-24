#ifndef CGI_HPP
#define CGI_HPP

# include <iostream>
# include <vector>
# include <map>
# include <cstring>
# include "request.hpp"
# include "response.hpp"
# include <sys/wait.h>
# include <unistd.h>
# include <fcntl.h>
# include <signal.h>

class   cgi
{

public:
    cgi     ( std::string const& );
    ~cgi    ( void );
    bool    executeCgi( request const&, response& );
private:
    cgi     ( void );
    std::string                         _cgiBinPath;
    std::string                         _scriptPath;
    std::string                         _error;
    int                                 _pipeFds[2];
    int                                 _pid;
    std::map<std::string, std::string>  _cgiEnvVars;
    std::vector<std::string>            _tmpEnvs;
    char**                              _envp;

    /*  Helper Private Functions */
    bool    _setAndCheckScript(std::string const& );
    bool    _initPipeFds( void );
    bool    _setupCgiEnvs( request const& );
    bool    _executeCgiScript( void );
    bool    _checkScriptExtension( std::string const& );
};
#endif