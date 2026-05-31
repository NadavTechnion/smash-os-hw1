// Ver: 04-11-2025
#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <vector>
#include <string>
#include <map>
#include <fcntl.h>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
class Command {
    // TODO: Add your data members
protected:
    std::string cmd_line;

public:
    Command(const char *cmd_line);
    std::string getCmdLine() {
        return cmd_line;
    }
    virtual ~Command();

    virtual void execute() = 0;

    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};




class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {
    }
};

class ExternalCommand : public Command {
    int p_id;

public:
    ExternalCommand(const char *cmd_line,int p_id);
    int getPid() const {
        return p_id;
    }
    virtual ~ExternalCommand() {
    }

    void execute() override;
};

class RedirectionCommand : public Command {
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {
    }

    void execute() override;
};

class PipeCommand : public Command {
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {
    }

    void execute() override;
};

class DiskUsageCommand : public Command {
public:
    DiskUsageCommand(const char *cmd_line);

    virtual ~DiskUsageCommand() {
    }

    void execute() override;
};

class WhoAmICommand : public Command {
public:
    WhoAmICommand(const char *cmd_line);

    virtual ~WhoAmICommand() {
    }

    void execute() override;
};

class USBInfoCommand : public Command {
    // TODO: Add your data members **BONUS: 10 Points**
public:
    USBInfoCommand(const char *cmd_line);

    virtual ~USBInfoCommand() {
    }

    void execute() override;
};
class ChpromptCommand : public BuiltInCommand{
    public:
    ChpromptCommand(const char *cmd_line);

    virtual ~ChpromptCommand() {
    }

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
    // TODO: Add your data members public:
    public:
    ChangeDirCommand(const char *cmd_line);
    virtual ~ChangeDirCommand() {
    }

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {
    }

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {
    }

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {

    // TODO: Add your data members public:
public:
    QuitCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~QuitCommand() {
    }

    void execute() override;
};
class JobsList {
public:
    void printJobs();



    class JobEntry {
        int job_id;
        int p_id;
        std::string cmd_line;
    bool is_stopped;
    public:
        JobEntry(int job_id , int p_id,std::string cmd_line , bool is_stopped):
        job_id(job_id) , p_id(p_id),cmd_line(cmd_line),is_stopped(is_stopped){}
        ~JobEntry(){}
        int getJobId() const { return job_id; }
        int getPid() const { return p_id; }
        std::string getCmdLine() const { return cmd_line; }
        bool getIsStopped() const { return is_stopped; }
        std::string info();
        bool find();
    };
private:
    std::map<int, JobEntry> jobs;

    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    JobEntry *getFirstJob();

    JobEntry *getLastJob();

    void printAndKillAllJobs();

    int size();

    void removeJobById(int jobId);

    JobEntry *getfirstJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {};

    virtual ~JobsCommand() {
    }

    void execute() override;
};

class KillCommand : public BuiltInCommand {
public:
    KillCommand(const char *cmd_line);

    virtual ~KillCommand() {
    }

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
public:
    ForegroundCommand(const char *cmd_line);

    virtual ~ForegroundCommand() {
    }

    void execute() override;
};

class AliasCommand : public BuiltInCommand {
public:
    AliasCommand(const char *cmd_line);

    virtual ~AliasCommand() {
    }

    void execute() override;
};

class UnAliasCommand : public BuiltInCommand {
public:
    UnAliasCommand(const char *cmd_line);

    virtual ~UnAliasCommand() {
    }

    void execute() override;
};

class UnSetEnvCommand : public BuiltInCommand {
    bool isEnvExistsInProc(const std::string& var_name);
    void removeEnvFromGlobalArray(const std::string& var_name);
public:
    UnSetEnvCommand(const char *cmd_line);

    virtual ~UnSetEnvCommand() {
    }

    void execute() override;
};

class SysInfoCommand : public BuiltInCommand {
public:
    SysInfoCommand(const char *cmd_line);

    virtual ~SysInfoCommand() {
    }

    void execute() override;
};

class SmallShell {
private:
    // TODO: Add your data members
    JobsList jobManager;
    std::string name;
    std::string lastPwd;
    std::vector<std::pair<std::string, std::string>> aliases;
    SmallShell();

public:

    Command *CreateCommand(const char *cmd_line);
    std::string getName(){
        return name;
    }
    JobsList& getJobManager() { return jobManager; }
    void setName(const std::string n){
        name = n;
    }
    // ToDo: move implementaion of alias helper funcs to cpp
    void addAlias(const std::string& name, const std::string& cmd) {
        aliases.push_back({name, cmd});
    }

    void removeAlias(const std::string& name) {
        for (auto it = aliases.begin(); it != aliases.end(); ++it) {
            if (it->first == name) {
                aliases.erase(it);
                return;
            }
        }
    }

    bool isAliasExists(const std::string& name) const {
        for (const auto& alias : aliases) {
            if (alias.first == name) {
                return true;
            }
        }
        return false;
    }

    std::string getAliasCommand(const std::string& name) const {
        for (const auto& alias : aliases) {
            if (alias.first == name) {
                return alias.second;
            }
        }
        return ""; 
    }

    const std::vector<std::pair<std::string, std::string>>& getAliases() const { 
        return aliases; 
    }

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    std::string getLastPwd() { return lastPwd; }
    void setLastPwd(const std::string& pwd) { lastPwd = pwd; }
};

#endif //SMASH_COMMAND_H_
