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
#include <fstream>
extern char **environ;
#include <unistd.h>

// המערך הגלובלי שלינוקס שומר בו את משתני הסביבה - בדיוק כפי שה-PDF דורש
extern char **__environ;


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

SmallShell::~SmallShell() {
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // מנקים רווחים מיותרים מההתחלה והסוף
    string cmd_s = _trim(string(cmd_line));
    
    // אם שורת הפקודה ריקה, אין מה לעשות
    if (cmd_s.empty()) {
        return nullptr;
    }



    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n\r\t"));
    
    if (isAliasExists(firstWord)) {
        std::string aliasContent = getAliasCommand(firstWord);
        std::string restOfCommand = cmd_s.substr(firstWord.length());
        std::string fullNewCmd = aliasContent + restOfCommand;
        return CreateCommand(fullNewCmd.c_str());
    }
    // check if output redirecting is needed
    if (cmd_s.find(">") != string::npos) {
        return new RedirectionCommand(cmd_line);
    }

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
    else if (firstWord.compare("alias") == 0) {
        return new AliasCommand(cmd_line);
    }
    else if (firstWord.compare("unalias") == 0) {
        return new UnAliasCommand(cmd_line);
    }
    else if (firstWord.compare("unsetenv") == 0) {
        return new UnSetEnvCommand(cmd_line);
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


Command::Command(const char *cmd_line) : cmd_line(_rtrim(std::string(cmd_line))) {}
Command::~Command() {}

BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {}

ExternalCommand::ExternalCommand(const char *cmd_line, int p_id) : Command(cmd_line), p_id(p_id) {}

void ExternalCommand::execute() {
    JobsList& jobManager = SmallShell::getInstance().getJobManager();
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
            jobManager.addJob(this);
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
    JobsList& jobManager = SmallShell::getInstance().getJobManager();
    if(is_kill) {
        cout << "smash: sending SIGKILL signal to " << jobManager.size() << " jobs:" << endl;
        jobManager.printAndKillAllJobs();
    }
    exit(0);
}

void JobsCommand::execute() {
    JobsList& jobManager = SmallShell::getInstance().getJobManager();
    jobManager.printJobs();
}

KillCommand::KillCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void KillCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    JobsList& jobManager = SmallShell::getInstance().getJobManager();
    if (num_args != 3) {
        cerr << "smash error: kill: invalid arguments" << endl;
    }
    else {
        if(string(args[1]).size() > 1 && isNumber(std::string(args[1]+1)) && isNumber(std::string(args[2]))) {
            int sigNum = atoi(args[1] + 1);
            int jobId = atoi(args[2]);
            JobsList::JobEntry* job = jobManager.getJobById(jobId);
            if (job != nullptr) {
                cout << "signal number " << sigNum << " was sent to pid " << job->getPid() << endl;
                kill(job->getPid(), sigNum);
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
    JobsList& jobManager = SmallShell::getInstance().getJobManager();
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
            jobId = jobId * 10 + (c - '0');
        }
        JobsList::JobEntry* job = jobManager.getJobById(jobId);
        if(job == nullptr) {
            std::cerr << "smash error: fg: job-id <" << jobId << "> does not exist" << std::endl;
            return;
        }
        cout << job->getCmdLine() << " : " << job->getPid() << endl;
        if (job->getIsStopped()) kill(job->getPid(), SIGCONT);
        waitpid(job->getPid(), nullptr, WUNTRACED);
        jobManager.removeJobById(jobId);
    }
    else {
        if(jobManager.size() == 0) {
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            return;
        }
        JobsList::JobEntry* lastJob = jobManager.getLastJob();
        cout << lastJob->getCmdLine() << " : " << lastJob->getPid() << endl;
        if (lastJob->getIsStopped()) kill(lastJob->getPid(), SIGCONT);
        int lastJobId = lastJob->getJobId();
        waitpid(lastJob->getPid(), nullptr, WUNTRACED);
        jobManager.removeJobById(lastJobId);
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

JobsList::JobEntry* JobsList::getLastJob() {
    if (jobs.empty()) return nullptr;
    return &jobs.rbegin()->second;
}

void JobsList::printAndKillAllJobs() {
    for (auto& pair : jobs) {
        cout << pair.second.getPid() << ": " << pair.second.getCmdLine() << endl;
        kill(pair.second.getPid(), SIGKILL);
    }
    jobs.clear();
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    auto it = jobs.find(jobId);
    if (it != jobs.end()) {
        return &it->second;
    }
    return nullptr;
}

AliasCommand::AliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void AliasCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    string cmd_s = _trim(string(cmd_line));

    // print all aliases
    if (cmd_s == "alias") {
        for (const auto& pair : smash.getAliases()) {
            cout << pair.first << "='" << pair.second << "'" << endl;
        }
        return;
    }

    // add a new alias
    size_t equal_sign = cmd_s.find('=');
    size_t first_quote = cmd_s.find('\'', equal_sign);
    size_t last_quote = cmd_s.find_last_of('\'');

    //check syntax 
    if (equal_sign == string::npos || first_quote == string::npos || last_quote == string::npos || first_quote == last_quote) {
        cerr << "smash error: alias: invalid alias format" << endl;
        return;
    }

    // parse
    string name = _trim(cmd_s.substr(5, equal_sign - 5)); 
    string command = cmd_s.substr(first_quote + 1, last_quote - first_quote - 1);
    //check for legal name
    for (char c : name) {
        if (!isalnum(c) && c != '_') {
            cerr << "smash error: alias: invalid alias format" << endl;
            return;
        }
    }

    bool is_existing_alias = smash.isAliasExists(name);
    
    Command* testCmd = smash.CreateCommand(name.c_str());
    bool is_reserved_cmd = (dynamic_cast<BuiltInCommand*>(testCmd) != nullptr);
    
    if (testCmd != nullptr) {
        delete testCmd;
    }

    if (is_existing_alias || is_reserved_cmd) {
        cerr << "smash error: alias: " << name << " already exists or is a reserved command" << endl;
        return;
    }

    smash.addAlias(name, command);
    }

UnAliasCommand::UnAliasCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void UnAliasCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);
    SmallShell& smash = SmallShell::getInstance();

    if (num_args == 1) {
        std::cerr << "smash error: unalias: not enough arguments\n";
    } else {
        for (int i = 1; i < num_args; ++i) {
            std::string alias_name = args[i];
            
            if (!smash.isAliasExists(alias_name)) {
                std::cerr << "smash error: unalias: " << alias_name << " alias does not exist\n";
                break;
            }

            smash.removeAlias(alias_name);
        }
    }

    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

UnSetEnvCommand::UnSetEnvCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};


bool UnSetEnvCommand::isEnvExistsInProc(const std::string& var_name) {
    pid_t pid = getpid();
    std::string proc_path = "/proc/" + std::to_string(pid) + "/environ";
    
    std::ifstream env_file(proc_path, std::ios::binary);
    if (!env_file.is_open()) {
        return false; 
    }

    std::string entry;
    while (std::getline(env_file, entry, '\0')) {
        size_t eq_pos = entry.find('=');
        if (eq_pos != std::string::npos) {
            std::string current_var = entry.substr(0, eq_pos);
            if (current_var == var_name) {
                return true; 
            }
        }
    }
    return false;
}

void UnSetEnvCommand::removeEnvFromGlobalArray(const std::string& var_name) {
    if (__environ == nullptr) return;

    int i = 0;
    while (__environ[i] != nullptr) {
        std::string entry(__environ[i]);
        size_t eq_pos = entry.find('=');
        std::string current_var = entry.substr(0, eq_pos);

        if (current_var == var_name) {
            int j = i;
            while (__environ[j] != nullptr) {
                __environ[j] = __environ[j + 1];
                j++;
            }
        } else {
            i++;
        }
    }
}

void UnSetEnvCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line.c_str(), args);

    if (num_args == 1) {
        std::cerr << "smash error: unsetenv: not enough arguments\n";
    } else {
        for (int i = 1; i < num_args; ++i) {
            std::string var_to_remove = args[i];

            if (!isEnvExistsInProc(var_to_remove)) {
                std::cerr << "smash error: unsetenv: " << var_to_remove << " does not exist\n";
                break; 
            }
            removeEnvFromGlobalArray(var_to_remove);
        }
    }

    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line) {}

void RedirectionCommand::execute() {
    string cmd_s = _trim(string(cmd_line));
    bool is_append = false;
    
    size_t pos = cmd_s.find(">>");
    if (pos != string::npos) {
        is_append = true;
    } else {
        pos = cmd_s.find(">");
    }

    if (pos == string::npos) return;

    string command_str = _trim(cmd_s.substr(0, pos));
    string file_str = _trim(cmd_s.substr(pos + (is_append ? 2 : 1)));

    if (!file_str.empty() && file_str.back() == '&') {
        file_str.pop_back();
        file_str = _trim(file_str);
    }

    int fd;
    if (is_append) {
        fd = open(file_str.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
    } else {
        fd = open(file_str.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    }

    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    int stdout_fd = dup(1);
    if (stdout_fd == -1) {
        perror("smash error: dup failed");
        close(fd);
        return;
    }

    if (dup2(fd, 1) == -1) {
        perror("smash error: dup2 failed");
        close(fd);
        close(stdout_fd);
        return;
    }

    SmallShell& smash = SmallShell::getInstance();
    Command* inner_cmd = smash.CreateCommand(command_str.c_str());
    if (inner_cmd) {
        inner_cmd->execute();
        delete inner_cmd;
    }

    close(fd);
    if (dup2(stdout_fd, 1) == -1) {
        perror("smash error: dup2 failed");
    }
    close(stdout_fd);
}