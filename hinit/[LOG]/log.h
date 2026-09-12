#ifndef HAIL2_LOG_H
#define HAIL2_LOG_H

#define LOG_FILE "/var/log/logs.log"

#define LOG_LEVEL_LOG      0
#define LOG_LEVEL_WARN     1
#define LOG_LEVEL_CRITICAL 2
#define LOG_LEVEL_FATAL    3


int log_init(void);

int log_write(
    const char *component,
    const char *message,
    int level
);

void log_close(void);


#endif
