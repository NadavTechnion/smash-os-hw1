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


bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}


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
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    if (idx == string::npos) {
        return;
    }
    if (cmd_line[idx] != '&') {
        return;
    }
    cmd_line[idx] = ' ';
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

SmallShell::SmallShell() {
    name = "smash";
}

SmallShell::~SmallShell() {}

Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("chprompt") == 0) {
        return new ChpromptCommand(cmd_line);
    }
    else if (firstWord.compare("quit") == 0) {
        return new QuitCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("jobs") == 0) {
        return new JobsCommand(cmd_line);
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("cd") == 0) {
        return new ChangeDirCommand(cmd_line);
    }
    else if (firstWord.compare("kill") == 0) {
        return new KillCommand(cmd_line);
    }
    else if (firstWord.compare("fg") == 0) {
        return new ForegroundCommand(cmd_line);
    }
    else {
        return new ExternalCommand(cmd_line, 0);
    }
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    if (_trim(string(cmd_line)).empty()) return;
    SmallShell& smash = SmallShell::getInstance();
    smash.getJobManager().removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    if (cmd == nullptr) return;
    cmd->execute();
    delete cmd;
}

Command::Command(const char *cmd_line) : cmd_line(_rtrim(std::string(cmd_line))) {}
Command::~Command() {}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ExternalCommand::ExternalCommand(const char *cmd_line, int p_id) : Command(cmd_line), p_id(p_id) {}

void ExternalCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    bool is_bg = _isBackgroundComamnd(cmd_line.c_str());
    bool is_complex = cmd_line.find('*') != string::npos || cmd_line.find('?') != string::npos;
    char command_line[COMMAND_MAX_LENGTH+1];

    strncpy(command_line, cmd_line.c_str(), COMMAND_MAX_LENGTH);
    _removeBackgroundSign(command_line);
    int pid = fork();
    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid == 0) {
        setpgrp();
        if (is_complex) {
            execl("/bin/bash", "bash", "-c", command_line, NULL);
        }
        else {
            char* args[COMMAND_MAX_ARGS+1];
            _parseCommandLine(command_line, args);
            execvp(args[0], args);
        }
        perror("smash error: exec failed");
        exit(1);
    }
    else {
        p_id = pid;
        if (!is_bg) {
            waitpid(pid, nullptr, WUNTRACED);
        }
        else {
            smash.getJobManager().addJob(this);
        }
    }
}

ChpromptCommand::ChpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChpromptCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    if (num_args == 1) {
        smash.setName("smash");
    }
    else if (num_args > 1) {
        smash.setName(args[1]);
    }
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    std::cout << "smash pid is " << getpid() << std::endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
    char* buf = getcwd(nullptr, 0);
    if (buf != nullptr) {
        std::cout << buf << std::endl;
        free(buf);
    } else {
        perror("smash error: getcwd failed");
    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    if (num_args > 2) {
        std::cerr << "smash error: cd: too many arguments\n";
    }
    else if (num_args == 2) {
        std::string target_path = args[1];
        if (target_path == "-") {
            if (smash.getLastPwd().empty()) {
                std::cerr << "smash error: cd: OLDPWD not set\n";
                for (int i = 0; i < num_args; i++) free(args[i]);
                return;
            } else {
                target_path = smash.getLastPwd();
            }
        }
        char* current_dir_buf = getcwd(nullptr, 0);
        std::string current_dir = (current_dir_buf != nullptr) ? std::string(current_dir_buf) : "";
        if (current_dir_buf) free(current_dir_buf);
        if (chdir(target_path.c_str()) == -1) {
            perror("smash error: chdir failed");
        } else {
            smash.setLastPwd(current_dir);
        }
    }
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

void QuitCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    bool is_kill = num_args > 1 && string(args[1]) == "kill";
    JobsList jobManager = SmallShell::getInstance().getJobManager();
    if(is_kill) {
        cout << "smash: sending SIGKILL signal to " << jobManager.size() << " jobs:" << endl;
        jobManager.killAllJobs();
    }
    exit(0);
}

void JobsCommand::execute() {
    SmallShell::getInstance().getJobManager().printJobs();
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    JobsList jobManager = SmallShell::getInstance().getJobManager();
    if (num_args != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
    else {
        if(string(args[1]).size() > 1 && isNumber(std::string(args[1]+1)) && isNumber(std::string(args[2]))) {
            int sigNum = atoi(args[1] + 1);
            int jobId = atoi(args[2]);
            JobsList::JobEntry* job = jobManager.getJobById(jobId);
            if (job != nullptr) {
                cout << "signal number " << sigNum << " was sent to pid" <<job-> getPid() << endl;
                jobManager.removeJobById(jobId);
            }
            else {
                cerr << "smash error: kill: job-id <"<< jobId << "> does not exist" << endl;
            }
        }
        else {
            cerr << "smash error: kill: invalid arguments" << endl;
        }


    }


}

ForegroundCommand::ForegroundCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ForegroundCommand::execute() {
    int jobId = 0;
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();
    if (num_args > 2) {
        std::cerr << "smash error: fg: invalid arguments" << std::endl;
        return;
    }
    if(num_args == 2) {
        for (char c : string(args[1])) {
            if (!std::isdigit(c)) {
                std::cerr << "smash error: fg: invalid arguments" << std::endl;
                return;
            }
                jobId+= c - '0'; }

        JobsList::JobEntry* job = smash.getJobManager().getJobById(jobId);
        if(job == nullptr) {
            std::cerr << "smash error: fg: job-id <" << jobId << "> does not exist" << std::endl;
            return;
        }
        cout << job ->getCmdLine() <<" " << job->getPid() << endl;
        waitpid(job->getPid(), nullptr, WUNTRACED);
        smash.getJobManager().removeJobById(jobId);
    }
    else {
        if(smash.getJobManager().size() == 0) {
            std::cerr << "smash error: fg: jobs list is empty"<< std::endl;
            return;
        }
         JobsList::JobEntry * firstJob = smash.getJobManager().getFirstJob();
        cout <<firstJob->getCmdLine() <<" " << firstJob->getPid() << endl;
        waitpid(firstJob->getPid(), nullptr, WUNTRACED);
        smash.getJobManager().removeJobById(firstJob->getJobId());
    }
}

JobsList::JobsList() {}
JobsList::~JobsList() {}

void JobsList::addJob(Command *cmd, bool isStopped) {
    if (ExternalCommand* ext = dynamic_cast<ExternalCommand*>(cmd)) {
        int new_id = jobs.empty() ? 1 : jobs.rbegin()->first + 1;
        jobs.emplace(new_id, JobEntry(new_id, ext->getPid(), cmd->getCmdLine(), isStopped));
    }
}

void JobsList::removeFinishedJobs() {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (auto it = jobs.begin(); it != jobs.end(); ++it) {
            if (it->second.getPid() == pid) {
                jobs.erase(it);
                break;
            }
        }
    }
}

void JobsList::killAllJobs() {
    for (auto& pair : jobs) {
        kill(pair.second.getPid(), SIGKILL);
    }
    jobs.clear();
}

void JobsList::printJobs() {
    for (auto& pair : jobs) {
        cout << pair.second.info() << endl;
    }
}

JobsList::JobEntry* JobsList::getFirstJob() {
    if (jobs.empty()) return nullptr;
    return &jobs.begin()->second;
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    auto it = jobs.find(jobId);
    if (it != jobs.end()) {
        return &it->second;
    }
    return nullptr;
}
void JobsList::removeJobById(int jobId){
    auto it = jobs.find(jobId);
    if(it != jobs.end()) jobs.erase(it);
}
int JobsList::size() {
    return jobs.size();
}

std::string JobsList::JobEntry::info() {
    return "[" + std::to_string(job_id) + "] " + cmd_line;
}