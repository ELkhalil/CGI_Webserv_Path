# include "cgi.hpp"

/*  CGI Constructors    */
cgi::cgi    ( server_data const& server, int locationIndex )
{
    _cgiBinPath = server.getLocations()[locationIndex].getCgiPath();
    errorMsg = "unknown Error";
    _pid = -1;
    pipeFds[0] = -1;
    _envp = NULL;
    _initServerEnvVariables(server);
}
cgi::cgi    ( void ) {}
cgi::~cgi   ( void )
{
    if (pipeFds[0] != -1)
        close(pipeFds[0]);
    if (_envp)
    {
        for (size_t i = 0; _envp[i]; ++i)
            delete[] _envp[i];
        delete[] _envp; 
        _envp = NULL;
    }
}

/* CGI Core Method  */
bool    cgi::executeCgi( request const& req)
{
    if (!_setupCgiEnvs(req) || !_initPipeFds() || !_executeCgiScript(req))
        return false;
    /*  if success  */
    return true;
}

/*  CGI Private Helper Methods  */
// Init Envirenment Variables for CGI and convert them to key=value for execve()
bool    cgi::_setupCgiEnvs( request const& req)
{
    _initRequestEnvVariables(req);

    /*  Convert the environment map to the required format (name=value) inside a vector and set it in envp */
    std::map<std::string, std::string>::const_iterator it;
    int i = 0;
    for (it = _cgiEnvVars.begin(); it != _cgiEnvVars.end(); ++it)
        _tmpEnvs.push_back(it->first + "=" + it->second);

    /*  Allocate memory for _envp based on the size of _tmpEnvs */
    _envp = new char*[_tmpEnvs.size() + 1];
    if (!_envp)
    {
        errorMsg = "Failed To Allocate Memory For execve() ENVS";
        return false;
    }
    _envp[_tmpEnvs.size()] = NULL;
    /*  Copy the contents of _tmpEnvs to _envp */
    for (size_t i = 0; i < _tmpEnvs.size(); i++)
    {
        _envp[i] = new char[_tmpEnvs[i].size() + 1];
        if (!_envp[i])
        {
            errorMsg = "Failed To Allocate Memory For execve() ENV";
            return false;
        }
        strcpy(_envp[i], _tmpEnvs[i].c_str());
    }
    return true;
}
// Init communication between parent and child processes.
bool    cgi::_initPipeFds( void) 
{
    if (pipe(pipeFds) < 0)
    {
        errorMsg = "Failed to init the pipe for communication";
        return false;
    }
    return true;
}
// execute the CGI script
bool    cgi::_executeCgiScript( FILE *body )
{
    _pid = fork();
    if (_pid < 0)
    {
        errorMsg = "Failed To Fork() to execute the CGI script";
        return false;
    }
    if (_pid == 0)
    {
        close(pipeFds[0]);
        if (dup2(fileno(body), STDIN_FILENO) == -1)
            _exit(EXIT_FAILURE);
        if (dup2(pipeFds[1], STDOUT_FILENO) == -1)
            _exit(EXIT_FAILURE);
        char    *args[2];
        args[0] = (char *)_script.c_str();
        args[1] = NULL;
        execve(_cgiBinPath.c_str(), args, _envp);
        std::cerr << "Failed to Execute: " << _script << std::endl;
        write(STDOUT_FILENO, "Status: 500\r\n\r\n", 15);
    }
    else
    {
        close(pipeFds[1]);
        int  status;
        waitpid(_pid, &status, WNOHANG);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            errorMsg = "A system call method failed inside the cgi child process";
            return false;
        }
    }
    return true;
}

// initialize envirenement variables releated to the server
void    cgi::_initServerEnvVariables( server_data const& server )
{
    _cgiEnvVars["SERVER_SOFTWARE"] = "webserv/1.0"; // The name and version of the information server software answering the request (and running the gateway). Format: name/version
    _cgiEnvVars["SERVER_NAME"] = server.getServerName();
    _cgiEnvVars["GATEWAY_INTERFACE"] = "CGI/1.1"; // The revision of the CGI specification to which this server complies. Format: CGI/revision
    _cgiEnvVars["SERVER_PORT"] = _intToString(server.getListenPort());
    _cgiEnvVars["SERVER_PROTOCOL"] = "HTTP/1.1";
}
// initialize envirenement variables releated to the request
void    cgi::_initRequestEnvVariables( request const& req )
{
    _script = "req.uri";
    _cgiEnvVars["SCRIPT_NAME"] = _script;
    _cgiEnvVars["REQUEST_METHOD"] = req._method;
    _cgiEnvVars["PATH_INFO"] = "";
        /*
            The extra path information, as given by the client. In other words, scripts can be accessed by their virtual pathname, 
            followed by extra information at the end of this path. The extra information is sent as PATH_INFO.
            This information should be decoded by the server if it comes from a URL before it is passed to the CGI script.
        */
    _cgiEnvVars["QUERY_STRING"] = req._query;
    _cgiEnvVars["CONTENT_TYPE"] = req._body.bodyType; // For queries which have attached information, such as HTTP POST and PUT, this is the content type of the data.
    _cgiEnvVars["CONTENT_LENGTH"] = req._body.contentLength; // The length of the said content as given by the client.
}
// convert port from int to string
std::string cgi::_intToString( int value )
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}