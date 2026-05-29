#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <cctype>
using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;
    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
    // TODO: add your implementation
    name = "smash";
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
    
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    
    if (firstWord.compare("chprompt") == 0) {
      return new ChpromptCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
      return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
      return new ChangeDirCommand(cmd_line);
    }
    // else {
    //   return new ExternalCommand(cmd_line);
    // }
    



    return nullptr;
}

Command::Command(const char *cmd_line) : cmd_line(_rtrim(std::string(cmd_line))) {}
Command::~Command() {}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ExternalCommand::ExternalCommand(const char *cmd_line, int p_id) : Command(cmd_line), p_id(p_id) {}

void ExternalCommand::execute() {

}

/*
void ExternalCommand::execute() {
    bool is_bg = _isBackgroundComamnd(cmd_line.c_str());
    bool is_complex = cmd_line.find('*') != string::npos || cmd_line.find('?') != string::npos;

    char cmd_copy[COMMAND_MAX_LENGTH + 1];
    strncpy(cmd_copy, cmd_line.c_str(), COMMAND_MAX_LENGTH);
    cmd_copy[COMMAND_MAX_LENGTH] = '\0';
    _removeBackgroundSign(cmd_copy);

    pid_t pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        // child
        setpgrp();
        if (is_complex) {
            execl("/bin/bash", "bash", "-c", cmd_copy, NULL);
        } else {
            char* args[COMMAND_MAX_ARGS + 1];
            _parseCommandLine(cmd_copy, args);
            execvp(args[0], args);
        }
        perror("smash error: exec failed");
        exit(1);
    } else {
        // parent
        p_id = pid;
        if (!is_bg) {
            waitpid(pid, nullptr, 0);
        }
    }
}
*/

JobsList::JobsList() {}
JobsList::~JobsList() {}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}


void SmallShell::executeCommand(const char *cmd_line) {
    Command* cmd = CreateCommand(cmd_line);
    if(cmd == nullptr) return;
    cmd->execute();
    if(dynamic_cast<ExternalCommand*>(cmd) != nullptr) {
        if(_isBackgroundComamnd(cmd_line)) {
            jobs.addJob(cmd);
        }
    }
    delete cmd;
}


ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* buf = getcwd(nullptr, 0); 
    if (buf != nullptr) {
        std::cout << buf << std::endl;
        free(buf); 
    } else {
        perror("smash error: getcwd failed");
    }
}


void ChangeDirCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);

    SmallShell& smash = SmallShell::getInstance();

    // too many args
    if (num_args > 2) {
        std::cerr << "smash error: cd: too many arguments\n";
    } 
    else if (num_args == 2) {
        std::string target_path = args[1];
        
        // "cd -" but no previous directory is set
        if (target_path == "-") {
            if (smash.getLastPwd().empty()) {
                std::cerr << "smash error: cd: OLDPWD not set\n";
                
                for (int i = 0; i < num_args; i++) free(args[i]);
                return; 
            } else {
                //  a prev path exists
                target_path = smash.getLastPwd();
            }
        }

        char* current_dir_buf = getcwd(nullptr, 0);
        std::string current_dir = (current_dir_buf != nullptr) ? std::string(current_dir_buf) : "";
        if (current_dir_buf) free(current_dir_buf);

        // ERROR CASE 3: chdir system call fails
        if (chdir(target_path.c_str()) == -1) {
            perror("smash error: chdir failed");
        } else {
            // Update the the last directory
            smash.setLastPwd(current_dir);
        }
    }

    // Clean up parsed arguments
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}
void JobsList::addJob(Command *cmd, bool isStopped) {
    if (ExternalCommand* ext = dynamic_cast<ExternalCommand*>(cmd)) {
        int new_id = 1;
        for (auto& j : jobs) {
            if (j.getJobId() >= new_id) new_id = j.getJobId();
        }
        jobs.push_back(JobEntry(new_id+1, ext->getPid(), cmd->getCmdLine(), isStopped));
    }
}

void ChpromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    // The skeleton provides this handy parser! It fills the 'args' array and returns the number of arguments.
    int num_args = _parseCommandLine(cmd_line.c_str(), args); 

    SmallShell& smash = SmallShell::getInstance();
    
    // If no arguments provided (just "chprompt"), reset to "smash"
    if (num_args == 1) {
        smash.setName("smash");
    } 
    // If arguments are provided, use the first one
    else if (num_args > 1) {
        smash.setName(args[1]);
    }

    // Clean up the memory allocated by _parseCommandLine
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}