# include "cgi.hpp"

/*  CGI Constructors    */
cgi::cgi    ( server_data const& server, location_data const& location )
{
    _cgiBinPath = location.getCgiPath();
    errorMsg = "unknown Error";
    _pid = -1;
    _envp = NULL;
    outputCGIFile = -1;
    _cgiOutputPath = "/tmp/.serveX_" + to_string(_initTime()) + "__.cgi";
    _initServerEnvVariables(server);
}
cgi::cgi    ( void ) {}
cgi::~cgi   ( void )
{
    if (outputCGIFile != -1)
        close(outputCGIFile);
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
    if (!_setupCgiEnvs(req) || !_initOutputFile() || !_executeCgiScript(req._body.bodycontent, req))
        return false;
    return true;
}

/*  ____________CGI Private Helper Methods___________  */

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
// Init output file to write CGI output
bool    cgi::_initOutputFile( void) 
{
    outputCGIFile = open(_cgiOutputPath.c_str(), O_CREAT | O_WRONLY, 0666);
    if (outputCGIFile < 0)
    {
        errorMsg = "Failed to create a file to hold CGI output";
        return false;
    }
    return true;
}
// execute the CGI script
bool    cgi::_executeCgiScript( FILE *body, request const& req )
{
    _pid = fork();
    if (_pid < 0)
    {
        errorMsg = "Fork() failed to execute the CGI script";
        return false;
    }
    if (_pid == 0)
    {
        if (req._method & POST)
            if (dup2(fileno(body), STDIN_FILENO) < 0)
                _exit(EXIT_FAILURE);
        if (dup2(outputCGIFile, STDOUT_FILENO) < 0)
            _exit(EXIT_FAILURE);
        char* args[3];
        args[0] = const_cast<char*>(_cgiBinPath.c_str());
        args[1] = const_cast<char*>(_script.c_str());
        args[2] = NULL;
        execve(args[0], args, _envp);
        write(STDOUT_FILENO, "Status: 500\r\n\r\n", 15);
        _exit(EXIT_FAILURE);
    }
    else
    {
        std::time_t start_time = std::time(NULL);
        int  status;
        while (waitpid(_pid, &status, WNOHANG) == 0)
        {
            std::time_t current_time = std::time(NULL);
            if (current_time > start_time + MAX_EXEC_TIME)
            {
                if (kill(_pid, SIGKILL) < 0)
                {
                    errorMsg = "A system call method failed inside the cgi child process";
                    return false;
                }
                errorMsg = "Execution time exceeded the server time limit";
                return false;
            }
            usleep(10000);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            errorMsg = "Failed to execute the requested (CGI)";
            return false;
        }
        close(outputCGIFile);
        outputCGIFile = -1;
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
    _script = req._path;
    _cgiEnvVars["SCRIPT_NAME"] = _script;
    if (req._method & GET)
        _cgiEnvVars["REQUEST_METHOD"] = "GET";
    else if (req._method & POST)
        _cgiEnvVars["REQUEST_METHOD"] = "POST";
    else
        _cgiEnvVars["REQUEST_METHOD"] = "DELETE";

    _cgiEnvVars["PATH_INFO"] = req._path;
        /*
            The extra path information, as given by the client. In other words, scripts can be accessed by their virtual pathname, 
            followed by extra information at the end of this path. The extra information is sent as PATH_INFO.
            This information should be decoded by the server if it comes from a URL before it is passed to the CGI script.
        */
    _cgiEnvVars["QUERY_STRING"] = req._query;
    _cgiEnvVars["CONTENT_TYPE"] = req._body.bodyType; // For queries which have attached information, such as HTTP POST and PUT, this is the content type of the data.
    _cgiEnvVars["CONTENT_LENGTH"] = req._body.contentLength; // The length of the said content as given by the client.
    // _cgiEnvVars["HTTP_COOKIE"] = req.cookie;
}
// convert port from int to string
std::string cgi::_intToString( int value )
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}
// init Time For File Creation
unsigned long   cgi::_initTime( void )
{
    struct timeval _time;
    gettimeofday(&_time, NULL);
    return (unsigned long)_time.tv_usec * 10000;
}