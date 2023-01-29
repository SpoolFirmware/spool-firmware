#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fix16.h"

#define MAX_LINE_LEN 100

enum GcodeType {
    G0 = 0,
    G1,
    GOther,
};

typedef struct Gcode {
    enum GcodeType type;

    fix16_t f, x, y, z, e;
} gcode_t;


gcode_t initGcode(void) {
    gcode_t g;
    memset(&g, 0, sizeof(g));
    return g;
}

void printGcode(gcode_t* p) {
    if (!p || p->type == GOther) return;
    char x[13], y[13], z[13], f[13], e[13];
    fix16_to_str(p->x, x, 10);
    fix16_to_str(p->y, y, 10);
    fix16_to_str(p->z, z, 10);
    fix16_to_str(p->f, f, 10);
    fix16_to_str(p->e, e, 10);

    printf("G%d: X%x, Y%x, Z%x, F%x, E%x\n", p->type, p->x, p->y, p->z, p->f, p->e);
    printf("G%d: X%s, Y%s, Z%s, F%s, E%s\n", p->type, x, y, z, f, e);
}


uint32_t getword(const char* input, char* token, size_t *p_input_pos, char delim) {
    size_t read_cnt = 0;
    size_t token_pos = 0;

    while (input[*p_input_pos] != '\0') {
        if (input[*p_input_pos] == delim || input[*p_input_pos] == '\n') {
            (*p_input_pos)++;
            token[token_pos] = '\0';
            return read_cnt;
        }
        read_cnt++;
        token[token_pos++] = input[*p_input_pos];
        (*p_input_pos)++;
    }
    token[token_pos] = '\0';

    return read_cnt;
}


enum GcodeType parseGcodeType(const char* str) {
    if (strcmp(str, "G0") == 0) return G0;
    if (strcmp(str, "G1") == 0) return G1;
    return GOther;
}


/*!
 * input, input_size is pointer and size of the input text buffer
 * @return number of characters consumed
 *
 */
uint32_t getGcode(const char* input, size_t input_size,
    gcode_t *pGcode) {
    size_t input_len = strlen(input);
    size_t ret = input_len < input_size ? input_len : input_size;
    size_t input_pos = 0;
    char token[1000];

    getword(input, token, &input_pos, ' ');
    enum GcodeType t = parseGcodeType(token);
    switch (t) {
    case G0:
        pGcode->type = G0;
        break;
    case 1:
        pGcode->type = G1;
        break;
    default:
        pGcode->type = GOther;
        goto exit;
    }

    while (getword(input, token, &input_pos, ' ')) {
        switch (token[0]) {
        case ';':
            goto exit;
        case 'F':
            pGcode->f = fix16_from_str(token+1);
            break;
        case 'X':
            pGcode->x = fix16_from_str(token+1);
            break;
        case 'Y':
            pGcode->y = fix16_from_str(token+1);
            break;
        case 'Z':
            pGcode->z = fix16_from_str(token+1);
            break;
        case 'E':
            pGcode->e = fix16_from_str(token+1);
            break;
        } 
    }

exit:
    return ret;
}


int main(int argc, char** argv) {
    if (argc != 2) printf("usage: [gcode filename]\n");

    char* fname = argv[1];
    FILE* fp = fopen(fname, "r");
    char line[MAX_LINE_LEN] = {0};
    if (!fp) {
        perror("open file failed\n");
        exit(EXIT_FAILURE);
        
    }
    
    while(fgets(line, MAX_LINE_LEN, fp)) {
        gcode_t gcode = initGcode();
        // printf("%s", line);
        getGcode(line, strlen(line), &gcode);
        // printGcode(&gcode);
        // puts("-----------------------");
    }

    exit(EXIT_SUCCESS);
}
