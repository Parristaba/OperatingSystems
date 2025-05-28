#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <pthread.h>
#define MAX_LINE_LENGTH 1000
#define MAX_PIPES 100


typedef struct {
    char command[50];
    char input[50];
    char option[50];
    char redirection;
    char outputFileName[50];
    char background;
    pid_t pid;
} ParsedInfo;


pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;

void printParsedInfo(const ParsedInfo *info) {
    pthread_mutex_lock(&fileMutex);
    FILE *file = fopen("parse.txt", "a");
    if (file == NULL) {
        perror("Error opening parse.txt");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "----------\n");
    fprintf(file, "Command : %s\n", info->command);
    if (info->input[0] != '0') {
        fprintf(file, "Inputs: %s\n", info->input);
    }
    if (info->option[0] != '0') {
        fprintf(file,"Options: %s\n", info->option);
    }
    if (info->redirection == '-') {
        fprintf(file, "Redirection : -\n");
    } else {
        fprintf(file,"Redirection : %c\n", info->redirection);
    }
    fprintf(file,"Background Job : %c\n", info->background);
    fprintf(file, "----------\n");
    fflush(file);
    fclose(file);
    pthread_mutex_unlock(&fileMutex);
}

ParsedInfo parseCommand(const char *line) {
    char *lineCopy = strdup(line);
    ParsedInfo info = {"", "", "", '-', "", 'n'};

    char *token = strtok(lineCopy, " ");
    strcpy(info.command, token);
    token = strtok(NULL, " ");
    bool option = false;
    bool redirection = false;
    bool input = false;

    while (token != NULL) {
        if (token[0] == '-' && strlen(token) > 1) {
            strcat(info.option, token);
            option = true;
        } else if ((token[0] == '<' || token[0] == '>') && !redirection) {
            info.redirection = token[0];
            redirection = true;
        } else if (!redirection && !option) {
            strcat(info.input, token);
            input = true;
        } else if (redirection && token[0] != '&') {
            strcat(info.outputFileName, token);
        }
        token = strtok(NULL, " ");
    }

    size_t lineLength = strlen(line);
    if (lineLength > 0 && line[lineLength - 1] == '&') {
        info.background = 'y';
    }

    free(lineCopy);

    return info;
}

#define MAX_COMMANDS 100
#define MAX_THREADS 100
pid_t backgroundProcesses[MAX_COMMANDS];
int backgroundProcessCount = 0;

pthread_t backgroundThreads[MAX_THREADS];
int backgroundThreadCount = 0;

int readCommandsFromFile(ParsedInfo *commandsArray) {
    FILE *file = fopen("commands.txt", "r");
    if (file == NULL) {
        perror("Error opening commands.txt");
        return -1;
    }

    int commandCount = 0;
    char line[256];

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        commandsArray[commandCount++] = parseCommand(line);

        if (commandCount >= MAX_COMMANDS) {
            fprintf(stderr, "Too many commands in the file. Maximum allowed: %d\n", MAX_COMMANDS);
            break;
        }
    }

    fclose(file);
    return commandCount;
}

typedef struct {
    int pipe_fd[2];
} PipeInfo;


typedef struct {
    PipeInfo pipeInfo;
    pthread_t tid;
    pid_t pid;
} ThreadInfo;

PipeInfo pipes[MAX_COMMANDS];
ThreadInfo processInfoArray[MAX_COMMANDS];
int processInfoCount = 0;


pthread_mutex_t printMutex = PTHREAD_MUTEX_INITIALIZER;

void *mythread(void *arg) {
    pthread_mutex_lock(&printMutex);

    pthread_t tid = pthread_self();
    printf("---- %lu\n", (unsigned long)tid);

    PipeInfo *pipeInfo = (PipeInfo *)arg;

    close(pipeInfo->pipe_fd[1]);

    FILE *readStream = fdopen(pipeInfo->pipe_fd[0], "r");
    if (readStream == NULL) {
        perror("fdopen failed");
        pthread_mutex_unlock(&printMutex);
        pthread_exit(NULL);
    }

    char buffer[MAX_LINE_LENGTH];
    ssize_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), readStream)) > 0) {
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    printf("---- %lu\n", (unsigned long)tid);

    pthread_mutex_unlock(&printMutex);

    pthread_exit(NULL);
}

void executeCommand(ParsedInfo *info, int i) {
    if(strcmp(info->command, "wait") == 0){
        for (int t = 0; t < processInfoCount; ++t) {
            waitpid(processInfoArray[t].pid, NULL, 0);
        }

        for (int t = 0; t < processInfoCount; ++t) {
            pthread_join(processInfoArray[t].tid, NULL);
        }
        processInfoCount = 0;
    }
    else if (info->redirection == '>') {
        int outputFile = open(info->outputFileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (outputFile == -1) {
            perror("Error opening output file");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            dup2(outputFile, STDOUT_FILENO);
            close(outputFile);

            char *args[4];

            args[0] = (char *)info->command;
            args[1] = (info->input[0] != '\0') ? (char *)info->input : NULL;
            args[2] = (info->option[0] != '\0') ? (char *)info->option : NULL;
            args[3] = NULL;

            execvp(info->command, args);

            perror("Execvp failed");
            exit(EXIT_FAILURE);
        } else {
            info->pid = pid;
            int status;
            waitpid(pid, &status, 0);
        }
    } else {
        if (pipe(pipes[i].pipe_fd) == -1) {
            perror("Pipe creation failed");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();

        if (pid == -1) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            close(pipes[i].pipe_fd[0]);

            FILE *writeStream = fdopen(pipes[i].pipe_fd[1], "w");
            if (writeStream == NULL) {
                perror("fdopen failed in main");
                exit(EXIT_FAILURE);
            }

            dup2(fileno(writeStream), STDOUT_FILENO);
            fclose(writeStream);

            if (info->redirection == '<' && info->outputFileName[0] != '\0') {
                int inputFile = open(info->outputFileName, O_RDONLY);
                if (inputFile == -1) {
                    perror("Error opening input file");
                    exit(EXIT_FAILURE);
                }
                dup2(inputFile, STDIN_FILENO);
                close(inputFile);
            }

            char *args[4];

            args[0] = (char *)info->command;
            args[1] = (info->input[0] != '\0') ? (char *)info->input : NULL;
            args[2] = (info->option[0] != '\0') ? (char *)info->option : NULL;
            args[3] = NULL;

            execvp(info->command, args);

            perror("Execvp failed");
            exit(EXIT_FAILURE);
        } else {
            close(pipes[i].pipe_fd[1]);

            ThreadInfo threadInfo;
            threadInfo.pipeInfo.pipe_fd[0] = pipes[i].pipe_fd[0];

            if (pthread_create(&threadInfo.tid, NULL, mythread, &threadInfo.pipeInfo) != 0) {
                fprintf(stderr, "Error creating thread\n");
                exit(EXIT_FAILURE);
            }

            if (info->background == 'y') {
                processInfoArray[processInfoCount].pid = pid;
                processInfoArray[processInfoCount].tid = threadInfo.tid;
                processInfoCount++;
            } else {
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }
}



int main() {
    ParsedInfo commandsArray[MAX_COMMANDS];
    int commandCount = readCommandsFromFile(commandsArray);

    if (commandCount < 0) {
        return EXIT_FAILURE;
    }

    for (int i = 0; i < commandCount; ++i) {

        printParsedInfo(&commandsArray[i]);
    }

    for (int i = 0; i < commandCount; ++i) {
        executeCommand(&commandsArray[i],  i);
    }

    for (int i = 0; i < processInfoCount; ++i) {
            waitpid(processInfoArray[i].pid, NULL, 0);
    }

    for (int i = 0; i < processInfoCount; ++i) {
        pthread_join(processInfoArray[i].tid, NULL);
    }

    return EXIT_SUCCESS;
}



Graph:
    V: integer
    adj: array of LinkedList of integer
    adjMatrix: 2D array of integer
    
    Graph(v):
        V = v
        adj = new array of LinkedList with size v
        for i = 0 to v-1:
            adj[i] = new LinkedList()
        
        adjMatrix = generateRandomGraph(v, 0.85)
    
    getAdjMatrix():
        return adjMatrix
    
    setAdjMatrix(adjMatrix):
        this.adjMatrix = adjMatrix
    
    generateRandomGraph(numVertices, edgeProbability):
        adjacencyMatrix = new 2D array of integer with size numVertices x numVertices
        random = new Random()
        for i = 0 to numVertices - 1:
            for j = i + 1 to numVertices - 1:
                if random.nextDouble() < edgeProbability:
                    adjacencyMatrix[i][j] = 1
                    adjacencyMatrix[j][i] = 1
        return adjacencyMatrix
    
    printGraph(adjacencyMatrix):
        for i = 0 to length of adjacencyMatrix - 1:
            for j = 0 to length of adjacencyMatrix[i] - 1:
                print adjacencyMatrix[i][j] + " "
            println()
    
    addEdge(v, w):
        adj[v].add(w)
        adj[w].add(v)
    
    greedyColoring():
        result = new array of integer with size V
        fill result with -1
        
        result[0] = 0
        
        available = new array of boolean with size V
        fill available with true
        
        for u = 1 to V - 1:
            for i = 0 to V - 1:
                if i != u and adjMatrix[u][i] == 1 and result[i] != -1:
                    available[result[i]] = false
            
            cr = 0
            while cr < V:
                if available[cr]:
                    break
                cr++
            
            result[u] = cr
            fill available with true
        
        for u = 0 to V - 1:
            print "Vertex " + u + " --->  Color " + result[u]
