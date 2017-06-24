#include <stdio.h>

void main(int argc, char *argv[]) {
    char temp[10] = {'a', 0, 0, 0, 0, 'b', 0, 0, 0, 0};
    struct {
        char ch;
        int num;
    } *fix = temp;
    printf("Source array = {'a', 0, 0, 0, 0, 'b', 0, 0, 0, 0}\n");
    printf("Size: temp = %db\n", sizeof temp);
    printf("Size: fix = %db, fix.ch = %db, fix.num = %db\n", sizeof *fix,
            sizeof fix->ch, sizeof fix->num);
    printf("Zero shift: fix.ch = %c, fix.num = %d\n", fix->ch, fix->num);
    fix ++;
    printf("First shift: fix.ch = %c, fix.num = %d\n", fix->ch, fix->num);
    return;
}
