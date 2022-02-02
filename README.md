# Shell Implementation in C
The implementation runs on Linux and performs similar commands to Linux commands such as foreground a job, background a job, change directory, execute programs, kill a process, list all current jobs and more. 

## Usage:
1. To run a program in background using either relative or absolute path, enter '&' at the end of the program name: <br />
   ./hello& or ./hello & <br />
   The spaces after the end of the executable name and '&' can vary. <br />
2. To run a suspended job in background:<br />
   **bg** <*jobID*><br />
3. To change directory:<br />
   **cd** <*path*><br />
4. To list all jobs:<br />
   **jobs**<br />
5. To foreground a background job or suspended job:<br />
   **fg** <*jobID*><br />
6. To kill a job by sending the signal SIGTERM:<br />
   **kill** <*jobID*><br />
7. To exit the program:<br />
   - **exit**<br />
   - Or press ctrl-d<br />
8. To suspend a running job:<br />
   Press ctrl-z<br />
9. To terminate a running job:<br />
   Press ctrl-c<br />
