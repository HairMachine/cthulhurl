#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dijkstra.h"

void _dijkstra_map_val_set(DijkstraMap* dm, int x, int y, int val) {
    *(dm->map + (y * dm->sizeX) + x) = val;
}

int dijkstra_map_val(DijkstraMap* dm, int x, int y) {
    return *(dm->map + (y * dm->sizeX) + x);
}

uint8_t _dijkstra_adjust_map(DijkstraMap* dm, int x, int y) {
    int current = dijkstra_map_val(dm, x, y);
    if (current == -1) {
        return 0;
    }
    uint8_t changed = 0;
    for (int xi = x - 1; xi <= x + 1; xi++) {
        for (int yi = y - 1; yi <= y + 1; yi++) {
            if (xi < 0 || yi < 0 || xi > dm->sizeX - 1 || yi > dm->sizeY - 1) {
                continue;
            }
            if (dijkstra_map_val(dm, xi, yi) - current > 1) {
                _dijkstra_map_val_set(dm, xi, yi, current + 1);
                changed = 1;
            }
        }
    }
    return changed;
}

DijkstraMap* dijkstra_map_init(int sizeX, int sizeY) {
    DijkstraMap* dm = malloc(sizeof(DijkstraMap));
    dm->map = malloc(sizeof(int) * sizeX * sizeY);
    dm->sizeX = sizeX;
    dm->sizeY = sizeY;
    // initialise the map
    for (int x = 0; x < dm->sizeX; x++) {
        for (int y = 0; y < dm->sizeY; y++) {
            _dijkstra_map_val_set(dm, x, y, 9999);
        }
    }
    return dm;
}

void dijkstra_map_set_impassable(DijkstraMap* dm, int x, int y) {
    _dijkstra_map_val_set(dm, x, y, -1);
}

void dijkstra_map_set_target(DijkstraMap* dm, int targetX, int targetY) {
    uint8_t changed;
    int loops;
    loops = 0;
    // set the target
    _dijkstra_map_val_set(dm, targetX, targetY, 0);
    // build the map
    do {
        changed = 0;
        for (int x = 0; x < dm->sizeX; x++) {
            for (int y = 0; y < dm->sizeY; y++) {
                if (_dijkstra_adjust_map(dm, x, y) == 1 && changed == 0) {
                    changed = 1;
                }
            }
        }
    } while (loops++ < 500 && changed == 1);
}

void dijkstra_map_free(DijkstraMap* dm) {
    free(dm->map);
    free(dm);
}