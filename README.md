# Smash

This project was implemented during the course Operating Systems in of which we were asked to implement a working like shell with changes to some functionality of some commands of the shell and keeping others to work as is



# How To Run

 1.download all the files


2. complie using the make command


3. run the command ./smash


# The changes made to the shell

fg command  usage:
fg [job-id]
job-id is optional if not specified it will run the max job id to continue running in the foreground

bg command usage:
bg [job-id]
job-id is optional if not specified it will run the last stopped job to continue running in the background

added command:
quit [kill]
this command will terminate the shell from running
kill is optional if specified it will kill all jobs (running/stopped in bg) and prints the number of jobs killed)

tail command:
tail [-N] [file]
-N is optional if not givn number N the shell will print as default the last 10 lines of a file (if it exists)

kill command:
kill -<signum> [job-id]

touch command:
touch <file-name> <timestamp>
touch command receives 2 arguments: <file-name> that describes the relative or full path to a file, and <timestamp> that contains time in the following format: ss:mm:hh:dd:mm:yyyy
This command will update the file’s last access and modification timestamps to be the time specified in the <timestamp> argument.
and assuming timestamp is correct
