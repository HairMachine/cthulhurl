#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dijkstra.h"

int main() {
    DijkstraMap* dm = dijkstra_map_generate(40, 40, 10, 15);
    for (int y = 0; y < 40; y++) {
        for (int x = 0; x < 40; x++) {
            printf("%i", dijkstra_map_val(dm, x, y));
        }
        printf("\n");
    }
    dijkstra_map_free(dm);
    return 0;
}