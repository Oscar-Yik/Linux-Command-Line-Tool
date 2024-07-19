# Linux-Command-Line-Tool

## How to Use Program 
- run the executable file `crash` using `./crash` in a UNIX command line after cloning the repository
- you can run all basic Linux commands like `ls`, `grep`, `cd`, and more 
- you can display, kill, and suspend currently running processes  
- all started processes are given a process id and job ID
- commands only work on processes started from this program
- processes can be started in either the foreground or background 
- the shell is inaccessible when processes in the foreground are running 
- commands are delimited by `&` and `;`
- commands delimited by `;` run in the foreground while those delimited by `&` run in the background

# Demo 

https://github.com/Oscar-Yik/Linux-Command-Line-Tool/blob/main/assets/Linux_Command_Line_Tool_Demo.mp4

## Commands Run in the Demo
1. `ls -l`
2. `sleep 4& sleep 4&`, `jobs`
3. `sleep 8`, `CTRL+Z`, `jobs`, `fg %<jobID>`
4. `sleep 14& sleep 13& sleep 16& sleep 10&`, `nuke`
5. `quit`

## List of Commands 
- `foo` runs any arbitrary program foo
- `jobs` displays all active processes
- `nuke` kills running processes; accepts both job IDs and process IDs as arguments
- `fg` moves process to foreground or resumes if suspended; accepts both job IDs and process IDs as arguments
- `CTRL+Z` suspends currently running foreground process 
- `CTRL+C` kills the foreground process 
- `CTRL+D` exits the program if there is no foreground process
- `quit` exits the program 

## Command Examples 
- `foo bar baz` runs program `foo` with arguments `bar` and `baz` in the foreground
- `foo bar bax &` runs program `foo` with arguments `bar` and `baz` in the background
- `foo ; ls -l ;` runs the program `foo` in the foreground, waits until it finishes and then runs `ls` with argument `-l`

- `nuke` kills all running processes
- `nuke 12345` kills process with ID 12345 if it hasn't already terminated 
- `nuke %7` kills process with jobID 7 

- `fg 12345` moves process with ID 12345 to the foreground or resumes it if suspended 
- `fg %7` moves process with job ID 12345 to the foreground or resumes it if suspended 

- `nuke` may take multiple arguments which are process IDs or job IDs i.e. `nuke %89 1627 951 %812`
environment.