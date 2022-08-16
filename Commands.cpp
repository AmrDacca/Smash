#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>
#include <utime.h>


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
const string command_tokens[] = {
        "chprompt",
        "showpid",
        "pwd",
        "cd",
        "jobs",
        "kill",
        "fg",
        "bg",
        "quit",
        "tail",
        "touch",
        "timeout"
};

enum COMMAND_E {
    ch = 0,
    showpid,
    pwd,
    cd,
    jobs,
    kil,
    fg,
    bg,
    quit,
    tail,
    touch,
    timeout,
    dne
};


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

//--------------------------------------------------//--------------------------------------------------
//Duplicates added for const case

int _parseCommandLineString(string cmd_line, char **args) {
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

void _removeBackgroundSignString(string& cmd_line) {
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

//--------------------------------------------------//--------------------------------------------------


bool isNumber(const char *num) {
    unsigned int i = 0;
    if (num[0] == '-')
        i++;
    for (; i < strlen(num); ++i) {
        if (!isdigit(num[i]))
            return false;
    }
    return true;
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

COMMAND_E getCommandType(string firstword) {
    for (int i = 0; i < 12; ++i) {
        if (firstword == command_tokens[i]) {
            return (COMMAND_E) i;
        }
    }
    return dne;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell() {
    chprompt = "smash";
    last_dir = "";
    smashPID = getpid();
    current_runnign_pid = 0;
    current_jov_id = -1;
    curre_job_cmd_line = "";
    current_start_time = 0;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_line_string(cmd_line);
    //Checking if command should be run in backGround!
    bool isBackGround = _isBackgroundComamnd(cmd_line);
    if (isBackGround)
        _removeBackgroundSignString(cmd_line_string);
    //Parsing the command line and putting it to a vector of strings.
    char **cmd_line_args = (char **) malloc(sizeof(char *) * 80);
    int cmd_line_length = _parseCommandLineString(cmd_line_string, cmd_line_args);
    string firstWord = string(cmd_line_args[0]);
    //Pipe case
    if (cmd_line_string.find("|&") != std::string::npos) {
        return new PipeCommandError(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
    }
    if (cmd_line_string.find("|")!=std::string::npos) {
        return new PipeCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
    }
    if (cmd_line_string.find(">")!= string::npos) {
        return new RedirectionCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
    }
    switch (getCommandType(firstWord)) {
        case ch: {
            return new ChangePromptCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case showpid: {
            return new ShowPidCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case pwd: {
            return new GetCurrDirCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case cd: {
            return new ChangeDirCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case jobs: {
            return new JobsCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case kil: {
            return new KillCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case fg: {
            return new ForegroundCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case bg: {
            return new BackgroundCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case quit: {
            return new QuitCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case tail: {
            //return new ExternalCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length, 0, false, -1);
            return new TailCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case touch: {
            return new TouchCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
        }
        case timeout: {//TODO timeout Bonus
            //    return new TimeoutCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length);
            if (cmd_line_length < 3 || !isNumber(cmd_line_args[1])) {
                cerr << "smash error: timeout: invalid arguments" << endl;
                return nullptr;
            }
            return new ExternalCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length, 0, true,
                                       atoi(cmd_line_args[1]));

        }
        default: {//Pid will be assigned later in exec

            return new ExternalCommand(cmd_line, cmd_line_args, isBackGround, cmd_line_length, 0, false, -1);
        }

    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    SmallShell &smash = SmallShell::getInstance();
    smash.removeFinishedJobsSmash();
   // jobsList.removeFinishedJobs();
    Command *cmd = CreateCommand(cmd_line);
    if (cmd == nullptr)
        return;
    cmd->execute();
    delete (cmd);
}

void ExternalCommand::execute() {
    string innerCommand, orig_cmd = cmd_string;
    if (isTimed) {
        if (cmd_line_length < 3 || !isNumber(parsed_cmd_line[1])) {
            cerr << "smash error: timeout: invalid arguments" << endl;
            return;

        }

        if (duration < 0) {
            cerr << "smash error: timeout: invalid arguments" << endl;
            return;
        }

        for (int i = 2; i < cmd_line_length; i++)
            innerCommand += " " + string(parsed_cmd_line[i]);
    }
    std::string cmd_line_string(cmd_string);
    if (isBackGround) {
        _removeBackgroundSignString(cmd_line_string);
    }
    if (isTimed) {
        cmd_line_string = innerCommand;
    }

    int fork_pid = fork();

    if (fork_pid == -1) {
        perror("smash error: fork failed");
    }
    if (fork_pid == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }

        if (execl("/bin/bash", "/bin/bash", "-c", cmd_line_string.c_str(), NULL) == -1) {
            perror("smash error: execv failed");
        }
    } else {
        if (isBackGround) {
            SmallShell::getInstance().addJobSmash(orig_cmd, fork_pid, false, isTimed, duration);
            alarm(SmallShell::getInstance().getMinTimeToFinishSmash());
        } else {
            //running in fg
            SmallShell::getInstance().setRunningInfo(fork_pid, -1, orig_cmd, time(NULL));
            if (isTimed) {
                int min_duration = min(duration, SmallShell::getInstance().getMinTimeToFinishSmash());
                alarm(min_duration);
            }
                if (waitpid(fork_pid, nullptr, WUNTRACED) == -1) {
                    perror("smash error: waitpid failed");
                    return;
                }


            SmallShell::getInstance().setRunningInfo(0, -1, "", 0);
        }
    }
}

vector<string> split(std::string str, char delim) {
    std::vector<std::string> result;
    std::istringstream ss{str};
    std::string token;
    while (std::getline(ss, token, delim)) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

void PipeCommand::execute() {
    vector<string> commands = split(cmd_string.c_str(), '|');
    SmallShell &smash = SmallShell::getInstance();


    int fd[2];


    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    int firstChild = fork();

    if (firstChild == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (firstChild == 0) {

        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }
        // first child - stdout

        if (dup2(fd[1], 1) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }

        char *cmd_line_c = new char[commands[0].length() + 1];
        strcpy(cmd_line_c, commands[0].c_str());
        Command *cmd = smash.CreateCommand(const_cast<char *>(cmd_line_c));


        cmd->execute();
        delete[] cmd_line_c;
        delete cmd;

        exit(0);
    }

    int secondChild = fork();

    if (secondChild == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (secondChild == 0) {

        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }
        // second child

        if (dup2(fd[0], 0) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }


        char *cmd_line_c = new char[commands[1].length() + 1];
        strcpy(cmd_line_c, commands[1].c_str());
        Command *cmd = smash.CreateCommand(const_cast<char *>(cmd_line_c));


        cmd->execute();
        delete[] cmd_line_c;
        delete cmd;


        exit(0);
    }

    if (close(fd[0]) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (close(fd[1]) == -1) {
        perror("smash error: close failed");
        return;
    }

    if (waitpid(firstChild, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }


    if (waitpid(secondChild, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }

    smash.deleteRunningInfo();
}

void RedirectionCommand::execute() {
    string innerCommand, outerCommand;
    int flags = O_RDWR | O_CREAT;

    int i;
    for (i = 0; i < cmd_line_length; i++) {
        if (std::string(parsed_cmd_line[i]).find(">>") != string::npos) {
            if (std::string(parsed_cmd_line[i]).size() != 2){
                innerCommand += " " + string(parsed_cmd_line[i]).substr(0, string(parsed_cmd_line[i]).find_first_of(">>"));
                if (string(parsed_cmd_line[i]).size() > string(parsed_cmd_line[i]).find_last_of(">>")+1){
                    outerCommand+= " " + string(parsed_cmd_line[i]).substr(string(parsed_cmd_line[i]).find_last_of(">>")+1, string(parsed_cmd_line[i]).size());
                }
            }
            flags = flags | O_APPEND;
            i++;
            break;
        } else if (std::string(parsed_cmd_line[i]).find('>')  != string::npos) {
            if (std::string(parsed_cmd_line[i]).size() != 1){
                innerCommand += " " + string(parsed_cmd_line[i]).substr(0, string(parsed_cmd_line[i]).find_first_of('>'));
                if (string(parsed_cmd_line[i]).size() > string(parsed_cmd_line[i]).find_last_of('>')+1){
                    outerCommand+= " " + string(parsed_cmd_line[i]).substr(string(parsed_cmd_line[i]).find_last_of('>')+1, string(parsed_cmd_line[i]).size());
                }
            }
            flags = flags | O_TRUNC;
            i++;
            break;
        }
        innerCommand += " " + string(parsed_cmd_line[i]);
    }
    for (; i < cmd_line_length; i++) {
        outerCommand += " " + string(parsed_cmd_line[i]);
    }

    outerCommand.erase(remove(outerCommand.begin(), outerCommand.end(), ' '), outerCommand.end());

    int fileFd = open(outerCommand.c_str(), flags, S_IRUSR | S_IWUSR | S_IXGRP | S_IRGRP | S_IROTH | S_IXOTH);

    if (fileFd == -1) {
        perror("smash error: open failed");
        return;
    }

    int saveStdOut = dup(1);
    if (saveStdOut == -1) {
        perror("smash error: dup failed");
        return;
    }

    if (close(1) == -1) {
        perror("smash error: close failed");
        return;
    }

    if (dup2(fileFd, 1) == -1) {
        perror("smash error: dup failed");
        return;
    }

    SmallShell::getInstance().executeCommand(const_cast<char *>(innerCommand.c_str()));

    if (close(1) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (dup(saveStdOut) == -1) {
        perror("smash error: dup failed");
        return;
    }
    if (close(saveStdOut) == -1) {
        perror("smash error: close failed");
        return;
    }

}

void PipeCommandError::execute() {
    vector<string> zebi = split(cmd_string.c_str(), '|');
    vector<string> ere = split(zebi[1], '&');
    vector<string> commands;
    commands.push_back(zebi[0]);
    commands.push_back(ere[0]);
    SmallShell &smash = SmallShell::getInstance();


    int fd[2];


    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    int firstChild = fork();

    if (firstChild == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (firstChild == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }
        // first child - stdout

        if (dup2(fd[1], 2) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }


        char *cmd_line_c = new char[commands[0].length() + 1];
        strcpy(cmd_line_c, commands[0].c_str());


        Command *cmd = smash.CreateCommand(const_cast<char *>(cmd_line_c));


        cmd->execute();
        delete[] cmd_line_c;
        delete cmd;

        exit(0);
    }

    int secondChild = fork();

    if (secondChild == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (secondChild == 0) {

        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            return;
        }
        // second child

        if (dup2(fd[0], 0) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd[0]) == -1) {
            perror("smash error: close failed");
            return;
        }
        if (close(fd[1]) == -1) {
            perror("smash error: close failed");
            return;
        }


        char *cmd_line_c = new char[commands[1].length() + 1];
        strcpy(cmd_line_c, commands[1].c_str());

        Command *cmd = smash.CreateCommand(const_cast<char *>(cmd_line_c));


        cmd->execute();
        delete[] cmd_line_c;
        delete cmd;


        exit(0);
    }

    if (close(fd[0]) == -1) {
        perror("smash error: close failed");
        return;
    }
    if (close(fd[1]) == -1) {
        perror("smash error: close failed");
        return;
    }

    if (waitpid(firstChild, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }


    if (waitpid(secondChild, nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }

    smash.deleteRunningInfo();
}



void TouchCommand::execute() {
    if (cmd_line_length != 3 ){
        cerr << "smash error: touch: invalid arguments" << endl;
        return;
    }
    vector<string> timeSplit = split(string (parsed_cmd_line[2]), ':');
    for (unsigned int i = 0; i < timeSplit.size(); ++i) {
        if (!isNumber(timeSplit[i].c_str())){
            cerr << "smash error: touch: invalid arguments" << endl;
            return;
        }//touch adi 56:48:20:12:8:1905
    }
    struct tm adi;
    adi.tm_sec = atoi(timeSplit[0].c_str());
    adi.tm_min = atoi(timeSplit[1].c_str());
    adi.tm_hour = atoi(timeSplit[2].c_str());
    adi.tm_mday = atoi(timeSplit[3].c_str());
    adi.tm_mon = atoi(timeSplit[4].c_str())-1;
    adi.tm_year = atoi(timeSplit[5].c_str())-1900;

    struct utimbuf amr;
    time_t fadi = mktime(& adi);
    if (fadi == -1){
        perror("smash error: mktime failed");
        return;
    }
    amr.actime = fadi;
    amr.modtime = fadi;
    string pathname = string (parsed_cmd_line[1]);
    pathname.erase(remove(pathname.begin(), pathname.end(), ' '), pathname.end());


    if (utime(pathname.c_str(), &amr)!=0){
        perror("smash error: utime failed");
        return;
    }
}
