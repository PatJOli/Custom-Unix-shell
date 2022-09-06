#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define CMDLENGTH 200

struct job
{
    int job_id;
    pid_t pid;
    char job_command[CMDLENGTH];
    struct job *next;
};

// GLOBAL VARIABLE
pid_t fgpid = 0;

void splitSting(char *line, char **args)
{
    char *space = " ";
    char *token;
    int i = 0;

    token = strtok(line, space);

    while (token != NULL)
    {
        strcpy(args[i], token);
        token = strtok(NULL, space);
        i++;
    }
    args[i] = NULL;
}

int readLine(char *line)
{
    int read = 1;

    if (fgets(line, CMDLENGTH, stdin))
    {
        read = 0;
    }
    else
    {
        read = 1;
    }

    if ((isatty(STDIN_FILENO) != 1) && (line[0] != '\0'))
    {
        printf("%s", line);
    }

    if (feof(stdin))
    {
        printf("\n");
    }
    fflush(stdout);

    int length = strlen(line);

    if (length > 0 && line[length - 1] == '\n')
    {
        line[length - 1] = '\0';
    }
    return read;
}

void addHistory(char *line, char **history, int *place, int ID[])
{

    int position = (*place - 1) % 10;
    strcpy(history[position], line);
    ID[position] = *place;
    (*place)++;
}

void printHistory(char **history, int ID[])
{
    int i = 0;
    int j;
    int values[10];

    for (int k = 0; k < 10; k++)
    {
        values[k] = ID[k];
    }

    while ((i < 10) && (history[i] != NULL) && (*history[i] != '\0'))
    {

        int small = __INT_MAX__;

        for (int k = 0; k < 10; k++)
        {
            if (values[k] < small)
            {
                small = values[k];
                j = k;
            }
        }
        printf("%5d: %s\n", values[j], history[j]);
        values[j] = __INT_MAX__;
        i++;
    }
}

int historyCheck(int ID[], char *check)
{
    int id = atoi(check);

    for (int i = 0; i < 10; i++)
    {
        if (id == ID[i])
        {
            return ID[i];
        }
    }
    return 0;
}

int containsAmp(char **cmd)
{
    int i = 0;
    while (cmd[i] != NULL)
    {
        if (!strcmp(cmd[i], "&") && (cmd[i + 1] == NULL))
        {
            cmd[i] = NULL; // remove the & to allow commands to run normally
            return 1;
        }
        i++;
    }
    return 0;
}

int containsPipe(char **cmd)
{
    int pipes = 0, i = 0;
    while (cmd[i] != NULL)
    {
        if (!strcmp(cmd[i], "|"))
        {
            pipes++;
        }
        i++;
    }
    return pipes;
}

int splitPipes(char **cmd, char **pipe, int start)
{
    int i = start;
    int j = 0;
    while (cmd[i] != NULL)
    {
        if ((!strcmp(cmd[i], "|")) && i != 0)
        {
            i++;
            break;
        }
        strcpy(pipe[j], cmd[i]);
        i++;
        j++;
    }
    pipe[j] = NULL;
    return i;
}

int runPipe(char **cmd)
{
    int numCmd = containsPipe(cmd);
    int pipefd[2];
    int start = 0, carryInput = STDIN_FILENO;
    pid_t ppid;

    char **pipeCmd = malloc(sizeof(char *) * 10);

    for (int i = 0; i < 9; i++)
    {
        pipeCmd[i] = malloc(sizeof(char) * 50);
    }

    for (int i = 0; i < numCmd; i++)
    { // for loop runs for each | in command

        char **pipeCmd = malloc(sizeof(char *) * 10);

        for (int i = 0; i < 9; i++)
        {
            pipeCmd[i] = malloc(sizeof(char) * 50);
        }

        start = splitPipes(cmd, pipeCmd, start);

        if (pipe(pipefd))
        {
            perror("pipe");
        }
        ppid = fork();

        if (ppid == -1)
        {
            perror("Fork");
            exit(EXIT_FAILURE);
        }
        if (ppid == 0)
        {

            if (carryInput != STDIN_FILENO)
            {
                dup2(carryInput, STDIN_FILENO);
                close(carryInput);
            }

            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            execvp(*pipeCmd, pipeCmd);

            exit(EXIT_SUCCESS);
        }

        waitpid(ppid, NULL, 0);

        close(pipefd[1]);

        carryInput = pipefd[0];
        free(pipeCmd);
    }

    dup2(carryInput, STDIN_FILENO);
    close(carryInput);
    splitPipes(cmd, pipeCmd, start);
    execvp(*pipeCmd, pipeCmd);
    free(pipeCmd);
}

int addJob(int id, pid_t pid, char *cmd, struct job *next, struct job *job)
{
    strcpy(job->job_command, cmd);
    job->job_id = id;
    job->pid = pid;
    job->next = next;
}

int getNewId(struct job *head)
{
    int id;
    if (head == NULL)
    {
        id = 1;
    }
    else
    {
        id = (head->job_id) + 1;
    }
    return id;
}

int getStatus(char *status, int pid)
{
    FILE *fp;
    char bpid[30];
    char line[CMDLENGTH];
    char get[50];
    char file[30] = "/proc/";

    sprintf(bpid, "%d", pid);
    strcat(file, bpid);
    strcat(file, "/status");
    fp = fopen(file, "r");

    if (fp == NULL)
    {
        return 0;
    }
    for (int i = 0; i < 3; i++)
    {
        // getline(&line,&n,fp);
        fgets(line, CMDLENGTH, fp);
        // printf("%s\n", line);
    }

    char *letter;
    char state[10];

    char **stateLine = malloc(sizeof(char *) * 5);

    for (int i = 0; i < 5; i++)
    {
        stateLine[i] = malloc(sizeof(char) * 50);
    }

    splitSting(line, stateLine);

    // sscanf(line, " %s %s %s ", state, letter, get);
    // printf("state = %s letter = %s get = %s\n ", state, letter, get);
    int i = 0;
    while (stateLine[1][i])
    {
        if (stateLine[1][i] == '\n')
            stateLine[1][i] = '\0';
        i++;
    }

    strcpy(status, stateLine[1]);
    fclose(fp);
    clearerr(stdin);
}

int deleteJob(struct job *job, struct job **head)
{
    struct job *travel = *head;
    if (job == *head)
    {
        *head = job->next;
        free(job);
        return 1;
    }
    while (travel != NULL)
    {
        if (travel->next == job)
        {
            travel->next = job->next; // remove job from the linked list path
            free(job);
            return 1;
        }
        travel = travel->next;
    }
}

int checkDone(struct job **head)
{
    struct job *travel = *head;

    while (travel != NULL)
    {
        int status = -1; // reset status

        waitpid(travel->pid, &status, WNOHANG);
        if (WIFEXITED(status))
        {
            printf("[%d] <Done> %s\n", travel->job_id, travel->job_command);
            struct job *temp = travel->next;
            deleteJob(travel, head);
            travel = temp;
            continue;
        }
        travel = travel->next;
    }
}

pid_t stoppedPid(struct job **head, int id, int del)
{
    struct job *travel = *head;
    char status[30];
    __pid_t temp;
    while (travel != NULL)
    {
        getStatus(status, travel->pid);
        if (!strcmp("(stopped)", status))
        {
            if (id == 0)
            {
                temp = travel->pid;
                if (del)
                    deleteJob(travel, head);
                return temp;
            }
            else if (travel->job_id == id)
            {
                temp = travel->pid;
                if (del)
                    deleteJob(travel, head);
                return temp;
            }
        }
        travel = travel->next;
    }
}

int hasStopped(struct job *head)
{
    struct job *travel = head;
    char status[30];
    while (travel != NULL)
    {
        getStatus(status, travel->pid);
        if (!strcmp("(stopped)", status))
        {
            return 1;
        }
        travel = travel->next;
    }
    return 0;
}

int hasID(struct job *head, int ID)
{
    struct job *travel = head;
    char status[30];
    while (travel != NULL)
    {
        getStatus(status, travel->pid);
        if ((travel->job_id == ID) && (!strcmp("(stopped)", status)))
        {
            return 1;
        }
        travel = travel->next;
    }
    return 0;
}

int contJob(struct job **head, int ID)
{
    pid_t pid = stoppedPid(head, ID, 0);
    struct job *travel = *head;

    while (travel != NULL)
    {
        if (travel->pid == pid)
        {
            break;
        }
        travel = travel->next;
    }
    char oldCmd[CMDLENGTH];
    strcpy(oldCmd, travel->job_command);
    deleteJob(travel, head);
    kill(pid, SIGCONT);
    fgpid = pid;
    int status = -1;

    waitpid(pid, &status, WUNTRACED);
    if (WIFSTOPPED(status))
    {
        struct job *job = malloc(sizeof(struct job));
        addJob(getNewId(*head), pid, oldCmd, *head, job);
        *head = job;
    }
}

void catchStop(int signum)
{
    if (signum == SIGTSTP)
    {
        if (fgpid)
        {
            kill(fgpid, SIGSTOP);
            fgpid = 0;
        }
        write(STDOUT_FILENO, "\n", 1);
    }
}

int main()
{
    char *line = (char *)malloc(sizeof(char) * CMDLENGTH); // TODO all for reallocating if the size is too big
    char cwd[CMDLENGTH];
    char **history = malloc(sizeof(char *) * 10);
    int histID[10];
    int *place = &(int){1};
    char *temp = (char *)malloc(sizeof(char) * CMDLENGTH);
    struct job *head = NULL;
    int amper = 0;
    struct sigaction sa;

    sa.sa_handler = catchStop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGTSTP, &sa, NULL) == -1)
    {
        perror("Signal");
    }

    for (int i = 0; i < 10; i++)
    {
        history[i] = malloc(sizeof(char) * CMDLENGTH);
        histID[i] = __INT_MAX__;
    }

    getcwd(cwd, sizeof(cwd));

    if (line == NULL)
    {
        printf("%s", "memory not allocated");
        exit(EXIT_FAILURE);
    }

    printf("%s", "ash> ");
    fflush(stdout);
    if (feof(stdin))
    {
        return 0;
    }

    readLine(line); // get the initial input

    while (1)
    {
        char **cmd = malloc(sizeof(char *) * 25);

        for (int i = 0; i < 24; i++)
        {
            cmd[i] = malloc(sizeof(char) * 50);
        }

        if (cmd == NULL)
        {
            printf("%s", "memory not allocated");
            exit(EXIT_FAILURE);
        }

        if (line[0] != '\0')
        {
            if (*place > 10)
            {
                strcpy(temp, history[(*place - 1) % 10]); // save the last available command in history
            }
            addHistory(line, history, place, histID);
        }

        splitSting(line, cmd);
        if (containsAmp(cmd))
        {
            amper = 1;
        }

        if (*cmd != NULL)
        {
            if (!strcmp(cmd[0], "cd"))
            {

                if (cmd[1] == NULL)
                {
                    chdir(cwd);
                }
                else if (cmd[2] == NULL)
                {
                    if (chdir(cmd[1]) != 0)
                    {
                        printf("cd: %s: No such file or directory\n", cmd[1]);
                    }
                }
                else
                {
                    printf("Too many arguments\n");
                }
            }
            else if (!strcmp(cmd[0], "fg"))
            {
                if (cmd[1] == NULL)
                {
                    contJob(&head, 0);
                }
                else if (cmd[2] == NULL)
                {
                    if (hasID(head, atoi(cmd[1])))
                    {
                        contJob(&head, atoi(cmd[1]));
                    }
                }
            }
            else if (!strcmp(cmd[0], "bg"))
            {
                if (cmd[1] == NULL)
                {
                    pid_t pid = stoppedPid(&head, 0, 0);
                    kill(pid, SIGCONT);
                }
                else if (cmd[2] == NULL)
                {
                    if (hasID(head, atoi(cmd[1])))
                    {
                        pid_t pid = stoppedPid(&head, atoi(cmd[1]), 0);
                        kill(pid, SIGCONT);
                    }
                }
            }
            else if (!strcmp(cmd[0], "kill"))
            {
                if (hasStopped(head))
                {
                    if (cmd[1] == NULL)
                    {
                        pid_t pid = stoppedPid(&head, 0, 1);
                        kill(pid, SIGKILL);
                    }
                    else if ((cmd[2] == NULL))
                    {
                        if (hasID(head, atoi(cmd[1])))
                        {
                            pid_t pid = stoppedPid(&head, atoi(cmd[1]), 1);
                            kill(pid, SIGKILL);
                        }
                    }
                }
            }
            else if ((!strcmp(cmd[0], "h")) || (!strcmp(cmd[0], "history")))
            {
                if (cmd[1] == NULL)
                {
                    printHistory(history, histID);
                }
                else if (cmd[2] == NULL)
                {
                    if (historyCheck(histID, cmd[1]))
                    {
                        strcpy(line, history[(atoi(cmd[1]) - 1) % 10]);
                        (*place)--;
                        strcpy(history[(*place - 1) % 10], "temp");
                        free(cmd);
                        continue;
                    }
                    else if (atoi(cmd[1]) == (*place - 11))
                    {
                        strcpy(line, temp);
                        (*place)--;
                        strcpy(history[(*place - 1) % 10], "temp");
                        temp[0] = '\0';
                        free(cmd);
                        continue;
                    }
                    else
                    {
                        printf("history: %s: No such command ID\n", cmd[1]);
                    }
                }
                else
                {
                    printf("Too many arguments\n");
                }
            }
            else if (!strcmp(cmd[0], "jobs"))
            {

                if (cmd[1] == NULL)
                {
                    checkDone(&head);
                    struct job *travel = head;
                    while (travel != NULL)
                    {
                        char *status = malloc(sizeof(char) * 50);
                        if (getStatus(status, travel->pid))
                        {
                            continue;
                        }
                        printf("[%d] %s %s\n", travel->job_id, status, travel->job_command);
                        travel = travel->next;
                        free(status);
                    }
                }
            }
            else if (containsPipe(cmd))
            {
                pid_t cpid;

                cpid = fork();

                if (cpid == -1)
                {
                    perror("Fork");
                    exit(EXIT_FAILURE);
                }
                if (cpid == 0)
                {
                    runPipe(cmd);
                    exit(EXIT_SUCCESS);
                }

                if (amper)
                {
                    struct job *job = malloc(sizeof(struct job));
                    addJob(getNewId(head), cpid, history[(*place - 2) % 10], head, job);
                    head = job;
                    printf("[%d]%8d\n", job->job_id, job->pid);
                }
                else
                {
                    int status = -1;
                    fgpid = cpid;
                    waitpid(cpid, &status, WUNTRACED);
                    if (WIFSTOPPED(status))
                    {
                        struct job *job = malloc(sizeof(struct job));
                        addJob(getNewId(head), cpid, history[(*place - 2) % 10], head, job);
                        head = job;
                    }
                }
            }
            else
            {
                pid_t cpid;

                cpid = fork();

                if (cpid == -1)
                {
                    perror("Fork");
                    exit(EXIT_FAILURE);
                }
                if (cpid == 0)
                {
                    execvp(*cmd, cmd);
                    exit(EXIT_SUCCESS);
                }

                if (setpgid(cpid, cpid) != 0)
                    perror("setpid");

                if (amper)
                {
                    struct job *job = malloc(sizeof(struct job));
                    addJob(getNewId(head), cpid, history[(*place - 2) % 10], head, job);
                    head = job;
                    printf("[%d]%8d\n", job->job_id, job->pid);
                }
                else
                {
                    int status = -1;
                    fgpid = cpid;
                    // strcpy(fgJob.job_command,history[(*place - 2) % 10]);

                    waitpid(cpid, &status, WUNTRACED);
                    if (WIFSTOPPED(status))
                    {
                        struct job *job = malloc(sizeof(struct job));
                        addJob(getNewId(head), cpid, history[(*place - 2) % 10], head, job);
                        head = job;
                    }
                }
            }
        }
        free(cmd);
        checkDone(&head);

        if (!feof(stdin))
        {
            printf("%s", "ash> ");
            fflush(stdout);
        }
        else
        {
            break;
        }
        fflush(stdout);
        line[0] = '\0'; // reset line to prevent printing twice when parsing files

        readLine(line);
        amper = 0;
    }

    free(line);
    free(history);
    return 0;
}