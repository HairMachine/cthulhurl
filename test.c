#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int map[20][20];
int xpos, ypos;

uint8_t dijkstra_adjust_map(int x, int y) {
    int current = map[x][y];
    uint8_t changed = 0;
    for (int xi = x - 1; xi <= x + 1; xi++) {
        for (int yi = y - 1; yi <= y + 1; yi++) {
            if (xi < 0 || yi < 0 || xi > 19 || yi > 19) {
                continue;
            }
            if (map[xi][yi] - current > 1) {
                map[xi][yi] = current + 1;
                changed = 1;
            }
        }
    }
    return changed;
}

int main() {
    uint8_t changed;
    int loops;
    changed = 0;
    loops = 0;
    for (int x = 0; x < 20; x++) {
        for (int y = 0; y < 20; y++) {
            map[x][y] = 9999;
        }
    }
    xpos = 5;
    ypos = 5;
    map[xpos][ypos] = 0;
    do {
        changed = 0;
        for (int x = 0; x < 20; x++) {
            for (int y = 0; y < 20; y++) {
                if (changed == 0 && dijkstra_adjust_map(x, y) == 1) {
                    changed = 1;
                }
            }
        }
    } while (loops++ < 10000 && changed == 1);
    for (int y = 0; y < 20; y++) {
        for (int x = 0; x < 20; x++) {
            printf("%i", map[x][y]);
        }
        printf("\n");
    }
    printf("LOOPS: %i\n", loops);
    return 0;
}
