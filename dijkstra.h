#ifndef DIJKSTRA_H

typedef struct DijkstraMap {
    int* map;
    int sizeX;
    int sizeY;
} DijkstraMap;

DijkstraMap* dijkstra_map_init(int sizeX, int sizeY);
void dijkstra_map_set_impassable(DijkstraMap* dm, int x, int y);
void dijkstra_map_set_target(DijkstraMap*, int targetX, int targetY);
void dijkstra_map_free(DijkstraMap* dm);
int dijkstra_map_val(DijkstraMap* dm, int x, int y);

#define DIJKSTRA_H
#endif