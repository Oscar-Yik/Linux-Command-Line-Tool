#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 1024

#define SIGPID SIGRTMIN + 1

typedef struct job {
    int jid;
    volatile pid_t pid; 
    char *name; // remember to free this when deleting a job
    struct job *next; 
    bool suspended; 
} job_t;

typedef struct {
    int curr_jid; 
    job_t *jobs_list; 
} job_list_t; 

job_list_t* jobs;
bool fg = false; 
bool flag_c = false;
bool flag_q = false;  
bool flag_z = false; 

job_t *get_last_job(job_t *head) {
    while(head->next) {
        head = head->next; 
    }
    return head; 
}

void add_job(job_list_t *jobs, const char *name, pid_t pid){
    job_t *new_job = malloc(sizeof(job_t)); 
    assert(new_job);
    new_job->pid = pid; 
    new_job->suspended = false; 
    new_job->name = malloc(strlen(name) + 1);
    assert(new_job->name);
    strcpy(new_job->name, name);
    new_job->next = NULL;  
    if (jobs->curr_jid == 0) {
        jobs->curr_jid = 1;
        jobs->jobs_list = new_job; 
        new_job->jid = 1; 
    } else {
        new_job->jid = jobs->curr_jid + 1; 
        if (jobs->jobs_list != NULL) {
            job_t *last_job = get_last_job(jobs->jobs_list);
            last_job->next = new_job; 
        }
        if (jobs->jobs_list == NULL) {
            jobs->jobs_list = new_job;
        }
        jobs->curr_jid++; 
    }
}

int get_curr_jid(job_list_t *jobs) {
    job_t *list = jobs->jobs_list;
    job_t *last_job = get_last_job(list); 
    return last_job->jid; 
}

void add_pid(job_list_t *jobs, pid_t pid){
    job_t *last_job = get_last_job(jobs->jobs_list);
    last_job->pid = pid; 
}

void free_job_list(job_list_t *jobs) {
    job_t *curr = jobs->jobs_list; 
    while (curr) {
        job_t *temp = curr; 
        curr = curr->next; 
        free(temp->name); 
        free(temp); 
    }
    free(jobs);
}

void clean_jobs() {
    job_t *curr = jobs->jobs_list; 
    job_t *prev = NULL; 
    while (curr) {
        if (kill(curr->pid, 0) == -1) {
            job_t *temp = curr;
            if (prev) {
                prev->next = curr->next; 
            }
            prev = curr; 
            curr = curr->next;
            if (jobs->jobs_list == prev) {
                jobs->jobs_list = curr; 
            }
            temp->next = NULL; 
            free(temp);
        } else {
            prev = curr; 
            curr = curr->next; 
        }
    }
}

int get_jid(pid_t cpid) {
    if (jobs == NULL || jobs->jobs_list == NULL) {
        return -1; 
    }
    job_t *curr = jobs->jobs_list; 
    while (curr && curr->pid != cpid) {
        curr = curr->next; 
    }
    if (!curr) {
        return -1; 
    }
    return curr->jid; 
}

char *get_name(pid_t cpid) {
    if (jobs == NULL || jobs->jobs_list == NULL)
        return NULL;
    job_t *curr = jobs->jobs_list; 
    while (curr && curr->pid != cpid) {
        curr = curr->next; 
    }
    if (!curr) {
        return NULL;
    }
    return curr->name; 
}

job_t *get_job_jid(int jid) {
    if (jobs == NULL || jobs->jobs_list == NULL) {
        return NULL; 
    }
    job_t *curr = jobs->jobs_list; 
    while (curr && curr->jid != jid) {
        curr = curr->next; 
    }
    if (!curr) {
        return NULL; 
    }
    return curr; 
}

job_t *get_job_pid(int pid) {
    if (jobs == NULL || jobs->jobs_list == NULL) {
        return NULL; 
    }
    job_t *curr = jobs->jobs_list; 
    while (curr && curr->pid != pid) {
        curr = curr->next; 
    }
    if (!curr) {
        return NULL; 
    }
    return curr; 
}

void print_jobs() {
    job_t *curr = jobs->jobs_list;
    while (curr) {
        printf("\tmem addr: %p, jid: %d, pid: %d, name: %s, next: %p, suspended: %d\n", curr, curr->jid, curr->pid, curr->name, curr->next, curr->suspended);
        curr = curr->next; 
    }
}

void kill_all_jobs() {
    job_t *curr = jobs->jobs_list;
    char cmd[100];
    while(curr) {
        kill(curr->pid, SIGKILL);
        snprintf(cmd, sizeof(cmd), "[%d] (%d)  killed  %s\n", curr->jid, curr->pid, curr->name);
        write(STDOUT_FILENO, cmd, strlen(cmd));
        curr = curr->next;
    }
}

void set_suspended(pid_t pid) {
    job_t *curr = get_job_pid(pid); 
    curr->suspended = true; 
}

bool jobs_full() {
    clean_jobs();
    int count = 0; 
    job_t *curr = jobs->jobs_list; 
    while (curr) {
        if (curr->suspended) {
            count++;
        }
        curr = curr->next; 
    }
    return (count >= 32); 
}

void print_status(pid_t p1, const char* name) {
    char r[100];
    if (flag_c) {
        kill(p1, SIGINT); 
        snprintf(r, sizeof(r), "[%d] (%d)  killed  %s\n", get_curr_jid(jobs), p1, name);
        write(STDOUT_FILENO, r, strlen(r));
        flag_c = false; 
    } else if (flag_q) {
        kill(p1, SIGQUIT); 
        snprintf(r, sizeof(r), "[%d] (%d)  killed  %s\n", get_curr_jid(jobs), p1, name);
        write(STDOUT_FILENO, r, strlen(r));
        flag_q = false; 
    } else if(flag_z) {
        kill(p1, SIGTSTP); 
        snprintf(r, sizeof(r), "[%d] (%d)  suspended  %s\n", get_curr_jid(jobs), p1, name);
        write(STDOUT_FILENO, r, strlen(r));
        flag_z = false;
        set_suspended(p1);
    } else {
        snprintf(r, sizeof(r), "[%d] (%d)  finished  %s\n", get_curr_jid(jobs), p1, name);
        write(STDOUT_FILENO, r, strlen(r));
    }
}

void eval(const char **toks, bool bg, struct sigaction *act, struct sigaction *act_fg) { // bg is true iff command ended with &
    assert(toks);
    if (*toks == NULL) return;
    if (strcmp(toks[0], "quit") == 0) {
        if (toks[1] != NULL) {
            const char *msg = "ERROR: quit takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            exit(0);
        }
    } else if (jobs_full()) {
        const char *msg = "ERROR: too many jobs\n";
        write(STDERR_FILENO, msg, strlen(msg));
    } else if (strcmp(toks[0], "jobs") == 0) {
        if (toks[1] != NULL) {
            const char *msg = "ERROR: jobs takes no arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            // remove dead processes from job list 
            clean_jobs(); 
            job_t *curr = jobs->jobs_list; 
            while (curr != NULL) {
                char p[100];
                if (curr->suspended) {
                    snprintf(p, sizeof(p), "[%d] (%d)  suspended  %s\n", curr->jid, curr->pid, curr->name);
                    write(STDOUT_FILENO, p, strlen(p));
                } else {
                    snprintf(p, sizeof(p), "[%d] (%d)  running  %s\n", curr->jid, curr->pid, curr->name);
                    write(STDOUT_FILENO, p, strlen(p));
                }
                curr = curr->next; 
            }
        }
    } else if (strcmp(toks[0], "nuke") == 0) {
        if (toks[1] == NULL) {
            clean_jobs();
            kill_all_jobs();
        } 

        int i = 1;
        while(toks[i]) {
            job_t *curr; 
            char cmd2[100];
            char* endptr; 
            int saved_errno = errno;

            const char *str = toks[i];
            if (str[0] == '%') {
                const char *temp = str; 
                temp++; 
                str = temp; 
            }

            long num = strtol(str, &endptr, 10);
            if (errno == ERANGE || endptr == str || *endptr != '\0') {
                snprintf(cmd2, sizeof(cmd2), "ERROR: bad argument for nuke: %s\n", toks[i]);
                write(STDERR_FILENO, cmd2, strlen(cmd2));
            } else {
                if (toks[i][0] == '%') {
                    const char *temp = toks[i];
                    temp++; 
                    clean_jobs();
                    curr = get_job_jid(num);
                    if (!curr) {
                        char err[100];
                        snprintf(err, sizeof(err), "ERROR: no job %s\n", temp);
                        write(STDERR_FILENO, err, strlen(err));
                    } else {
                        char cmd[100];
                        kill(curr->pid, SIGKILL);
                        snprintf(cmd, sizeof(cmd), "[%d] (%d)  killed  %s\n", curr->jid, curr->pid, curr->name);
                        write(STDOUT_FILENO, cmd, strlen(cmd));
                    }
                } else {
                    clean_jobs();
                    curr = get_job_pid(num); 
                    if (!curr) {
                        char err[100];
                        snprintf(err, sizeof(err), "ERROR: no PID %s\n", toks[i]);
                        write(STDERR_FILENO, err, strlen(err));
                    } else {
                        char cmd[100];
                        kill(curr->pid, SIGKILL);
                        snprintf(cmd, sizeof(cmd), "[%d] (%d)  killed  %s\n", curr->jid, curr->pid, curr->name);
                        write(STDOUT_FILENO, cmd, strlen(cmd));
                    }
                }
            }
            i++;
            errno = saved_errno; 
        }

    } else if (strcmp(toks[0], "fg") == 0) {
        sigprocmask(SIG_BLOCK, &(act->sa_mask), NULL); 
        if (toks[2] != NULL) {
            const char *msg = "ERROR: fg needs exactly one argument\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            job_t *curr; 
            char cmd2[100];
            char* endptr; 
            int saved_errno = errno;

            const char *str = toks[1];
            if (str[0] == '%') {
                const char *temp = str; 
                temp++; 
                str = temp; 
            }

            long num = strtol(str, &endptr, 10);
            if (errno == ERANGE || endptr == str || *endptr != '\0') {
                snprintf(cmd2, sizeof(cmd2), "ERROR: bad argument for fg: %s\n", toks[1]);
                write(STDERR_FILENO, cmd2, strlen(cmd2));
            } else {
                // set fg true
                fg = true; 
                if (toks[1][0] == '%') {
                    const char *temp = toks[1];
                    temp++; 
                    clean_jobs();
                    curr = get_job_jid(num);
                    if (!curr) {
                        char err[100];
                        snprintf(err, sizeof(err), "ERROR: no job %s\n", temp);
                        write(STDERR_FILENO, err, strlen(err));
                    } else {
                        if (curr->suspended) {
                            char cmd[100];
                            kill(curr->pid, SIGCONT);
                            snprintf(cmd, sizeof(cmd), "[%d] (%d)  continued  %s\n", curr->jid, curr->pid, curr->name);
                            write(STDOUT_FILENO, cmd, strlen(cmd));
                            curr->suspended = false;
                        }
                        // waitpid
                        pid_t p2;
            
                        int status;
                        while((flag_c == false && flag_q == false && flag_z == false)) {
                            p2 = waitpid(curr->pid, &status, WNOHANG); 
                            if (p2 > 0) {
                                break; 
                            }
                            sleep(0.001);
                        }
                    
                        print_status(curr->pid, curr->name);
                        sigprocmask(SIG_UNBLOCK, &(act->sa_mask), NULL);
                    }
                } else {
                    clean_jobs();
                    curr = get_job_pid(num); 
                    if (!curr) {
                        char err[100];
                        snprintf(err, sizeof(err), "ERROR: no PID %s\n", toks[1]);
                        write(STDERR_FILENO, err, strlen(err));
                    } else {
                        if (curr->suspended) {
                            char cmd[100];
                            kill(curr->pid, SIGCONT);
                            snprintf(cmd, sizeof(cmd), "[%d] (%d)  continued  %s\n", curr->jid, curr->pid, curr->name);
                            write(STDOUT_FILENO, cmd, strlen(cmd));
                            curr->suspended = false;
                        }
                        // waitpid
                        pid_t p2;

                        int status;
                        while((flag_c == false && flag_q == false && flag_z == false)) {
                            p2 = waitpid(curr->pid, &status, WNOHANG); 
                            if (p2 > 0) {
                                break; 
                            }
                            sleep(0.001);
                        }

                        print_status(curr->pid, curr->name);
                        sigprocmask(SIG_UNBLOCK, &(act->sa_mask), NULL);
                    }
                }
                // set fg false again
                fg = false; 
            }
            errno = saved_errno; 
        }
    } else if (strcmp(toks[0], "bg") == 0) { 
        if (toks[1] == NULL) {
            const char *msg = "ERROR: bg needs some arguments\n";
            write(STDERR_FILENO, msg, strlen(msg));
        } else {
            int i = 1;
            while(toks[i]) {
                job_t *curr; 
                char cmd2[100];
                char* endptr; 
                int saved_errno = errno;

                const char *str = toks[i];
                if (str[0] == '%') {
                    const char *temp = str; 
                    temp++; 
                    str = temp; 
                }

                long num = strtol(str, &endptr, 10);
                if (errno == ERANGE || endptr == str || *endptr != '\0') {
                    snprintf(cmd2, sizeof(cmd2), "ERROR: bad argument for bg: %s\n", toks[i]);
                    write(STDERR_FILENO, cmd2, strlen(cmd2));
                } else {
                    if (toks[i][0] == '%') {
                        const char *temp = toks[i];
                        temp++; 
                        clean_jobs();
                        curr = get_job_jid(num);
                        if (!curr || curr->suspended == false) {
                            char err[100];
                            snprintf(err, sizeof(err), "ERROR: no job %s\n", temp);
                            write(STDERR_FILENO, err, strlen(err));
                        } else {
                            char cmd[100];
                            kill(curr->pid, SIGCONT);
                            snprintf(cmd, sizeof(cmd), "[%d] (%d)  continued  %s\n", curr->jid, curr->pid, curr->name);
                            write(STDOUT_FILENO, cmd, strlen(cmd));
                            curr->suspended = false;
                        }
                    } else {
                        clean_jobs();
                        curr = get_job_pid(num); 
                        if (!curr || curr->suspended == false) {
                            char err[100];
                            snprintf(err, sizeof(err), "ERROR: no PID %s\n", toks[i]);
                            write(STDERR_FILENO, err, strlen(err));
                        } else {
                            char cmd[100];
                            kill(curr->pid, SIGCONT);
                            snprintf(cmd, sizeof(cmd), "[%d] (%d)  continued  %s\n", curr->jid, curr->pid, curr->name);
                            write(STDOUT_FILENO, cmd, strlen(cmd));
                            curr->suspended = false;
                        }
                    }
                }
                i++;
                errno = saved_errno; 
            }
        }
    } else {
        sigprocmask(SIG_BLOCK, &(act->sa_mask), NULL);  
        add_job(jobs, toks[0], getpid());  // add job without pid 
        if (!bg) {
            fg = true;
        }
        pid_t ppgid = getpgid(getpid());
        pid_t p1 = fork(); 
        if (p1 == 0) {
            sigaction(SIGINT, act_fg, NULL);
            sigprocmask(SIG_UNBLOCK, &(act->sa_mask), NULL); 
            // if (bg) {
            //     sigprocmask(SIG_BLOCK, &(act_fg->sa_mask), NULL);  
            // }
            int bandage = errno; 
            int cpgid = setpgid(getpid(), getpid());
            errno = bandage; 
            int error = execvp(toks[0], (char *const *) toks);
            if (error == -1) {
                char err[50]; 
                snprintf(err, sizeof(err), "ERROR: cannot run %s\n", toks[0]);
                write(STDERR_FILENO, err, strlen(err));
                exit(1); // will definitely cause problems but works for now
            }
        } else {
            add_pid(jobs, p1);
            if (bg) {
                char r[100];
                snprintf(r, sizeof(r), "[%d] (%d)  running  %s\n", get_curr_jid(jobs), p1, toks[0]);
                write(STDOUT_FILENO, r, strlen(r));
            } else {
                pid_t p2;
                int status;
                while((flag_c == false && flag_q == false && flag_z == false)) {
                    p2 = waitpid(p1, &status, WNOHANG); 
                    if (p2 > 0) {
                        break; 
                    }
                    sleep(0.001);
                }
                print_status(p1, toks[0]);
                fg = false; 
            }
            sigprocmask(SIG_UNBLOCK, &(act->sa_mask), NULL);
        }
    }
}

void parse_and_eval(char *s, struct sigaction* act, struct sigaction *act_fg) {
    assert(s);
    const char *toks[MAXLINE+1];
    
    while (*s != '\0') {
        bool end = false;
        bool bg = false;
        int t = 0;

        while (*s != '\0' && !end) {
            while (*s == '\n' || *s == '\t' || *s == ' ') ++s;
            if (*s != ';' && *s != '&' && *s != '\0') toks[t++] = s;
            while (strchr("&;\n\t ", *s) == NULL) ++s;
            switch (*s) {
            case '&':
                bg = true;
                end = true;
                break;
            case ';':
                end = true;
                break;
            }
            if (*s) *s++ = '\0';
        }
        toks[t] = NULL;
        eval(toks, bg, act, act_fg);
    }
}

void prompt() {
    const char *prompt = "crash> ";
    ssize_t nbytes = write(STDOUT_FILENO, prompt, strlen(prompt));
}

void handle_SIGCHLD(int sig, siginfo_t *info, void *context) {
    int saved_errno = errno; 
    int status; 
    // reap children 
    while (waitpid(-1, &status, WNOHANG) > 0) {
        char cmd[100];
        int jid = get_jid(info->si_pid);
        if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            if (sig == SIGQUIT || sig == SIGSEGV) {
                snprintf(cmd, sizeof(cmd), "[%d] (%d)  killed (core dumped)  %s\n", jid,  info->si_pid, get_name(info->si_pid));
                write(STDOUT_FILENO, cmd, strlen(cmd));
            } else if (sig == SIGKILL){
                
            } else {
                snprintf(cmd, sizeof(cmd), "[%d] (%d)  killed  %s\n", jid,  info->si_pid, get_name(info->si_pid));
                write(STDERR_FILENO, cmd, strlen(cmd));
            }
        } else {
            snprintf(cmd, sizeof(cmd), "[%d] (%d)  finished  %s\n", jid,  info->si_pid, get_name(info->si_pid));
            write(STDOUT_FILENO, cmd, strlen(cmd));
        }
    }
    errno = saved_errno; 
}

void handle_sigint_sigtstp_sigquit(int sig, siginfo_t *info, void *context) {
    if (sig == SIGQUIT) {
        if (fg) {
            flag_q = true; 
        } else {
            exit(0);
        }
    }
    if (fg) {
        if (sig == SIGINT) {
            flag_c = true;
        } else if (sig == SIGTSTP || sig == SIGSTOP){
            flag_z = true;
        }
    } else {
        // do nothing?
    }
} 

int repl() {
    char *buf = NULL;
    size_t len = 0;
    jobs = malloc(sizeof(job_list_t)); 
    jobs->curr_jid = 0;
    jobs->jobs_list = NULL;  

    // signal stuff
    struct sigaction actc;
    actc.sa_sigaction = handle_SIGCHLD;
    actc.sa_flags = SA_RESTART | SA_SIGINFO;
    sigemptyset(&actc.sa_mask);
    sigaddset(&actc.sa_mask, SIGCHLD);
    sigaction(SIGCHLD, &actc, NULL);

    struct sigaction act_fg; 
    act_fg.sa_sigaction = handle_sigint_sigtstp_sigquit; 
    act_fg.sa_flags = SA_RESTART | SA_SIGINFO; 
    sigemptyset(&act_fg.sa_mask);
    sigaddset(&act_fg.sa_mask, SIGINT);
    sigaddset(&act_fg.sa_mask, SIGTSTP);
    sigaction(SIGINT, &act_fg, NULL);
    sigaction(SIGTSTP, &act_fg, NULL);
    sigaction(SIGQUIT, &act_fg, NULL);
    sigaction(SIGSTOP, &act_fg, NULL);

    while (prompt(), true) {
        int read_result = getline(&buf, &len, stdin); 
        if (read_result == -1) {
            if (!fg) {
                break; 
            }
        } else {
            parse_and_eval(buf, &actc, &act_fg);
        }
    }

    if (buf != NULL) free(buf);
    if (ferror(stdin)) {
        perror("ERROR");
        return 1;
    }
    free_job_list(jobs);
    return 0;
}

int main(int argc, char **argv) {
    return repl();
}
