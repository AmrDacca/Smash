#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
class Command;
#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_JOBS (100)
#define MAX_PROCESS_NAME (50)

//--------------------------------------------------//--------------------------------------------------
/*
 * Global Variable section :
 * 1) chprompt made global in order to use it in smash
 */
//extern std::string chprompt;
//extern std::string last_dir;

//--------------------------------------------------//--------------------------------------------------


//--------------------------------------------------//--------------------------------------------------
class SmallShell;
class JobsList {
public:
    class JobEntry {
    public:
        std::string cmd;
        pid_t pid;
        time_t starting_time;
        int JobId;
        bool isStopped;
        bool isTimed;
        int duration;
        JobEntry(std::string cmd, pid_t pid, time_t starting_time, int JobId, bool isStopped = false,bool isTimed = false,int duration = - 1) : cmd(cmd), pid(pid),
                                                                                                                                                starting_time(starting_time), JobId(JobId), isStopped(isStopped) , isTimed(isTimed),duration(duration){}

        ~JobEntry() = default;
        bool operator<(JobEntry job2){
            return JobId < job2.JobId;
        }
        bool operator==(JobEntry job2){
            return JobId == job2.JobId;
        }
        bool operator>(JobEntry job2){
            return JobId > job2.JobId;
        }
    };

public:
    std::vector<JobEntry> jobs;
public:
    JobsList() : jobs(std::vector<JobEntry>()){}
    ~JobsList()=default;
    void addJob(std::string cmd, pid_t pid, bool isStopped = false ,bool isTimed = false ,int duration = -1 ){
//        removeFinishedJobs();
        int max = 0;
        for(auto it : jobs){
            max = (max > it.JobId)*max + (it.JobId >= max)*it.JobId;
        }
        JobEntry new_job = JobEntry(cmd, pid, time(nullptr), max+1, isStopped,isTimed,duration);
        jobs.push_back(new_job);
    }
    void printJobsList(){
//        removeFinishedJobs();
        std::vector<JobEntry> tmp_jobs(jobs);
        std::sort(tmp_jobs.begin(),tmp_jobs.end());
        for(auto it : tmp_jobs){
            //[<job-id>] <command> : <process id> <seconds elapsed>
            std::cout << "[" << it.JobId << "]" << " " << it.cmd << " : " << it.pid << " " << difftime(time(
                    nullptr),it.starting_time) << " secs";
            if (it.isStopped){
                std::cout << " (stopped)";
            }
            std::cout << std::endl;
        }
    }
    void killAllJobs(){
//        removeFinishedJobs();

        std::vector<JobEntry> tmp_jobs(jobs);
        std::sort(tmp_jobs.begin(),tmp_jobs.end());
        for(auto it : tmp_jobs){
            if (kill(it.pid, SIGKILL) == -1){
                perror("smash error: kill failed");
            }
            std::cout << it.pid<< ": " << it.cmd<< std::endl;
        }
    }
    void removeFinishedJobs(pid_t smash_pid){//TODO Check this function for edge cases
        if (getpid() != smash_pid){
            return;
        }
        int status;
        JobsList::JobEntry *entryToDelete = nullptr;
        for(auto ite=jobs.begin() ; ite != jobs.end() ; ){
//            if (entryToDelete != nullptr){
//                jobs.erase(ite);
//                entryToDelete = nullptr;
//            }
            pid_t result = waitpid(ite->pid, &status, WNOHANG);
            if (  result == -1)
            {
                perror("smash error: waitpid failed");//TODO PROBLEM!!!!!
                return;
            }
            if (result > 0){
                entryToDelete = &*ite;
            }
            if (entryToDelete != nullptr) {
                jobs.erase(ite);
                entryToDelete = nullptr;
            } else {
                ite++;
            }
        }

    }
    JobEntry * getJobById(int jobId){
 //       removeFinishedJobs();
        for(auto ite=jobs.begin() ; ite != jobs.end() ; ite++ ){
            if (ite->JobId == jobId) {
                return &*ite;
            }
        }
        return nullptr;
    }
    void removeJobById(int jobId){
        for(auto ite = jobs.begin() ; ite != jobs.end(); ite++) {
            if (ite->JobId == jobId) {
                jobs.erase(ite);
                return;
            }
        }
    }
    JobEntry * getLastJob(int* lastJobId){
     //   removeFinishedJobs();
        return &jobs[jobs.size()-1];
    }
    JobEntry *getLastStoppedJob(int *jobId){
   //     removeFinishedJobs();
        for (auto ite = jobs.end()-1 ; ite >= jobs.begin(); ite--) {
            if (ite->isStopped){
                return &(*(ite));//first cast ite to job entry and then return a pointer to it
            }
        }
    }
    JobEntry* getMaxJob(){
       // removeFinishedJobs();
        int max = -1;
        JobEntry* tmp = nullptr;
        for(auto ite = jobs.begin() ; ite != jobs.end(); ite++) {
            if (ite->JobId > max) {
                tmp = &*ite;
                max = ite->JobId;
            }
        }
        return tmp;
    }
    JobEntry* getMaxJobStopped(){
      //  removeFinishedJobs();
        int max = -1;
        JobEntry* tmp = nullptr;
        for(auto ite = jobs.begin() ; ite != jobs.end(); ite++) {
            if (ite->isStopped && ite->JobId > max) {
                tmp = &*ite;
                max = ite->JobId;
            }
        }
        return tmp;
    }

    int getMinTimeToFinish(){
      //  removeFinishedJobs();
        int minTimeToFinsih = std::numeric_limits<int>::max();
        for(auto ite = jobs.begin() ; ite != jobs.end(); ite++) {
            if (ite->isTimed) {
                if (ite->duration - difftime(time(NULL), ite->starting_time) < minTimeToFinsih)
                    minTimeToFinsih = ite->duration - difftime(time(NULL), ite->starting_time);
            }
        }
        return minTimeToFinsih;
    }
    // TODO: Add extra methods or modify exisitng ones as needed
};

//--------------------------------------------------//--------------------------------------------------


class SmallShell {
public:
    std::string chprompt;
    std::string last_dir;
    JobsList jobsList;
   // JobsList alarmedJobs;
    pid_t smashPID;

    JobsList::JobEntry* afterFg;

    pid_t current_runnign_pid = 0;
    int current_jov_id = -1;
    std::string curre_job_cmd_line;
    time_t current_start_time;

private:
    SmallShell();
public:
    SmallShell(SmallShell &)      = delete; // disable copy ctor
    void operator=(SmallShell &)  = delete; // disable = operator
    Command *CreateCommand( const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    void setRunningInfo(pid_t current_runnign_pid, int current_jov_id, std::string curre_job_cmd_line,
                        time_t current_start_time){
        this->current_runnign_pid = current_runnign_pid;
        this->current_jov_id = current_jov_id;
        this->curre_job_cmd_line = curre_job_cmd_line;
        this->current_start_time = current_start_time;
    }
    void deleteRunningInfo(){
        this->current_runnign_pid = 0;
        this->current_jov_id = -1;
        this->curre_job_cmd_line = "";
        this->current_start_time = 0;
    }
    void printJobListSmash(){
        SmallShell::getInstance().jobsList.removeFinishedJobs(smashPID);
        SmallShell::getInstance().jobsList.printJobsList();
    }
    void killAllJobsSmash(){
        SmallShell::getInstance().removeFinishedJobsSmash();
        SmallShell::getInstance().jobsList.killAllJobs();
    }
    void removeFinishedJobsSmash(){
        SmallShell::getInstance().jobsList.removeFinishedJobs(smashPID);
    }
    int getMinTimeToFinishSmash(){
        SmallShell::getInstance().removeFinishedJobsSmash();
        return SmallShell::getInstance().jobsList.getMinTimeToFinish();
    }
    JobsList::JobEntry* getMaxJobSmash(){
        SmallShell::getInstance().removeFinishedJobsSmash();

        return SmallShell::getInstance().jobsList.getMaxJob();
    }
    JobsList::JobEntry* getMaxJobStoppedSmash(){

        SmallShell::getInstance().removeFinishedJobsSmash();

        return SmallShell::getInstance().jobsList.getMaxJobStopped();
    }
    void addJobSmashZ(std::string cmd, pid_t pid, bool isStopped = false ,bool isTimed = false ,int duration = -1 ){
        addJobSmash(cmd, pid, isStopped, isTimed, duration);
        JobsList::JobEntry* tmp =  SmallShell::getInstance().jobsList.getMaxJobStopped();
        if (current_runnign_pid != 0) {
            tmp->starting_time = current_start_time;
        }
        if (current_jov_id != -1){
            tmp->JobId = current_jov_id;
        }
    }
    void addJobSmash(std::string cmd, pid_t pid, bool isStopped = false ,bool isTimed = false ,int duration = -1 ){
        SmallShell::getInstance().removeFinishedJobsSmash();

        SmallShell::getInstance().jobsList.addJob(cmd, pid, isStopped, isTimed, duration);
    }
    void removeJobByIdSmash(int jobId){
        SmallShell::getInstance().removeFinishedJobsSmash();
        SmallShell::getInstance().jobsList.removeJobById(jobId);
        SmallShell::getInstance().removeFinishedJobsSmash();

    }
    };





class Command {
public:
    std::string cmd_string;
    char** parsed_cmd_line;
    bool isBackGround;
    int cmd_line_length;
public:
    Command(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length){
        cmd_string = cmd_line;
        this->parsed_cmd_line = parsed_cmd_line;
        this->isBackGround = isBackGround;
        this->cmd_line_length = cmd_line_length;

    }
    virtual ~Command() = default;
    virtual void execute() = 0;
//  std::string getCmdLine(){
//      return cmd_string;
//  }TODO check if u wanna do get and set
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : Command(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};

    virtual ~BuiltInCommand() = default;
};

class ExternalCommand : public Command {
public:
    pid_t pid;
    bool isTimed;
    int duration;
public:
    ExternalCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length, pid_t pid,bool isTimed,int duration)
            : Command(cmd_line, parsed_cmd_line, isBackGround, cmd_line_length), pid(pid) , isTimed(isTimed),duration(duration) {};
    virtual ~ExternalCommand() = default;
    void execute() override;
    pid_t getPid(){
        return pid;
    }
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : Command(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    virtual ~PipeCommand() {}
    void execute() override;
};

class PipeCommandError : public Command {
public:
    PipeCommandError(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : Command(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    virtual ~PipeCommandError() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : Command(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangePromptCommand : public BuiltInCommand { //CHPROMPT
public:
    explicit ChangePromptCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length) : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length) {}
    ~ChangePromptCommand() override {
        free(parsed_cmd_line);
    }
    void execute() override{
        SmallShell& smash = SmallShell::getInstance();
        if (cmd_line_length == 1){
            smash.chprompt = "smash";
            return;
        }
        smash.chprompt = parsed_cmd_line[1];
    }
};

class ChangeDirCommand : public BuiltInCommand {//CD
public:
    ChangeDirCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~ChangeDirCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        SmallShell& smash = SmallShell::getInstance();
        if (cmd_line_length > 2){
            std::cerr << "smash error: cd: too many arguments" <<std::endl;
            return;
        }
        if (std::string(parsed_cmd_line[1]) == "-"){
            if (smash.last_dir == ""){
                std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
                return;
            } else{
                std::string oldPath = smash.last_dir;
                smash.last_dir = std::string (get_current_dir_name());
                if (chdir(oldPath.c_str()) != 0) {
                    perror("smash error: chdir failed");
                    return;
                }
                return;
            }
        } else{
            std::string tmp_dir = get_current_dir_name();
            if (chdir(parsed_cmd_line[1]) != 0)
                perror("smash error: chdir failed");
            else
                smash.last_dir = tmp_dir;
        }
    }
};

class GetCurrDirCommand : public BuiltInCommand {//PWD
public:
    GetCurrDirCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~GetCurrDirCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        std::cout << get_current_dir_name() << std::endl;
    }
};

class ShowPidCommand : public BuiltInCommand {//PID
public:
    ShowPidCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~ShowPidCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        SmallShell& smash = SmallShell::getInstance();
        std::cout<< "smash pid is " << smash.smashPID << std::endl;
    }
};

class QuitCommand : public BuiltInCommand {//QUIT
public:
    QuitCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~QuitCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        SmallShell &smash = SmallShell::getInstance();

        if (cmd_line_length >= 2 && std::string (parsed_cmd_line[1]) == "kill") {
            std::cout << "smash: sending SIGKILL signal to " << smash.jobsList.jobs.size() <<  " jobs:" << std::endl;
            smash.killAllJobsSmash();
        }
        exit(0);
    }
};


class JobsCommand : public BuiltInCommand {//JOBS
public:
    JobsCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~JobsCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override {
        SmallShell& smash = SmallShell::getInstance();
        smash.printJobListSmash();
    }
};
extern bool isNumber (const char* num);
class KillCommand : public BuiltInCommand {//KILL
public:
    KillCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~KillCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        if (cmd_line_length != 3 || parsed_cmd_line[1][0] != '-' || !isNumber(parsed_cmd_line[1]) || !isNumber(parsed_cmd_line[2])){
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return;
        }
        int jobID = atoi(parsed_cmd_line[2]);
        JobsList::JobEntry* job = SmallShell::getInstance().jobsList.getJobById(jobID);
        if (!job){
            std::cerr << "smash error: kill: job-id " << jobID <<" does not exist" << std::endl;
            return;
        }
        if((-1*atoi(parsed_cmd_line[1])) < 1 || (-1*atoi(parsed_cmd_line[1])) > 64){
            std::cerr << "smash error: kill: invalid arguments" << std::endl;
            return;
        }
        if (kill(job->pid, -1*atoi(parsed_cmd_line[1])) == -1){
            perror("smash error: kill failed");
            return;
        } else{
            std::cout << "signal number " << -1* atoi(parsed_cmd_line[1]) <<" was sent to pid " << job->pid << std::endl;
        }
    }
};

class ForegroundCommand : public BuiltInCommand {//FG
public:
    ForegroundCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    virtual ~ForegroundCommand(){
        free(parsed_cmd_line);
    }
    void execute() override{
        SmallShell& smash = SmallShell::getInstance();
        JobsList::JobEntry* curr_job = nullptr;
        if (cmd_line_length > 2 || (cmd_line_length == 2 && !isNumber(parsed_cmd_line[1]))){
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            return;
        }
        else if (cmd_line_length == 1){
            if (smash.jobsList.jobs.empty()) {
                std::cerr << "smash error: fg: jobs list is empty" << std::endl;
                return;
            } else {
                curr_job = smash.getMaxJobSmash();
            }
        }
        else if (cmd_line_length == 2) {
            if (!smash.jobsList.getJobById(atoi(parsed_cmd_line[1]))) {
                std::cerr << "smash error: fg: job-id " << atoi(parsed_cmd_line[1]) << " does not exist" << std::endl;
                return;
            }
            curr_job = smash.jobsList.getJobById(atoi(parsed_cmd_line[1]));
        }
        if (curr_job->isStopped){
            if (kill(curr_job->pid, SIGCONT) == -1) {
                perror("smash error: Kill failed");
                return;
            }
        }
        smash.setRunningInfo(curr_job->pid, curr_job->JobId, curr_job->cmd, curr_job->starting_time);
        std::cout << curr_job->cmd << " : " << curr_job->pid << std::endl;
        smash.removeJobByIdSmash(curr_job->JobId);
        if (waitpid(smash.current_runnign_pid, nullptr, WUNTRACED) == -1){
            perror("smash error: waitpid failed");
            return;
        }
        smash.deleteRunningInfo();
    }
};

class BackgroundCommand : public BuiltInCommand {//BG
    // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~BackgroundCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        SmallShell &smash = SmallShell::getInstance();
        if (cmd_line_length > 2) {
            std::cerr<< "smash error: bg: invalid arguments" << std::endl;
            return;
        }
        int job_id = 0;
        if (cmd_line_length == 2) {
            if (!isNumber(parsed_cmd_line[1])) {
                std::cerr << "smash error: bg: invalid arguments" << std::endl;
                return;
            }
            job_id = atoi(parsed_cmd_line[1]);
        } else {
            if (!smash.getMaxJobStoppedSmash()) {
                std::cerr << "smash error: bg: there is no stopped jobs to resume" << std::endl;
                return;
            } else{
                job_id = smash.getMaxJobStoppedSmash()->JobId;
            }
        }
        JobsList::JobEntry *jobEntry = smash.jobsList.getJobById(job_id);
        if(jobEntry == nullptr){
            std::cerr<<"smash error: bg: job-id "+std::to_string(job_id)+" does not exist"<<std::endl;
            return;
        }

        if (!jobEntry->isStopped) {
            std::cerr << "smash error: bg: job-id " << job_id <<  " is already running in the background" << std::endl;
            return;
        }


        jobEntry->isStopped = false;
        std::cout << jobEntry->cmd + " : " << jobEntry->pid << std::endl;

        if (kill(jobEntry->pid, SIGCONT) == -1) {
            perror("smash error: Kill failed");
            return;
        }
    }
};

class TailCommand : public BuiltInCommand {//TAIL
public:
    TailCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length)
            : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~TailCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override{
        if(cmd_line_length > 3 || cmd_line_length < 2){
            std::cerr << "smash error: tail: invalid arguments" << std::endl;
            return;
        }
        // if number of lines is specifed
        if (cmd_line_length == 3){
            if(!isNumber(parsed_cmd_line[1])){
                std::cerr << "smash error: tail: invalid arguments" << std::endl;
                return;
            }
            long lines_to_print = atoi(parsed_cmd_line[1]);
            if(lines_to_print > 0){
                std::cerr << "smash error: tail: invalid arguments" << std::endl;
		return;
            }
            int fd = open(parsed_cmd_line[2],0444);
            if(fd == -1){
                perror("smash error: open failed");
                return;
            }

            //TODO CHECK NEGATIVE
            lines_to_print = lines_to_print*-1;
            char buffer = 1;
            unsigned long lines = 0;//number of lines in the files
            std::vector<std::string> vece(0);
            std::string tmp("");
            while (buffer){
                ssize_t x=read(fd,&buffer,1);
                if(x==-1){
                    perror("smash error: read failed");
                    return;
                }
                if(x==0){
                    if (tmp != ""){
                        vece.push_back(tmp);
                    }
                    lines++;
                    break;
                }
                tmp+=std::string(1,buffer);
                if(std::string(1,buffer) == "\n"){
                    lines++;
                    vece.push_back(tmp);
                    tmp = std::string("");
                }
            }
            lines = (lines <= lines_to_print)  ? 0 : vece.size()-lines_to_print;//could be -1()
            for (unsigned long i = lines; i < vece.size(); ++i) {
                std::cout << vece[i];
            }
            close(fd);
            return;
        }
        //print last 10 / all file is the file is samller that 10 lines of the file
        int fd = open(parsed_cmd_line[1],0444);

        if(fd == -1){
            perror("smash error: open failed");
            return;
        }
        char buffer = 1;
        unsigned long lines = 0;

        std::vector<std::string> vece(0);
        std::string tmp("");
        while (buffer){
            ssize_t x=read(fd,&buffer,1);
            if(x==-1){
                perror("smash error: read failed");
                return;
            }
            if(x==0){
                lines++;
                if (tmp != ""){
                    vece.push_back(tmp);
                }
                break;
            }
            tmp+=std::string(1,buffer);
            if(std::string(1,buffer) == "\n"){
                lines++;
                vece.push_back(tmp);
                tmp = std::string("");
            }
        }
        lines = (lines <= 10)  ? 0 : vece.size()-10;
        for (unsigned long i = lines; i < vece.size(); ++i) {
            std::cout << vece[i];
        }
        close(fd);
    };
};

class TouchCommand : public BuiltInCommand {//TOUCH
public:
    TouchCommand(const char* cmd_line, char** parsed_cmd_line, bool isBackGround, int cmd_line_length) : BuiltInCommand(cmd_line, parsed_cmd_line,isBackGround,cmd_line_length){};
    ~TouchCommand() override{
        free(parsed_cmd_line);
    }
    void execute() override;
};



#endif //SMASH_COMMAND_H_
