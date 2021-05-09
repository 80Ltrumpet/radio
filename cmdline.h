#ifndef _CMDLINE_H
#define _CMDLINE_H

/**
 * Initializes static variables necessary to operate the command line.
 */
void cmdline_init(void);

/**
 * Prompts the user for a line of input. This function must be called in a
 * polling manner. This function may be used in non-command-line contexts.
 * @param [in]  prompt - The prompt to display before the input.
 * @param [out] line   - A pointer to where the null-terminated input is stored
 *                       only if a newline or carriage return was entered. NULL
 *                       otherwise.
 * @return 0  - A single character of input was handled, or there was no input
 *              to process. If a newline or carriage return was entered, line
 *              will be non-NULL.
 * @return -1 - Input was cancelled by the user.
 */
int cmdline_get_input(const char *prompt, char **line);

/**
 * Turns a command line into a standard argument vector.
 * @param [in]  cmd  - The null-terminated command string to process.
 * @param [in]  argc - The length of argv.
 * @param [out] argv - Array into which to store pointers to arguments in cmd.
 *                     The arguments are NOT copied, so modifications to storage
 *                     pointed to by cmd will affect argv and vice versa.
 * @return The number of arguments in argv.
 * @return 0  - The command is empty.
 * @return -1 - There are too many arguments.
 */
int cmdline_get_argv(char *cmd, int argvlen, const char *argv[]);

/**
 * Cooperatively scheduled task that operates the command line.
 */
void cmdline_task(void *arg);

#endif
