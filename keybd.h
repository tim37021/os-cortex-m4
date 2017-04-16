#ifndef _KEYBD_H_
#define _KEYBD_H_

typedef void (*write_row)(int index, int value);
typedef int (*read_col)(int index);
typedef void (*delay)(int);

typedef struct IOInterface {
    write_row write_row;
    read_col read_col;
} IOInterface;

typedef enum KeyEvent {
    UNCHANGED,
    KEY_DOWN,
    KEY_UP
} KeyEvent;

void init_keybd(const IOInterface *interface, int rows, int cols);
// row major
void scan_keybd(const IOInterface *interface, int rows, int cols, int result[*][cols]);
void update_keybd_event(int rows, int cols, int last_result[*][cols], int result[*][cols], KeyEvent event[*][cols]);

#endif