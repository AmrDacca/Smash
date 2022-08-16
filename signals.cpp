#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";
string Signal_ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string Signal_rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string Signal_trim(const std::string &s) {
    return Signal_rtrim(Signal_ltrim(s));
}
int Signal_parseCommandLineString(string cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(Signal_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

void ctrlZHandler(int sig_num) {

    SmallShell& smash = SmallShell::getInstance();

    cout<<"smash: got ctrl-Z"<<endl;

    if(smash.current_runnign_pid != 0){
        if (kill(smash.current_runnign_pid, SIGTSTP) == -1)
            perror("smash error: Kill failed");
        cout<<"smash: process "+to_string(smash.current_runnign_pid)+" was stopped"<<endl;
        bool isTimeOut = false;
        string cmd_line_string(smash.curre_job_cmd_line);
        //Parsing the command line and putting it to a vector of strings.
        char **cmd_line_args = (char **) malloc(sizeof(char *) * 80);
        int cmd_line_length = Signal_parseCommandLineString(cmd_line_string, cmd_line_args);
        string firstWord = string(cmd_line_args[0]);
        int duration = -1;
        if (firstWord =="timeout"){
            isTimeOut = true;
            duration = atoi(cmd_line_args[1]);
        }
        smash.addJobSmashZ(smash.curre_job_cmd_line, smash.current_runnign_pid, true ,isTimeOut,duration );
        smash.deleteRunningInfo();
    }

}

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();

    cout<<"smash: got ctrl-C"<<endl;
    if(smash.current_runnign_pid >0){
        cout<<"smash: process "+to_string(smash.current_runnign_pid)+" was killed"<<endl;
        kill(SmallShell::getInstance().current_runnign_pid,SIGKILL);
        smash.deleteRunningInfo();

    }}

void alarmHandler(int sig_num) {

    SmallShell& smash = SmallShell::getInstance();

    smash.removeFinishedJobsSmash();
    const JobsList::JobEntry *entryToDelete= nullptr;
    cout<<"smash: got an alarm"<< endl;
    for(auto ite=smash.jobsList.jobs.begin() ; ite != smash.jobsList.jobs.end() ; ){
        if(ite->isTimed && ite->duration-difftime(time(NULL),ite->starting_time)==0 ) {
            entryToDelete = &*ite;
            if(kill(ite->pid,SIGKILL)==-1) {
                perror("smash error: kill failed");
            }
            cout<<"smash: "+ite->cmd+" timed out!"<<endl;

            smash.jobsList.jobs.erase(ite);
            entryToDelete = nullptr;
        } else {
            ite++;
        }
    }
    alarm(smash.getMinTimeToFinishSmash());

    bool isTimeOut = false;
    string cmd_line_string(smash.curre_job_cmd_line);
    if (smash.curre_job_cmd_line == ""){
        if(smash.jobsList.jobs.size()>0) {
            alarm(smash.getMinTimeToFinishSmash());
        }
        return;
    }
    //Parsing the command line and putting it to a vector of strings.
    char **cmd_line_args = (char **) malloc(sizeof(char *) * 80);
    int cmd_line_length = Signal_parseCommandLineString(cmd_line_string, cmd_line_args);
    string firstWord = string(cmd_line_args[0]);
    int duration = -1;
    if (firstWord =="timeout"){
        isTimeOut = true;
        duration = atoi(cmd_line_args[1]);
        alarm(min((duration-difftime(time(NULL),smash.current_start_time)), (double)smash.getMinTimeToFinishSmash()));

    }
    if(isTimeOut && (duration-difftime(time(NULL),smash.current_start_time)==0) ) {
        if(kill(smash.current_runnign_pid,SIGKILL)==-1) {
            perror("smash error: kill failed");
        }
        cout<<"smash: "+smash.curre_job_cmd_line+" timed out!"<<endl;
        smash.current_runnign_pid=0;
        if(smash.jobsList.jobs.size()>0) {
            alarm(smash.getMinTimeToFinishSmash());
        }


    }






}
