#include <stddef.h>

#include "cmdline.h"
#include "dhcp.h"
#include "printf.h"
#include "string.h"
#include "test.h"
#include "timer.h"
#include "usart.h"
#include "w5100.h"

#define CMDLINE_INPUT_SIZE          128
#define CMDLINE_MAX_INPUT           (CMDLINE_INPUT_SIZE - 1)
#define CMDLINE_HISTORY_COUNT       16
#define CMDLINE_HISTORY_INDEX_MASK  (CMDLINE_HISTORY_COUNT -1 )
#define CMDLINE_PROMPT              "> "
#define CMDLINE_MAX_ARGC            16

typedef enum {
   ESC_NONE,
   ESC_ESCAPE,
   ESC_BRACKET,
   ESC_1,
   ESC_3,
   ESC_LITERAL
} esc_t;

typedef struct {
   const char *name;
   void (*cmd)(int, const char *[]);
} cmd_t;

static char input[CMDLINE_INPUT_SIZE];
static unsigned int input_length;
static unsigned int cursor_pos;
static int need_prompt;
static esc_t escape;

static char history[CMDLINE_HISTORY_COUNT][CMDLINE_INPUT_SIZE];
static unsigned int history_oldest;
static unsigned int history_latest;
static unsigned int history_current;

static void help_cmd(int argc, const char *argv[]) {
   printf(
"Type a command by itself to see its usage.\n"
"\n"
"dhcp  - Monitor and control the DHCP state machine\n"
"help  - Print this help message\n"
"test  - Run a test suite\n"
"timer - Configure the timer\n"
"w5100 - Read or write W5100 memory\n"
"\n");
}

static cmd_t commands[] = {
   { "dhcp",  dhcp_cmd  },
   { "help",  help_cmd  },
   { "test",  test_cmd  },
   { "timer", timer_cmd },
   { "w5100", w5100_cmd },
   { NULL, NULL }
};

static void clear_input(void) {
   usart_putcn(KEY_BACKSPACE, input_length - cursor_pos);
   usart_putcn(' ', input_length);
   need_prompt = 1;
   input_length = 0;
   cursor_pos = 0;
}

void cmdline_init(void) {
   input[0] = '\0';
   input_length = 0;
   cursor_pos = 0;
   need_prompt = 0;
   escape = ESC_NONE;

   for (int i = 0; i < CMDLINE_HISTORY_COUNT; i++)
      history[i][0] = '\0';
   history_oldest = 0;
   history_latest = 0;
   history_current = 0;
}

int cmdline_get_input(const char *prompt, char **line) {
   int rc = 0;
   char c = usart_getc();
   unsigned int precursor_length = input_length - cursor_pos;

   if (need_prompt) {
      need_prompt = 0;
      input[input_length] = '\0';
      printf("\r%s%s", prompt, input);
   }

   if (c == '\0')
      return 0;

   switch (escape) {
      default:
      case ESC_NONE:
         if (c == KEY_ESCAPE) {
            escape = ESC_ESCAPE;
            return 0;
         }
         break;

      case ESC_ESCAPE:
         if (c == '[') {
            escape = ESC_BRACKET;
            return 0;
         }
         break;

      case ESC_BRACKET:
         switch (c) {
            case 'A':   c = KEY_UP;          break;
            case 'B':   c = KEY_DOWN;        break;
            case 'C':   c = KEY_RIGHT;       break;
            case 'D':   c = KEY_LEFT;        break;
            case 'F':   c = KEY_END;         break;
            case 'M':   c = KEY_CR;          break;
            case '1':   escape = ESC_1;      return 1;
            case '3':   escape = ESC_3;      return 1;
            default:
               /* Swallow unknown bracket escapes */
               escape = ESC_NONE;
               return 0;
         }
         break;

      case ESC_1:
         if (c != '~') {
            escape = ESC_NONE;
            return 0;
         }
         c = KEY_HOME;
         break;

      case ESC_3:
         if (c != '~') {
            escape = ESC_NONE;
            return 0;
         }
         c = KEY_DEL;
         break;

      case ESC_LITERAL:
         escape = ESC_NONE;
         goto literal_input;
         break;
   }

   switch (c) {
      case KEY_NL:
      case KEY_CR:
         usart_putc('\n');
         input[input_length] = '\0';
         *line = input;
         need_prompt = 1;
         cursor_pos = 0;
         /* Add this line to history, if not empty or the same as previous */
         if (input_length == 0)
            break;
         input_length = 0;
         if (strncmp(history[(history_latest - 1) & CMDLINE_HISTORY_INDEX_MASK],
                  input, CMDLINE_INPUT_SIZE) == 0)
            goto history_ff;
         strncpy(history[history_latest++ & CMDLINE_HISTORY_INDEX_MASK], input,
               CMDLINE_INPUT_SIZE);
         if (history_latest - history_oldest >= CMDLINE_HISTORY_COUNT &&
             ++history_oldest >= CMDLINE_HISTORY_COUNT) {
            history_oldest &= CMDLINE_HISTORY_INDEX_MASK;
            history_latest -= CMDLINE_HISTORY_COUNT;
         }
history_ff:
         history_current = history_latest;
         break;

      case KEY_CANCEL:
         rc = -1;
         input_length = 0;
         cursor_pos = 0;
         /* FALLTHROUGH */

      case KEY_REDRAW:
      case KEY_REDRAW2:
         usart_putc('\n');
         need_prompt = 1;
         break;

      case KEY_HOME:
         usart_putcn(KEY_BACKSPACE, precursor_length);
         cursor_pos = input_length;
         break;

      case KEY_END:
         input[input_length] = '\0';
         usart_puts(input + precursor_length);
         cursor_pos = 0;
         break;

      case KEY_UP:
         if (history_current <= history_oldest)
            break;
         if (history_current >= history_latest)
            strncpy(history[history_latest & CMDLINE_HISTORY_INDEX_MASK], input,
                  CMDLINE_INPUT_SIZE);
         strncpy(input, history[--history_current & CMDLINE_HISTORY_INDEX_MASK],
               CMDLINE_INPUT_SIZE);
         clear_input();
         input_length = strlen(input);
         break;

      case KEY_DOWN:
         if (history_current >= history_latest)
            break;
         strncpy(input, history[++history_current & CMDLINE_HISTORY_INDEX_MASK],
               CMDLINE_INPUT_SIZE);
         clear_input();
         input_length = strlen(input);
         break;

      case KEY_RIGHT:
         if (cursor_pos == 0)
            break;
         usart_putc(input[precursor_length]);
         cursor_pos--;
         break;

      case KEY_LEFT:
         if (precursor_length == 0)
            break;
         usart_putc(KEY_BACKSPACE);
         cursor_pos++;
         break;

      case KEY_CLEAR_HOME:
         for (unsigned int i = 0; i < cursor_pos; i++)
            input[i] = input[precursor_length + i];
         usart_putcn(KEY_BACKSPACE, precursor_length);
         input[cursor_pos] = '\0';
         usart_puts(input);
         usart_putcn(' ', precursor_length);
         usart_putcn(KEY_BACKSPACE, input_length);
         input_length = cursor_pos;
         break;

      case KEY_CLEAR_END:
         usart_putcn(' ', cursor_pos);
         usart_putcn(KEY_BACKSPACE, cursor_pos);
         input_length = precursor_length;
         cursor_pos = 0;
         break;

      case KEY_DELETE:
      case KEY_BACKSPACE:
         if (precursor_length == 0)
            break;
         for (unsigned int i = precursor_length - 1; i < input_length; i++)
            input[i] = input[i + 1];
         input[--input_length] = '\0';
         usart_putc(KEY_BACKSPACE);
         usart_puts(input + precursor_length - 1);
         usart_putc(' ');
         usart_putcn(KEY_BACKSPACE, cursor_pos + 1);
         break;

      case KEY_DEL:
         if (cursor_pos == 0)
            break;
         for (unsigned int i = precursor_length; i < input_length; i++)
            input[i] = input[i + 1];
         input[--input_length] = '\0';
         usart_puts(input + precursor_length);
         usart_putc(' ');
         usart_putcn(KEY_BACKSPACE, cursor_pos--);
         break;

      case KEY_DELETE_WORD:
         if (precursor_length > 0) {
            unsigned int i, del_length;
            int is_word = 0;
            for (i = precursor_length; i > 0; i--) {
               if (is_word && input[i - 1] == ' ')
                  break;
               if (input[i - 1] != ' ')
                  is_word = 1;
            }
            del_length = precursor_length - i;
            for (; i < precursor_length; i++)
               input[i] = input[i + del_length];
            input_length -= del_length;
            input[input_length] = '\0';
            usart_putcn(KEY_BACKSPACE, precursor_length);
            usart_puts(input);
            usart_putcn(' ', del_length);
            usart_putcn(KEY_BACKSPACE, del_length + cursor_pos);
         }
         break;

      case KEY_DELETE_LINE:
         clear_input();
         break;

      case KEY_TAB:
      case KEY_BEEP:
         usart_putc(KEY_BEEP);
         break;

      case KEY_LITERAL:
         escape = ESC_LITERAL;
         return 0;

      default:
         if (c < ' ' || c > '~')
            break;
literal_input:
         if (input_length >= CMDLINE_MAX_INPUT)
            break;
         for (unsigned int i = input_length; i > precursor_length; i--)
            input[i] = input[i - 1];
         input[++input_length] = '\0';
         input[precursor_length] = c;
         usart_puts(input + precursor_length);
         usart_putcn(KEY_BACKSPACE, cursor_pos);
         break;
   }

   escape = ESC_NONE;
   return rc;
}

int cmdline_get_argv(char *cmd, int argvlen, const char *argv[]) {
   int argc = 0, is_arg = 0;

   for (char c = *cmd; c != '\0'; c = *(++cmd)) {
      if (c != ' ' && !is_arg) {
         if (argc >= argvlen)
            return -1;
         is_arg = 1;
         argv[argc++] = cmd;
         continue;
      }

      if (c == ' ' && is_arg) {
         is_arg = 0;
         *cmd = '\0';
      }
   }
   return argc;
}

void cmdline_task(void *arg) {
   char *command = NULL;
   int argc;
   const char *argv[CMDLINE_MAX_ARGC];

   if (cmdline_get_input(CMDLINE_PROMPT, &command) != 0 || command == NULL)
      return;

   argc = cmdline_get_argv(command, CMDLINE_MAX_ARGC, argv);
   if (argc < 1) {
      if (argc == -1)
         printf("Too many arguments!\n");
      return;
   }

   for (cmd_t *cmd = commands; cmd->name != NULL; cmd++) {
      if (strcmp(cmd->name, argv[0]) == 0) {
         cmd->cmd(argc, argv);
         return;
      }
   }

   printf("Unrecognized command \"%s\". Try \"help\".\n", argv[0]);
}
