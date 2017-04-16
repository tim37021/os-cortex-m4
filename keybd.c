#include "keybd.h"

#define HIGH 1
#define LOW 0

void init_keybd(const IOInterface *interface, int rows, int cols)
{
    for(int r=0; r<rows; r++)
        interface->write_row(r, HIGH);
}

void scan_keybd(const IOInterface *interface, int rows, int cols, int result[rows][cols])
{
    for(int r = 0; r < rows; r++) {
        interface->write_row(r, LOW);
        for(int c = 0; c < cols; c++) {
            result[r][c] = !interface->read_col(c);
        }
        interface->write_row(r, HIGH);
    }
}

void update_keybd_event(int rows, int cols, int last_result[rows][cols], int result[rows][cols], KeyEvent event[rows][cols])
{
    for(int r=0; r<rows; r++) {
        for(int c=0; c<cols; c++) {
            if(last_result[r][c]!=result[r][c]) {
                if(result[r][c])
                    event[r][c] = KEY_DOWN;
                else
                    event[r][c] = KEY_UP;
                last_result[r][c] = result[r][c];
            } else
                event[r][c] = UNCHANGED;
        }
    }
}