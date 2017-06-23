#include <stdio.h>
#include <ctype.h>

typedef struct {
    char ch;
    int num;
} mix_t;

void main(int argc, char *argv[]) {
    char temp[10] = {'a', 1, 2, 3, 4, 'b', 6, 7, 8, 9}, *i_temp = temp;
    mix_t mix[10], *i_mix = mix;
    printf("Initialized with {'a', 1, 2, 3, 4, 'b', 6, 7, 8, 9}:\n");
    printf("% 10s\t% 10s\n", "characters", "numbers");
    for (int i = 0; i < 10; i ++, i_temp ++, i_mix ++) {
        if (isalpha(*i_temp)) {
            i_mix->ch = *i_temp;
            i_mix->num = 0;
	} else {
            i_mix->ch = '\0';
            i_mix->num = *i_temp;
	}
        printf("% 10c\t% 10d\n", i_mix->ch, (i_mix)->num);
    }
    return;
}
