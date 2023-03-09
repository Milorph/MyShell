#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <cstring>
#include <sstream>
#include <errno.h>

using namespace std;

vector<vector<string> > commands;

int main() {
    // Read the command line from the user
    string line;
    cout << "myshell$";
    getline(cin, line);

    // Split the command line into commands and arguments
    stringstream ss(line);
    string command;
    while (getline(ss, command, '|')) {
        vector<string> args;
        string arg;
        stringstream arg_ss(command);
        while (arg_ss >> arg) {
            args.push_back(arg);
        }
        commands.push_back(args);
    }

    // Create pipes (N - 1) ~ N is number of commands
    int num_pipes = commands.size() - 1;
    int pipes[num_pipes][2]; //Two ends to each pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipes[i]) < 0) {
            cerr << "Error creating pipe: " << strerror(errno) << endl;
            return 1;
        }
    }

    // Fork child processes ~ equivalent to number of commands
    for (int i = 0; i < commands.size(); i++) {

        // Child Process
        if (fork() == 0) {
            // If its not the first command, then redirect.
            if (i > 0) {
                // Redirect input to the read end of the previous pipe
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    cerr << "Error redirecting input: " << strerror(errno) << endl;
                    exit(1);
                }
            }
            if (i < num_pipes) {
                // Redirect output to the write end of the current pipe
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    cerr << "Error redirecting output: " << strerror(errno) << endl;
                    exit(1);
                }
            }

            // Close all unused file descriptors
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Convert vector<string> to char*[] for execvp to work
            vector<char*> args;
            for (string& s : commands[i]) {
                args.push_back(&s[0]);
            }
            args.push_back(nullptr);

            // Execute command
            execvp(args[0], args.data());
            cerr << "Error executing command: " << strerror(errno) << endl;
            exit(1);
        }
    }

    // Close unused file descriptors in parent process
    for (int i = 0; i < num_pipes; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
    }
    // Wait for child processes to terminate
    for (int i = 0; i < commands.size(); i++) {
        int status;
        pid_t pid = wait(&status);
        cout << "Child process " << i << " (PID " << pid << ") terminated with status " << status << endl;
    }

    return 0;
}

