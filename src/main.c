#include "shell/shell.h"

int main(int argc, char *argv[], char *envp[])
{
	(void)argc;
	(void)argv;
	
	// run shell
	shell_init(envp);
	return 0;
}
