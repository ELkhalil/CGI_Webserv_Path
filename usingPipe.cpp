#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctime>
#include <cstdlib>
#include <signal.h>

#define MAX_EXEC_TIME 10

int main(int ac, char *av[], char **env) {
    if (ac != 3) {
        return 1;
    }

    std::string binPath = av[1];
    std::string scriptPath = av[2];

    int pipeFds[2];
    int pid;

    if (pipe(pipeFds) < 0) {
        std::cerr << "Failed to initialize the pipe for communication" << std::endl;
        return 1;
    }
    
    pid = fork();
    
    if (pid < 0) {
        std::cerr << "Failed to fork() to execute the CGI script" << std::endl;
        return 1;
    }

    if (pid == 0) {
        close(pipeFds[0]);
        
        if (dup2(pipeFds[1], STDOUT_FILENO) == -1) {
            _exit(EXIT_FAILURE);
        }

        char* args[3];
        args[0] = const_cast<char*>(binPath.c_str());
        args[1] = const_cast<char*>(scriptPath.c_str());
        args[2] = NULL;

        execve(args[0], args, env);
        
        // If execve fails, return an error response
        write(STDOUT_FILENO, "Status: 500\r\n\r\n", 15); // Internal Server Error
        _exit(EXIT_FAILURE);
    } 
    else 
    {
        close(pipeFds[1]);

        std::time_t start_cgi = std::time(nullptr); // Store the start time
        int status;
        pid_t result;

        // Wait for the child process to exit with a timeout
        while ((result = waitpid(pid, &status, WNOHANG) == 0)) 
        {
            std::time_t current_time = std::time(nullptr);

            if (current_time > start_cgi + MAX_EXEC_TIME) {
                // Execution time exceeded the limit; kill the hung process
                if (kill(pid, SIGKILL) == -1) {
                    std::cerr << "Failed to send SIGKILL to terminate the process." << std::endl;
                }
                std::cerr << "Execution time exceeded the limit; process terminated." << std::endl;
            }

            usleep(10000); // Introduce a short delay between checks
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            std::cerr << "A system call method failed inside the CGI child process." << std::endl;
            return 1;
        }
        
        char buffer[1024];
        ssize_t bytesRead;
        
        while ((bytesRead = read(pipeFds[0], buffer, sizeof(buffer))) > 0) {
            write(STDOUT_FILENO, buffer, bytesRead);
        }

        close(pipeFds[0]);
    }

    return 0;
}
