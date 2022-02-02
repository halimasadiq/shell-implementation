# Shell Implementation in C
The implementation runs on Linux and performs similar commands to Linux commands such as foreground a job, background a job, change directory, execute programs, kill a process, list all current jobs and more. 

## Usage:
1. To run a program in background using either relative or absolute path, enter '&' at the end of the program name:
   ./hello& or ./hello &
   The spaces after the end of the executable name and '&' can vary.
2. To run a suspended job in background:
   **bg** <*jobID*>
3. To change directory:
   **cd** <*path*>
4. To list all jobs:
   **jobs**
5. To foreground a background job or suspended job:
   **fg** <*jobID*>
6. To kill a job by sending the signal SIGTERM:
   **kill** <*jobID*>
7. To exit the program:
   - **exit**
   - Or press ctrl-d
8. To suspend a running job:
   Press ctrl-z
9. To terminate a running job:
   Press ctrl-c
