# include "cgi.hpp"

/*  CGI Constructors    */
cgi::cgi    ( std::string const& cgiBinPath) : _cgiBinPath(cgiBinPath)
{
    _error = "unknown Error";
    _pid = -1;
    _pipeFds[0] = -1;
    _envp = NULL;
}
cgi::cgi    ( void )
{

}
cgi::~cgi   ( void )
{
    if (_pipeFds[0] != -1)
    {
        close(_pipeFds[0]);
        close(_pipeFds[1]);
    }
    if (_envp)
    {
        for (size_t i = 0; _envp[i]; ++i)
            delete[] _envp[i];
        delete[] _envp; 
        _envp = NULL;
    }
}

/* CGI Core Method  */
bool    cgi::executeCgi( request const& req, response& res)
{
    if (!_setAndCheckScript(req.uri) || !_setupCgiEnvs(req) || !_initPipeFds() || !_executeCgiScript())
        return false;

    /*  read the CGI results to the response body   */
    std::string responseBody;
    ssize_t     bytesRead;
    char        buffer[4096];

    while ((bytesRead = read(_pipeFds[0], buffer, sizeof(buffer))) > 0)
        responseBody.append(buffer, bytesRead);

    if (bytesRead == -1)
    {
        _error = "Failed To Read Script Result";
        return false;
    }
    // Set the response body and appropriate headers.
    response.body = responseBody;
    response.headers["Content-Length"] = std::to_string(response.body.size());
    return true;
}

/*  CGI Private Helper Methods  */
// Set the full path to the CGI script. and Check if we have a valid access
bool    cgi::_setAndCheckScript(std::string const& scrpitName)
{
    /*  Checking Script Extention   */
    _checkScriptExtension(scrpitName);
    _scriptPath = _cgiBinPath + "/" + scrpitName;
    if (access(_scriptPath.c_str(), X_OK) == -1)
    {
        _error = "Script not found or not executable";
        return false;
    }
    return true;
}
// Init Envirenment Variables for CGI and convert them to key=value for execve()
bool    cgi::_setupCgiEnvs( request const& req)
{
    // Populate the environment map with CGI-related variables.
    _cgiEnvVars["REQUEST_METHOD"] = req.method;
    _cgiEnvVars["SCRIPT_NAME"] = req.uri;
    _cgiEnvVars["QUERY_STRING"] = req.query;
    // Add more environment variables as needed.

    // Convert the environment map to the required format (name=value) and set it in envp.
    std::map<std::string, std::string>::const_iterator it;

    int i = 0;
    for (it = _cgiEnvVars.begin(); it != _cgiEnvVars.end(); ++it)
        _tmpEnvs.push_back(it->first + "=" + it->second);

    // Allocate memory for _envp based on the size of _tmpEnvs
    _envp = new char*[_tmpEnvs.size() + 1]; // +1 for the NULL terminator
    if (!_envp)
    {
        _error = "Failed To Allocate Memory For execve() ENVS";
        return false;
    }
    _envp[_tmpEnvs.size()] = NULL;
    // Copy the contents of _tmpEnvs to _envp
    for (size_t i = 0; i < _tmpEnvs.size(); i++)
    {
        _envp[i] = new char[_tmpEnvs[i].size() + 1];
        if (!_envp[i])
        {
            _error = "Failed To Allocate Memory For execve() ENV";
            // You might want to add cleanup here
            return false;
        }
        strcpy(_envp[i], _tmpEnvs[i].c_str());
    }
    return true;
}
// Init communication between parent and child processes.
bool    cgi::_initPipeFds( void) 
{
    if (pipe(_pipeFds) < 0)
    {
        _error = "Failed to create the pipe";
        return false;
    }
    return true;
}
// execute the CGI script
bool    cgi::_executeCgiScript( void )
{
    _pid = fork();
    if (_pid < 0)
    {
        _error = "Failed To Fork() to execute the CGI script";
        return false;
    }
    if (_pid == 0)
    {
        close(_pipeFds[0]);
        if (dup2(_pipeFds[1], STDOUT_FILENO) == -1)
            _exit(EXIT_FAILURE);
        execve(_scriptPath.c_str(), NULL, _envp);
        _exit(EXIT_FAILURE);
    }
    else
    {
        int status;
        waitpid(_pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            _error = "Failed executing CGI script";
            return false;
        }
    }
    return true;
}
// Check Script Extention
bool    cgi::_checkScriptExtension( std::string const& scriptName)
{
    // Define a list of valid extensions
    const char* validExtensions[] = {".py", ".cgi", ".php"};

    for (size_t i = 0; i < sizeof(validExtensions) / sizeof(validExtensions[0]); i++)
    {
        const char* extension = validExtensions[i];
        size_t extensionLength = strlen(extension);
        if (scriptName.length() >= extensionLength &&
            scriptName.compare(scriptName.length() - extensionLength, extensionLength, extension) == 0) {
            return true;
        }
    }
    _error = "Invalid Script Extension Try: .py , .cgi , .php ";
    return false;
}