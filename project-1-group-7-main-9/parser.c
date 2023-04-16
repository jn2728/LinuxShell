#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

typedef struct
{
	int size;
	char **items;
} tokenlist;

typedef struct queue_node
{
	int position;
	pid_t pid;
	tokenlist *tokens;
	struct queue_node *next;
} process_node;

typedef struct
{
	process_node *head;
	process_node *tail;
} queue;
/* Provided Functionality - From initial parser.c on Canvas*/
char *get_input(void);
tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

/* Queue Functionality - Added to support background processing*/
process_node *init_process_node(int position, pid_t pid, tokenlist *tokens);
queue *init_queue();
int enqueue(queue *q, int position, pid_t pid, tokenlist *tokens);
process_node *dequeue(queue *q);

/* Added Functionality - Added to support kernel functionality*/
char *get_exec_path(char *dirs, char *name);
int Change_Directory(tokenlist *tokens);
int execute_command_tree(tokenlist *tokens);
int is_built_in(char *cmd);
int process_command();
tokenlist *parse(tokenlist *tokens);
int execute_command(tokenlist *tokens);
void execute_redirect(tokenlist *tokens);
void pipe_exec(tokenlist *tokens, int index);
void printEcho(tokenlist *tokens);

/* Global Variables */
tokenlist *command_history;		// Stores command history
queue *process_queue;			// Meant to store background process pid's, position, and tokens for pid in a queue

int main()
{
	command_history = new_tokenlist();
	process_queue = init_queue();
	
	/* Running the shell */
	while (1)
	{
		if (!process_command())
			break;
	}

	/* Exit Fucntionality */
	int command = 1;
	if (command_history->size == 0)
		puts("There were no valid commands executed");
	else if (command_history->size > 3)
	{
		puts("The following were the last three valid commands executed:");
		for (int i = command_history->size - 1; i >= command_history->size - 3; i--)
		{
			printf("%d: %s\n", command, command_history->items[i]);
			command++;
		}
	}
	else
	{
		puts("The following were the last commands executed:");
		for (int i = command_history->size - 1; i >= 0; i--)
		{
			printf("%d: %s\n", command, command_history->items[i]);
			command++;
		}
	}
	free_tokens(command_history);
	return 0;
}

/* char *get_input(void): Retrieves command line input from the user*/
char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL)
	{
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *)realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *)realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;
	return buffer;
}

/* tokenlist *get_tokens(char *input): Parses command line input into tokens (tokens are separated by a space (""))*/
tokenlist *get_tokens(char *input)
{
	char *buf = (char *)malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL)
	{
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

/* tokenlist *new_tokenlist(void): Creates and initializes a tokenlist structure and returns it for further use*/
tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **)malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

/* void add_token(tokenlist *tokens, char *item): Adds the item to the tokenlist tokens structure */
void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *)malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

/* void free_tokens(tokenlist *tokens): Frees the tokenlist structure, tokens, from memory*/
void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}

/* int process_command(): Processes a command by getting input, parsing into tokens, extending parsed tokens, executing the command, then freeing memory blocks */
int process_command()
{
	printf("%s@%s: %s > ", getenv("USER"), getenv("HOSTNAME"), getenv("PWD"));	// Command Prompt
	char *input = get_input();							// Get input, tokenize, and parse
	tokenlist *tokens = get_tokens(input);
	parse(tokens);
	if (strcmp(tokens->items[0], "exit") == 0 && tokens->size == 1)			// If command is "exit," wait until background processes are finished and exit
	{
		waitpid(-1, NULL, 0);
		free(input);
		free_tokens(tokens);
		return 0;
	}
	else if (strcmp(tokens->items[0], "exit") == 0)
	{
		puts("exit: Expression Syntax.");
		free(input);
		free_tokens(tokens);
	}
	else
	{
		if (execute_command_tree(tokens))					// Execute command and add command to history if valid
			add_token(command_history, input);
		free(input);
		free_tokens(tokens);
	}
	return 1;
}

/* process_node *init_process_node(int position, pid_t pid, tokenlist *tokens):
 * creates a new process_node instance*/
process_node *init_process_node(int position, pid_t pid, tokenlist *tokens)
{
	process_node *new_node = (process_node *)malloc(sizeof(process_node));
	new_node->position = position;
	new_node->pid = pid;
	new_node->tokens = new_tokenlist();
	for (int i = 0; i < tokens->size; i++)
		add_token(new_node->tokens, tokens->items[i]);
	new_node->next = NULL;
	return new_node;
}

/* queue *init_queue(): initializes queue structure*/
queue *init_queue()
{
	queue *q = (queue *)malloc(sizeof(queue));
	q->head = NULL;
	q->tail = NULL;
	return q;
}

/* int enqueue(queue * q, int position, pid_t pid, tokenlist *tokens):
 * adds a process to the end of a queue*/
int enqueue(queue *q, int position, pid_t pid, tokenlist *tokens)
{
	process_node *new_node = init_process_node(position, pid, tokens);
	if (q->tail != NULL)
	{
		q->tail->next = new_node;
		q->tail = q->tail->next;
	}
	else
		q->head = q->tail = new_node;
	return 1;
}

/* process_node *dequeue(queue * q):
 * removes a process from the front of the queue and returns it */
process_node *dequeue(queue *q)
{
	if (q->head == NULL)
		return NULL;
	process_node *tmp = q->head;
	process_node *result = init_process_node(tmp->position, tmp->pid, tmp->tokens);
	q->head = q->head->next;
	if (q->head == NULL)
		q->tail = NULL;
	free(tmp);
	return result;
}

/* tokenlist *parse(tokenlist *tokens): Extends tokens further */
tokenlist *parse(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
	{	
		if (tokens->items[i][0] == '$') // Environmental Variables
		{
			if (getenv(tokens->items[i] + 1) == NULL)
			{
				char *tmp = (char*) malloc(sizeof("") * sizeof(char*));
				strcpy(tmp, "");
				tokens->items[i] = tmp;
				break;
			}
			else
			{
				char *temp = (char *)malloc(sizeof(getenv(tokens->items[i] + 1)) * 2 * sizeof(char *) + 1);
				strcpy(temp, getenv(tokens->items[i] + 1));
				free(tokens->items[i]);
				tokens->items[i] = temp;
			}
		}
		else if (tokens->items[i][0] == '~') // Tilde Expansion
		{
			char *temp = (char *)malloc((sizeof(tokens->items[i]) + sizeof(getenv("HOME")) - 1) * sizeof(char *));
			strcpy(temp, getenv("HOME"));
			strcat(temp, tokens->items[i] + 1);
			free(tokens->items[i]);
			tokens->items[i] = temp;
		}
		else if (!is_built_in(tokens->items[0])) // Path Search
		{
			if (get_exec_path(getenv("PATH"), tokens->items[i]) != NULL)
			{
				char *temp = (char *)malloc(sizeof(get_exec_path(getenv("PATH"), tokens->items[i])) * sizeof(char *));
				strcpy(temp, get_exec_path(getenv("PATH"), tokens->items[i]));
				free(tokens->items[i]);
				tokens->items[i] = temp;
			}
		}
		else if (is_built_in(tokens->items[i]) == 1) // Built in Command specifications
		{
			if (strcmp(tokens->items[0], "echo") == 0)
			{
				if (i != 0)
				{
					char *temp = (char *)malloc(sizeof(get_exec_path(getenv("PATH"), tokens->items[i])) * sizeof(char *));
					strcpy(temp, get_exec_path(getenv("PATH"), tokens->items[i]));
					free(tokens->items[i]);
					tokens->items[i] = temp;
				}
			}
		}
	}
	return tokens;
}

/* Checks if a command is built in, returns 1 if so, 0 otherwise. */
int is_built_in(char *cmd)
{
	if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "echo") == 0 || strcmp(cmd, "jobs") == 0 || strcmp(cmd, "exit") == 0)
		return 1;
	return 0;
}

/* Gets executable path for cmd */
char *get_exec_path(char *paths, char *cmd)
{
	/* Create a tokenlist of directories, separated by ":" */
	char *dirs = (char *)malloc(strlen(paths) + 1);
	strcpy(dirs, paths);

	tokenlist *dir_tokens = new_tokenlist();

	char *dir = strtok(dirs, ":");
	while (dir != NULL)
	{
		add_token(dir_tokens, dir);
		dir = strtok(NULL, ":");
	}

	/* If the executable path is found, it is also constructed here */
	for (int i = 0; i < dir_tokens->size; i++)
	{
		char *temp = (char *)malloc((sizeof(dir_tokens->items[i]) + sizeof("/") + sizeof(cmd)) * sizeof(char *));
		strcpy(temp, dir_tokens->items[i]);
		strcat(temp, "/");
		strcat(temp, cmd);
		free(dir_tokens->items[i]);
		dir_tokens->items[i] = temp;
		if (fopen(dir_tokens->items[i], "r") != NULL)
		{
			free(dirs);
			return dir_tokens->items[i];
		}
	}
	free(dirs);
	return NULL;
}

/* Makes decision on creating a background process or a child process */
int execute_command_tree(tokenlist *tokens)
{
	/* Background process */
	if (strcmp(tokens->items[tokens->size - 1], "&") == 0)
	{
		tokenlist *b_tokens = new_tokenlist();
		for (int i = 0; i < tokens->size - 1; i++)
			add_token(b_tokens, tokens->items[i]);

		pid_t pid = fork();
		if (pid == -1)
		{
			perror("Fork");
			exit(1);
		}
		else if (pid == 0)
		{
			enqueue(process_queue, 5, pid, b_tokens);
			execute_redirect(b_tokens);
		}
		if (pid > 0)
		{
			int status;
			if (waitpid(pid, &status, WNOHANG) == 0)
			{
				return 1;
			}
			else
			{
				printf("Finished [%d] %d\n", pid, pid);
				return 1;
			}
		}
	}
	/* Child process*/
	else
	{
		int status;
		pid_t pid = fork();
		if (pid < 0)
		{
			perror("Fork Failed");
			exit(1);
		}
		else if (pid == 0)
		{
			execute_redirect(tokens);
		}
		else
		{
			waitpid(pid, &status, 0);
		}
	}
	return 1;
}

/* Handles I/O Redirection */
void execute_redirect(tokenlist *tokens)
{
	char *input_file;
	char *output_file;
	int in = 0;
	int out = 0;
	tokenlist *in_tokens;
	tokenlist *out_tokens;

	for (int i = 0; i < tokens->size; i++)
	{
		if (strcmp(tokens->items[i], "<") == 0)
		{
			input_file = (char *)malloc(sizeof(tokens->items[i + 1]) * sizeof(char *));
			strcpy(input_file, tokens->items[i + 1]);
			in = 1;
		}
		else if (strcmp(tokens->items[i], ">") == 0)
		{
			output_file = (char *)malloc(sizeof(tokens->items[i + 1]) * sizeof(char *));
			strcpy(output_file, tokens->items[i + 1]);
			out = 1;
		}
	}
	if (in)
	{
		in_tokens = new_tokenlist();
		for (int i = 0; i < tokens->size - 2; i++)
			add_token(in_tokens, tokens->items[i]);
		free_tokens(tokens);
		tokens = in_tokens;
	}
	if (out)
	{
		out_tokens = new_tokenlist();
		for (int i = 0; i < tokens->size - 2; i++)
			add_token(out_tokens, tokens->items[i]);
		free_tokens(tokens);
		tokens = out_tokens;
	}
	if (in)
	{
		int file = open(input_file, O_RDONLY);
		if (file == -1)
			return;
		dup2(file, 0);
		close(file);
	}
	if (out)
	{
		int file = open(output_file, O_WRONLY | O_CREAT, 0666);
		if (file == -1)
			return;
		dup2(file, 1);
		close(file);
	}
	int index = -1;
	for (int i = 0; i < tokens->size; i++)
	{
		if (strcmp(tokens->items[i], "|") == 0)
		{
			index = i;
			break;
		}
	}
	if (index == -1)
		execute_command(tokens);
	else
		pipe_exec(tokens, index);
}

/* Handles piping, error here, specified in README */
void pipe_exec(tokenlist *tokens, int index)
{

	int ind = index;
	int numPipes = 0;
	for (int i = 0; i < tokens->size; i++)
	{
		if (strcmp(tokens->items[i], "|") == 0)
			numPipes++;
	}

	if (numPipes == 1)
	{
		tokenlist *tokens1 = new_tokenlist();
		tokenlist *tokens2 = new_tokenlist();
		for (int i = 0; i < ind; i++)
		{
			add_token(tokens1, tokens->items[i]);
		}
		for (int i = ind + 1; i < tokens->size; i++)
		{
			add_token(tokens2, tokens->items[i]);
		}

		int fd[2];
		pipe(fd);
		pid_t pid1 = fork();
		pid_t pid2 = fork();

		if (pid1 < 0)
			return;
		if (pid2 < 0)
			return;
		if (pid1 == 0)
		{
			close(fd[0]);
			dup2(fd[1], 1);
			execute_command(tokens1);
			close(fd[1]);
			exit(0);
		}
		waitpid(pid1, NULL, 0);
		if (pid2 == 0)
		{
			close(fd[1]);
			printf("here2");
			dup2(fd[0], 0);
			execute_command(tokens2);
			close(fd[0]);
			exit(0);
		}
		close(fd[0]);
		close(fd[1]);
		waitpid(pid2, NULL, 0);
	}
	if (numPipes == 2)
	{
		int index1 = 0, index2 = 0;
		for (int i = 0; i < tokens->size; i++)
		{
			if (strcmp(tokens->items[i], "|") == 0)
			{
				index1 = i;
				break;
			}
		}
		for (int i = index1; i < tokens->size; i++)
		{
			if (strcmp(tokens->items[i], "|") == 0)
				index2 = i;
		}
		tokenlist *tokens1 = new_tokenlist();
		tokenlist *tokens2 = new_tokenlist();
		tokenlist *tokens3 = new_tokenlist();
		for (int i = 0; i < index1; i++)
			add_token(tokens1, tokens->items[i]);
		for (int i = index1 + 1; i < index2; i++)
			add_token(tokens2, tokens->items[i]);
		for (int i = index2 + 1; i < tokens->size; i++)
			add_token(tokens3, tokens->items[i]);
		int fd[2], fd2[2];
		pipe(fd2);
		pipe(fd);
		pid_t pid1 = fork();
		pid_t pid2 = fork();
		pid_t pid3 = fork();
		if (pid1 < 0)
			return;
		if (pid2 < 0)
			return;
		if (pid3 < 0)
			return;

		if (pid1 == 0)
		{
			close(fd2[0]);
			close(fd2[1]);
			close(fd[0]);
			dup2(fd[1], 1);
			execute_command(tokens1);
			close(fd[1]);
		}
		waitpid(pid1, NULL, 0);
		if (pid2 == 0)
		{
			close(fd[1]);
			close(fd2[0]);
			dup2(fd[0], 0);
			close(fd[0]);
			dup2(fd2[1], 1);
			execute_command(tokens2);
		}
		waitpid(pid2, NULL, 0);
		if (pid3 == 0)
		{
			close(fd[0]);
			close(fd[1]);
			close(fd2[1]);
			dup2(fd2[0], 0);
			close(fd2[0]);
		}
		close(fd[0]);
		close(fd2[0]);
		close(fd[1]);
		close(fd2[1]);
		waitpid(pid3, NULL, 0);
	}
}

/* echo built-in */
void printEcho(tokenlist *tokens)
{
	for (int i = 1; i < tokens->size; i++)
		printf("%s", tokens->items[i]);
	printf("\n");
}

/*cd built in*/
int Change_Directory(tokenlist *tokens)
{
	/* Too large */
	if(tokens->size > 2)
	{
		printf("Too many arguments.\n");
		return 0;
	}
	else if(tokens->size == 2)
	{
		if(strcmp(tokens->items[1], ".") == 0)
		{
			// Do nothing
			return 1;
		}
		else if(strcmp(tokens->items[1], "..") == 0)
		{
			// cd ..
			char *cwd = getcwd(NULL, 0);
			int i = strlen(cwd);
			while(cwd[i] != '/')
			{
				cwd[i] = '\0';
				i--;
			}
			setenv("PWD", cwd, 1);
			chdir(cwd);
			free(cwd);
			return 1;
		}
		else if(tokens->items[1][0] == '/')
		{
			// cd *absolute path*
			DIR *dir;
			if((dir = opendir(tokens->items[1])) == NULL)
			{
				puts("Target is not a directory");
			}
			else if(chdir(tokens->items[1]) != 0)
			{
				puts("Target does not exist");
			}
			else setenv("PWD", tokens->items[1], 1);
			return 1;
		}
		else
		{
			// cd *relative path*
			DIR *dir;
			char * cwd = (char*) malloc((sizeof(getenv("PWD")) + sizeof(tokens->items[1]) + 1) * sizeof(char*));
			strcpy(cwd, getenv("PWD"));
			strcat(cwd, "/");
			strcat(cwd, tokens->items[1]);
			if((dir = opendir(cwd)) == NULL)
			{
				puts("Target is not a directory");
			}
			else if(chdir(cwd) != 0)
                        {
                                puts("Target does not exist");
                        }
			else 
			{
				setenv("PWD", cwd, 1);
			}
			closedir(dir);
			free(cwd);
			return 1;
		}
	}
	/* cd */
	else if(tokens->size == 1)
	{
		char *pwd = getenv("HOME");
		setenv("PWD", pwd, 1);
		chdir(pwd);
		return 1;
	}
	/* Not enough arguments, essentially 0 input*/
	else
	{
		printf("Not enough arguments");
		return 0;
	}
	
	return 0;
}

/* execute_command: determine if the command should be run as a built-in or through execv*/
int execute_command(tokenlist *tokens)
{
	if (strcmp(tokens->items[0], "cd") == 0)
	{
		Change_Directory(tokens);
	}
	else if (strcmp(tokens->items[0], "echo") == 0)
	{
		printEcho(tokens);
	}
	else if (strcmp(tokens->items[0], "jobs") == 0)
	{
		// We didn't get to write a jobs function
		// But, if we did, we would print out all jobs as they were completed in process_queue
		// process_queue was having trouble in its enqueue function in a child process
	}
	else
		execv(tokens->items[0], tokens->items);
	return 1;
}
