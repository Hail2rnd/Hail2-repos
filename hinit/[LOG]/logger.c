#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <time.h>

#include "log.h"

#define LOGGER_MAX_LINES 4096
#define LOGGER_LINE_SIZE 512
#define LOGGER_REFRESH_MS 250


static char lines[
    LOGGER_MAX_LINES
][LOGGER_LINE_SIZE];

static size_t line_count = 0;
static size_t top_line = 0;

static struct termios old_terminal;
static int terminal_raw = 0;


static void logger_terminal_restore(void)
{
    if (!terminal_raw)
    {
        return;
    }

    tcsetattr(
        STDIN_FILENO,
        TCSAFLUSH,
        &old_terminal
    );

    terminal_raw = 0;

    printf(
        "\033[?25h"
    );

    fflush(stdout);
}


static int logger_terminal_raw(void)
{
    struct termios raw;


    if (tcgetattr(
            STDIN_FILENO,
            &old_terminal
        ) < 0)
    {
        return -1;
    }


    raw = old_terminal;


    raw.c_lflag &= ~(
        ICANON |
        ECHO
    );


    raw.c_iflag &= ~(
        IXON |
        ICRNL
    );


    raw.c_oflag |= OPOST;


    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;


    if (tcsetattr(
            STDIN_FILENO,
            TCSAFLUSH,
            &raw
        ) < 0)
    {
        return -1;
    }


    terminal_raw = 1;


    printf(
        "\033[?25l"
    );


    fflush(stdout);


    return 0;
}


static void logger_clear_screen(void)
{
    printf(
        "\033[2J"
        "\033[H"
    );
}


static void logger_trim_newline(char *line)
{
    size_t length;


    if (line == NULL)
    {
        return;
    }


    length = strlen(line);


    while (length > 0 &&
           (line[length - 1] == '\n' ||
            line[length - 1] == '\r'))
    {
        line[length - 1] = '\0';
        length--;
    }
}


static int logger_load_file(
    FILE *file
)
{
    char buffer[LOGGER_LINE_SIZE];


    if (file == NULL)
    {
        return -1;
    }


    rewind(file);


    line_count = 0;


    while (fgets(
        buffer,
        sizeof(buffer),
        file
    ) != NULL)
    {
        logger_trim_newline(buffer);


        if (line_count < LOGGER_MAX_LINES)
        {
            snprintf(
                lines[line_count],
                LOGGER_LINE_SIZE,
                "%s",
                buffer
            );

            line_count++;
        }
        else
        {
            /*
             * Keep the newest entries.
             */
            memmove(
                lines[0],
                lines[1],
                (LOGGER_MAX_LINES - 1) *
                    sizeof(lines[0])
            );


            snprintf(
                lines[LOGGER_MAX_LINES - 1],
                LOGGER_LINE_SIZE,
                "%s",
                buffer
            );
        }
    }


    fseek(
        file,
        0,
        SEEK_END
    );


    return 0;
}


static size_t logger_visible_lines(void)
{
    struct winsize size;


    memset(
        &size,
        0,
        sizeof(size)
    );


    if (ioctl(
            STDOUT_FILENO,
            TIOCGWINSZ,
            &size
        ) < 0)
    {
        return 20;
    }


    if (size.ws_row <= 2)
    {
        return 1;
    }


    return size.ws_row - 2;
}


static void logger_draw(void)
{
    size_t visible;
    size_t i;
    size_t end;


    visible = logger_visible_lines();


    if (line_count == 0)
    {
        top_line = 0;
    }
    else if (top_line >= line_count)
    {
        top_line = line_count - 1;
    }


    logger_clear_screen();


    printf(
        " Hail2 Logger  |  %zu logs\n",
        line_count
    );


    printf(
        " ────────────────────────────────────────────────\n"
    );


    end = top_line + visible;


    if (end > line_count)
    {
        end = line_count;
    }


    for (i = top_line; i < end; i++)
    {
        printf(
            "%s\n",
            lines[i]
        );
    }


    /*
     * Fill the remaining terminal space.
     */
    for (; i < top_line + visible; i++)
    {
        putchar('\n');
    }


    printf(
        "\033[7m"
        " ↑↓ Navigate   PgUp/PgDn Scroll   Home/End   q Quit "
        "\033[0m"
    );


    fflush(stdout);
}


static void logger_scroll_up(size_t amount)
{
    if (top_line >= amount)
    {
        top_line -= amount;
    }
    else
    {
        top_line = 0;
    }
}


static void logger_scroll_down(size_t amount)
{
    size_t visible;
    size_t maximum;


    visible = logger_visible_lines();


    if (line_count <= visible)
    {
        top_line = 0;
        return;
    }


    maximum = line_count - visible;


    if (top_line + amount >= maximum)
    {
        top_line = maximum;
    }
    else
    {
        top_line += amount;
    }
}


static void logger_home(void)
{
    top_line = 0;
}


static void logger_end(void)
{
    size_t visible;


    visible = logger_visible_lines();


    if (line_count > visible)
    {
        top_line = line_count - visible;
    }
    else
    {
        top_line = 0;
    }
}


static int logger_read_key(void)
{
    unsigned char key;


    if (read(
            STDIN_FILENO,
            &key,
            1
        ) != 1)
    {
        return 0;
    }


    if (key == 'q' ||
        key == 'Q')
    {
        return 'q';
    }


    if (key == '\033')
    {
        unsigned char sequence[3];
        ssize_t length;


        length = read(
            STDIN_FILENO,
            sequence,
            sizeof(sequence)
        );


        if (length >= 2 &&
            sequence[0] == '[')
        {
            if (sequence[1] == 'A')
            {
                return 1001;
            }


            if (sequence[1] == 'B')
            {
                return 1002;
            }


            if (sequence[1] == 'H')
            {
                return 1005;
            }


            if (sequence[1] == 'F')
            {
                return 1006;
            }


            if (sequence[1] == '5' &&
                length >= 3 &&
                sequence[2] == '~')
            {
                return 1003;
            }


            if (sequence[1] == '6' &&
                length >= 3 &&
                sequence[2] == '~')
            {
                return 1004;
            }
        }


        return 0;
    }


    if (key == 'g')
    {
        logger_home();
        return 1;
    }


    if (key == 'G')
    {
        logger_end();
        return 1;
    }


    return 0;
}


static int logger_process_key(
    int key
)
{
    size_t visible;


    visible = logger_visible_lines();


    switch (key)
    {
        case 'q':
            return 1;


        case 1001:
            logger_scroll_up(1);
            break;


        case 1002:
            logger_scroll_down(1);
            break;


        case 1003:
            logger_scroll_up(
                visible > 1
                    ? visible - 1
                    : 1
            );
            break;


        case 1004:
            logger_scroll_down(
                visible > 1
                    ? visible - 1
                    : 1
            );
            break;


        case 1005:
            logger_home();
            break;


        case 1006:
            logger_end();
            break;


        default:
            return 0;
    }


    return 2;
}


int main(void)
{
    FILE *log_file;
    int key;
    int result;


    log_file = fopen(
        LOG_FILE,
        "r"
    );


    if (log_file == NULL)
    {
        fprintf(
            stderr,
            "logger: cannot open %s: %s\n",
            LOG_FILE,
            strerror(errno)
        );


        return EXIT_FAILURE;
    }


    if (logger_terminal_raw() < 0)
    {
        fprintf(
            stderr,
            "logger: cannot configure terminal: %s\n",
            strerror(errno)
        );


        fclose(log_file);

        return EXIT_FAILURE;
    }


    atexit(
        logger_terminal_restore
    );


    logger_load_file(
        log_file
    );


    logger_end();


    logger_draw();


    for (;;)
    {
        key = logger_read_key();


        if (key != 0)
        {
            result = logger_process_key(
                key
            );


            if (result == 1)
            {
                break;
            }


            logger_draw();
        }


        /*
         * Reload the file so new log entries
         * appear automatically.
         */
        {
            long position;


            position = ftell(log_file);


            if (logger_load_file(log_file) < 0)
            {
                break;
            }


            /*
             * When the viewer was already at the
             * bottom, follow newly appended logs.
             */
            if (position >= 0)
            {
                size_t visible;


                visible = logger_visible_lines();


                if (line_count > visible &&
                    top_line + visible >= line_count - 1)
                {
                    logger_end();
                }
            }
        }


        logger_draw();


        {
            struct timespec delay;

            delay.tv_sec = 0;
            delay.tv_nsec =
                LOGGER_REFRESH_MS * 1000000L;

            nanosleep(
                &delay,
                NULL
            );
        }
    }


    fclose(log_file);


    return EXIT_SUCCESS;
}
