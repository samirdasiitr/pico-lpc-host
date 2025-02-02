#ifndef _DEBUG_H
#define _DEBUG_H

#define DEBUG_BUFFER_SIZE 4096

void debug_init(uint baudrate, char *buffer, bool is_enabled);

void debug_reinit(void);

void debug(const char *format, ...);

void debug_block(const char *format, ...);

bool debug_is_enabled(void);

#endif