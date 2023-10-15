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
#define MAX_EXEC_TIME 20

std::string                         _cgiBinPath;
std::string                         _script;
std::string                         _cgiOutputPath;
int                                 outputCGIFile = -1;


unsigned long   initTime( void )
{
    struct timeval _time;
    gettimeofday(&_time, NULL);
    return (unsigned long)_time.tv_usec * 10000;
}

bool    initOutputFile( void) 
{
    outputCGIFile = open(_cgiOutputPath.c_str(), O_CREAT | O_WRONLY, 0666);
    if (outputCGIFile < 0)
    {
        std::cerr << "Failed to create a file to hold CGI output";
        _exit(1);
    }
    return true;
}


int main(int ac, char* av[], char **env)
{
    int pid;

    if (ac != 3)
        return 1;
    
    _cgiBinPath = av[1];
    _script = av[2];
    _cgiOutputPath =  "/tmp/.serveX__.cgi";

    if (!initOutputFile()){
        std::cerr << "Failed to initialize the pipe for communication" << std::endl;
        return 1;
    }

    pid = fork();
    
    if (pid < 0) {
        std::cerr << "Failed to fork() to execute the CGI script" << std::endl;
        return 1;
    }
        std::time_t start_time = std::time(NULL);
    if (pid == 0)
    {
        if (dup2(outputCGIFile, STDOUT_FILENO) < 0)
            _exit(EXIT_FAILURE);
        char* args[3];
        args[0] = const_cast<char*>(_cgiBinPath.c_str());
        args[1] = const_cast<char*>(_script.c_str());
        args[2] = NULL;
        execve(args[0], args, env);
        write(STDOUT_FILENO, "Status: 500\r\n\r\n", 15);
        _exit(EXIT_FAILURE);
    }
    else
    {
        int  status;
        while (waitpid(pid, &status, WNOHANG) == 0)
        {
            std::time_t current_time = std::time(NULL);
            if (current_time > start_time + MAX_EXEC_TIME)
            {
                if (kill(pid, SIGKILL) < 0)
                {
                    std::cerr << "A system call method failed inside the cgi child process";
                    _exit(1);
                }
                std::cerr << "Execution time exceeded the limit: script hanged";
                _exit(1);
            }
            usleep(10000);
        }
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
        {
            std::cerr << "A system call method failed inside the child process (CGI)";
            _exit(1);
        }
    }
    close(outputCGIFile);
    usleep(10000);

    int newfd = open(_cgiOutputPath.c_str(), O_RDONLY); // Use O_RDONLY to open for reading
    char buffer[1024];
    ssize_t bytesRead;

    if (newfd < 0) {
        std::cerr << "Failed to open the CGI output file for reading" << std::endl;
        return 1;
    }

    while ((bytesRead = read(newfd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    close(newfd);
    return 0;
}