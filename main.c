#include "include/libtcod.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "dijkstra.h"

#define MAXCOMPONENTS 2048
#define MAPSIZEX 40
#define MAPSIZEY 40

int eid = 0;
int turns = 0;
int level = 0;
// FIXME: Garbage
int controlMode = 0;
// TODO: Get this from the player's body component
int playerSpeed = 1000;


// ===== UTILITY =====

int str2lines(char* str, char lines[40][40], int start, int lineLength, int max) {
    int lineNo = start;
    int startLineNo = start;
    int lineStart = 0;
    int lineEnd = 0;
    int i;
    for (i = 0; i < strlen(str); ++i) {
        if (lineNo - startLineNo > max) {
            return lineNo;
        }
        if  (str[i] == ' ') {
            if (i < lineStart + lineLength) {
                lineEnd = ++i;
            }
            else {
                memcpy(lines[lineNo++], &str[lineStart], lineEnd - lineStart);
                lineStart = lineEnd;
            }
        }
    }
    // get the last line if it's not 0
    if (lineStart < i)
        memcpy(lines[lineNo++], &str[lineStart], i - lineStart);

    return lineNo - startLineNo;
}


void print_wrap(char* message, int startx, int starty, int endx, TCOD_color_t col) {
    char lines[40][40];
    for (int i = 0; i < 40; i++) {
        strcpy(lines[i], "");
    }
    int lineNo = str2lines(message, lines, 0, endx - startx, 40);
    for (int j = 0; j < lineNo; ++j) {
        TCOD_console_set_default_foreground(NULL, TCOD_white);
        TCOD_console_print(NULL, startx, starty + j, lines[j]);
    }
}


// ===== MESSAGE LOG =====

#define MSG_NUM 40

char message_log[MSG_NUM][40];

void message_log_init() {
    for (int i = 0; i < MSG_NUM; ++i) {
        strcpy(message_log[i], "");
    }
}

void message_add(char* m) {
    char lines[MSG_NUM][40];
    int shift = str2lines(m, lines, 0, 40, 40);
    for (int i = MSG_NUM - 1; i > 0; --i) {
        strcpy(message_log[i], message_log[i-1]);
    }
    for (int j = 0; j < shift; ++j) {
        strcpy(message_log[j], lines[j]);
    }
}

void message_log_display() {
    TCOD_console_set_default_foreground(NULL, TCOD_white);
    for (int i = 0; i < MSG_NUM; ++i) {
        TCOD_console_print(NULL, 41, i, message_log[i]);
    }
}

// ===== TARGETER =====

typedef enum TargeterType {
    TT_EXAMINE, TT_SHOOT
} TargeterType;

typedef struct Targeter {
    char glyph;
    int x;
    int y;
    TCOD_color_t col;
    TargeterType type;
} Targeter;

Targeter examineTargeter;
Targeter shootTargeter;
Targeter* currentTargeter;

Targeter targeter_create(char glyph, TCOD_color_t col, TargeterType type) {
    Targeter t;
    t.glyph = glyph;
    t.x = -1;
    t.y = -1;
    t.col = col;
    t.type = type;
    return t;
}

void targeter_init() {
    examineTargeter = targeter_create('X', TCOD_green, TT_EXAMINE);
    shootTargeter = targeter_create('X', TCOD_red, TT_SHOOT);
}

void targeter_render() {
    if (!currentTargeter) {
        return;
    }
    if (currentTargeter->x >=0 && currentTargeter->y >= 0) {
        TCOD_console_set_default_foreground(NULL, currentTargeter->col);
        TCOD_console_put_char(NULL, currentTargeter->x, currentTargeter->y, currentTargeter->glyph, TCOD_BKGND_NONE);
    }
}

void _targeter_check_bounds() {
    if (currentTargeter->x < 0)
        currentTargeter->x = 0;
    if (currentTargeter->y < 0)
        currentTargeter->y = 0;
    if (currentTargeter->x > MAPSIZEX)
        currentTargeter->x = MAPSIZEX;
    if (currentTargeter->y > MAPSIZEY)
        currentTargeter->y = MAPSIZEY;
}

void targeter_move(int x, int y) {
    currentTargeter->x += x;
    currentTargeter->y += y;
    _targeter_check_bounds();
}

void targeter_set_pos(int x, int y) {
    currentTargeter->x = x;
    currentTargeter->y = y;
    _targeter_check_bounds();
}

void targeter_set(TargeterType tt) {
    switch (tt) {
        case TT_EXAMINE:
            currentTargeter = &examineTargeter;
            break;
        case TT_SHOOT:
            currentTargeter = &shootTargeter;
            break;
    }
}

// ===== ECS =====

#define MACRO_ComponentList(listname, type, var)\
    typedef struct listname {\
        type list[MAXCOMPONENTS];\
        int count;\
    } listname;\
    listname var = {{}, 0};

#define MACRO_ComponentFindById(id, cl, c)\
    for (int MACRO_i = 0; MACRO_i < cl.count; ++MACRO_i) {\
        if (cl.list[MACRO_i].eid == id)\
        c = &cl.list[MACRO_i];\
    }

typedef enum EntityType {
    ENT_NONE, ENT_PLAYER, ENT_SLIME, ENT_WALL, ENT_TREE, ENT_BULLET
} EntityType;

typedef enum SolidityType {
    ST_NONE, ST_SOLID, ST_PROJECTILE
} SolidityType;

typedef enum PresenceType {
    PT_ELSEWHERE, PT_HERE, PT_REMOVED
} PresenceType;

typedef struct PositionComponent {
    int eid;
    int x;
    int y;
    char c;
    SolidityType solid;
    TCOD_color_t colour;
    PresenceType presence;
} PositionComponent;

typedef enum AiType {
    AI_RANDOM, AI_BEELINE, AI_STRAIGHT, AI_SMART
} AiType;

typedef struct AiComponent {
    int eid;
    AiType type;
    int state;
    int speed;
    int lastMoved;
} AiComponent;

typedef struct BodyComponent {
    int eid;
    int hp;
    int maxHp;
} BodyComponent;

typedef struct Attribute {
    int val;
    int max;
} Attribute;

typedef struct AttributeComponent {
    int eid;
    Attribute strength;
    Attribute dexterity;
    Attribute charisma;
    Attribute intelligence;
    Attribute perception;
    Attribute constitution;
    Attribute fortitude;
} AttributeComponent;

typedef enum RewardType {
    RW_NONE, RW_LORE
} RewardType;

typedef struct ExaminationComponent {
    int eid;
    char baseDesc[500];
    char pDesc[500];
    char iDesc[500];
    int pTest;
    int iTest;
    RewardType reward;
} ExaminationComponent;

MACRO_ComponentList(PositionComponentList, PositionComponent, pcl);
MACRO_ComponentList(AiComponentList, AiComponent, aicl);
MACRO_ComponentList(BodyComponentList, BodyComponent, bcl);
MACRO_ComponentList(AttributeComponentList, AttributeComponent, acl);
MACRO_ComponentList(ExaminationComponentList, ExaminationComponent, excl);

void component_create_position(int eid, int x, int y, char c, bool solid, TCOD_color_t colour) {
    PositionComponent p = {eid, x, y, c, solid, colour, PT_HERE};
    pcl.list[pcl.count++] = p;
}

void component_create_ai(int eid, int type, int state, int speed) {
    AiComponent ai = {eid, type, state, speed};
    aicl.list[aicl.count++] = ai;
}

void component_create_body(int eid, int maxHp) {
    BodyComponent bc = {eid, maxHp, maxHp};
    bcl.list[bcl.count++] = bc;
}

void component_create_attribute(int eid) {
    // todo: create
}

void component_create_examination(int eid, char* baseDesc, char* pDesc, char* iDesc, int pTest, int iTest) {
    ExaminationComponent exc;
    exc.eid = eid;
    strcpy(exc.baseDesc, baseDesc);
    strcpy(exc.pDesc, pDesc);
    strcpy(exc.iDesc, iDesc);
    exc.pTest = pTest;
    exc.iTest = iTest;
    excl.list[excl.count++] = exc;
}

int entity_create_at_pos(EntityType t, int x, int y, int dir) {
    switch (t) {
        case ENT_PLAYER:
            component_create_position(eid, x, y, '@', ST_SOLID, TCOD_white);
            component_create_body(eid, 1000);
            break;
        case ENT_SLIME:
            component_create_position(eid, x, y, 'S', ST_SOLID, TCOD_light_blue);
            component_create_ai(eid, AI_SMART, 0, 1250);
            component_create_body(eid, 1);
            break;
        case ENT_WALL:
            component_create_position(eid, x, y, '#', ST_SOLID, TCOD_darker_grey);
            component_create_examination(eid, "You see an unremarkable stone wall.", "", "", 0, 0);
            break;
        case ENT_TREE:
            component_create_position(eid, x, y, '&', ST_SOLID, TCOD_darker_green);
            component_create_examination(eid, "You see a gnarled, twisted tree.", "", "", 0, 0);
            break;
        case ENT_BULLET:
            component_create_position(eid, x, y, '*', ST_PROJECTILE, TCOD_light_yellow);
            component_create_ai(eid, AI_STRAIGHT, dir, 1);
            break;
        default:
            printf("No entity definition for %i", t);
            return 0;
    }
    return eid++;
}

void helper_damage(BodyComponent* b, int damage) {
    if (b == 0 || b->hp <= 0) {
        return;
    }
    message_add("Entity takes 1 damage.");
    b->hp -= damage;
    if (b->hp <= 0) {
        PositionComponent* p = 0;
        MACRO_ComponentFindById(b->eid, pcl, p);
        p->presence = PT_REMOVED;
    }
}

void helper_move(PositionComponent* p, int x, int y) {
    int newx, newy;
    newx = p->x + x;
    newy = p->y + y;
    for (int i = 0; i < pcl.count; ++i) {
        PositionComponent* p2 = &pcl.list[i];
        if (newx == p2->x && newy == p2->y) {
            if (p->solid == ST_SOLID && p2->presence == PT_HERE && p2->solid == ST_SOLID) {
                // collision!
                BodyComponent* b = 0;
                MACRO_ComponentFindById(p2->eid, bcl, b);
                if (b > 0) {
                    helper_damage(b, 1);
                }
                return;
            } else if (p->solid == ST_PROJECTILE && p2->presence == PT_HERE && p2->solid == ST_SOLID) {
                BodyComponent* b = 0;
                MACRO_ComponentFindById(p2->eid, bcl, b);
                if (b != 0) {
                    helper_damage(b, 1);
                }
                return;
            }
        }
    }
    p->x = newx;
    p->y = newy;
}

PositionComponent* helper_find_closest(int fromX, int fromY, PositionComponent* excluding) {
    uint8_t dist;
    uint8_t diffx;
    uint8_t diffy;
    dist = MAPSIZEX + MAPSIZEY;
    PositionComponent* c = 0;
    for (int i = 0; i < pcl.count; ++i) {
        PositionComponent* p = &pcl.list[i];
        diffx = abs(fromX - p->x);
        diffy = abs(fromY - p->y);
        if (diffx + diffy < dist && p->eid != excluding->eid) {
            dist = diffx + diffy;
            c = p;
        }
    }
    return c;
}

void system_ranged_attack(int fromX, int fromY, int toX, int toY) {
    for (int i = 0; i < pcl.count; ++i) {
        PositionComponent* pc = &pcl.list[i];
        if (pc->x == toX && pc->y == toY) {
            BodyComponent* bc = 0;
            MACRO_ComponentFindById(pc->eid, bcl, bc);
            helper_damage(bc, 1);
            return;
        }
    }
    message_add("You fire and hit nothing.");
}

void system_examine(int fromX, int fromY, int toX, int toY) {
    for (int i = 0; i < pcl.count; ++i) {
        PositionComponent* pc = &pcl.list[i];
        if (pc->x == toX && pc->y == toY) {
            ExaminationComponent* exc = 0;
            MACRO_ComponentFindById(pc->eid, excl, exc);
            if (exc != 0) {
                if (exc->pTest > 0) {
                    message_add(exc->pDesc);
                }
                if (exc->iTest > 0) {
                    message_add(exc->iDesc);
                }
                message_add(exc->baseDesc);
                return;
            }
        }
    }
}

int system_player_control() {
    TCOD_key_t key;
    TCOD_sys_check_for_event(TCOD_EVENT_KEY_PRESS, &key, NULL);
    PositionComponent* p = &pcl.list[0];
    if (key.vk == TCODK_UP || key.c == 'k') {
        if (controlMode >= 1) {
            targeter_move(0, -1);
            return 0;
        }
        helper_move(p, 0, -1);
        return 1;
    }
    else if (key.vk == TCODK_DOWN || key.c == 'j') {
        if (controlMode >= 1) {
            targeter_move(0, 1);
            return 0;
        }
        helper_move(p, 0, 1);
        return 1;
    }
    else if (key.vk == TCODK_LEFT || key.c == 'h') {
        if (controlMode >= 1) {
            targeter_move(-1, 0);
            return 0;
        }
        helper_move(p, -1, 0);
        return 1;
    }
    else if (key.vk == TCODK_RIGHT || key.c == 'l') {
        if (controlMode >= 1) {
            targeter_move(1, 0);
            return 0;
        }
        helper_move(p, 1, 0);
        return 1;
    }
    else if (key.c == 'y') {
        if (controlMode >= 1) {
            targeter_move(-1, -1);
            return 0;
        }
        helper_move(p, -1, -1);
        return 1;
    }
    else if (key.c == 'u') {
        if (controlMode >= 1) {
            targeter_move(1, -1);
            return 0;
        }
        helper_move(p, 1, -1);
        return 1;
    }
    else if (key.c == 'b') {
        if (controlMode >= 1) {
            targeter_move(-1, 1);
            return 0;
        }
        helper_move(p, -1, 1);
        return 1;
    }
    else if (key.c == 'n') {
        if (controlMode >= 1) {
            targeter_move(1, 1);
            return 0;
        }
        helper_move(p, 1, 1);
        return 1;
    }
    else if (key.c == '.') {
        if (controlMode == 1) {
            system_ranged_attack(p->x, p->y, currentTargeter->x, currentTargeter->y);
        }
        if (controlMode == 2) {
            system_examine(p->x, p->y, currentTargeter->x, currentTargeter->y);
        }
        return 1;
    }
    else if (key.c == 'f') {
        if (controlMode != 1) {
            targeter_set(TT_SHOOT);
            PositionComponent* ct = helper_find_closest(p->x, p->y, p);
            targeter_set_pos(ct->x, ct->y);
            controlMode = 1;
            return 0;
        }
        else if (controlMode == 1) {
            system_ranged_attack(p->x, p->y, currentTargeter->x, currentTargeter->y);
            controlMode = 0;
            currentTargeter = 0;
            return 1;
        }
    }
    else if (key.c == 'x') {
        if (controlMode != 2) {
            targeter_set(TT_EXAMINE);
            PositionComponent* ce = helper_find_closest(p->x, p->y, p);
            targeter_set_pos(ce->x, ce->y);
            controlMode = 2;
            return 0;
        }
        else if (controlMode == 2) {
            system_examine(p->x, p->y, currentTargeter->x, currentTargeter->y);
            controlMode = 0;
            currentTargeter = 0;
            return 1;
        }
    }
    return 0;
}

void system_render() {
    for (int i = 0; i < pcl.count; ++i) {
        if (pcl.list[i].c && pcl.list[i].presence == PT_HERE) {
            TCOD_console_set_default_foreground(NULL, pcl.list[i].colour);
            TCOD_console_put_char(NULL, pcl.list[i].x, pcl.list[i].y, pcl.list[i].c, TCOD_BKGND_NONE);
        }
    }
}

void helper_ai_smart(PositionComponent* p, PositionComponent* target) {
    int lowest = 9999;
    int targetX = 0;
    int targetY = 0;
    int val;
    int closest = 9999;
    // todo: Some form of caching for maps? Reusable amongst all enemies, for example.
    DijkstraMap* dm = dijkstra_map_init(MAPSIZEX, MAPSIZEY);
    for (int i = 0; i < pcl.count; ++i) {
        PositionComponent* tp = &pcl.list[i];
        if (tp->solid == ST_SOLID && tp->presence == PT_HERE) {
            dijkstra_map_set_impassable(dm, tp->x, tp->y);
        }
    }
    dijkstra_map_set_target(dm, target->x, target->y);
    for (int x = p->x - 1; x <= p->x + 1; ++x) {
        for (int y = p->y - 1; y <= p->y + 1; ++y) {
            if (x == p->x && y == p->y) {
                continue;
            }
            val = dijkstra_map_val(dm, x, y);
            if (val > 0 && (val < lowest || (val == lowest && abs(x - p->x) + abs(y - p->y) < closest))) {
                lowest = val;
                closest = abs(x - p->x) + abs(y - p->y);
                targetX = x - p->x;
                targetY = y - p->y;
            }
        }
    }
    helper_move(p, targetX, targetY);
    dijkstra_map_free(dm);
}

void system_ai() {
    int xm, ym;
    for (int i = 0; i < aicl.count; ++i) {
        PositionComponent* p = 0;
        AiComponent* ai = &aicl.list[i];
        MACRO_ComponentFindById(aicl.list[i].eid, pcl, p);
        if (p != 0 && p->presence == PT_HERE) {
            ai->lastMoved++;
            if (ai->lastMoved >= ai->speed) {
                switch (ai->type) {
                    case AI_RANDOM:
                        helper_move(p, (rand() % 3) - 1, rand() % 3 - 1);
                        break;
                    case AI_BEELINE:
                        xm = pcl.list[0].x - p->x;
                        ym = pcl.list[0].y - p->y;
                        if (xm > 1) xm = 1;
                        if (xm < -1) xm = -1;
                        if (ym > 1) ym = 1;
                        if (ym < -1) ym = -1;
                        helper_move(p, xm, ym);
                        break;
                    case AI_STRAIGHT:
                        switch (ai->state) {
                            case 0: helper_move(p, 0, -1); break;
                            case 1: helper_move(p, 0, 1); break;
                            case 2: helper_move(p, -1, 0); break;
                            case 3: helper_move(p, 1, 0); break;
                        }
                        break;
                    case AI_SMART:
                        helper_ai_smart(p, &pcl.list[i]);
                        break;
                    default:
                        break;
                }
                ai->lastMoved = 0;
            }
        }
    }
}

int system_attribute_test(Attribute* a, int bonus, int difficulty) {
    if (a->val + bonus >= difficulty)
        return 1;
    return 0;
}

// ===== MAP =====

#define WORLDMAPX 16
#define WORLDMAPY 16
#define WORLDMAPZ 16
#define MAXENT 255

typedef enum MapType {
    MAP_FOREST, MAP_STREETS
} MapType;

typedef struct Map {
    int generated;
    int entityCount;
    int entityList[MAXENT];
} Map;

typedef struct Pos3d {
    int z;
    int x;
    int y;
} Pos3d;

Map worldMap[WORLDMAPZ][WORLDMAPX][WORLDMAPY];

Pos3d worldPos;

Map map_init() {
    Map m;
    for (int i = 0; i < MAXENT; i++) {
        m.generated = 0;
        m.entityCount = 0;
        m.entityList[i] = 0;
    }
    return m;
}

void world_map_init() {
    for (int z = 0; z < WORLDMAPZ; z++) {
        for (int x = 0; x < WORLDMAPX; x++) {
            for (int y = 0; y < WORLDMAPY; y++) {
                worldMap[z][x][y] = map_init();
            }
        }
    }
}

void map_add_entity(Map* m, int eid) {
    m->entityList[m->entityCount++] = eid;
}

Map map_gen(MapType mt) {
    Map m;
    switch (mt) {
        case MAP_FOREST:
            for (int x = 0; x < MAPSIZEX; x++) {
                for (int y = 0; y < MAPSIZEY; y++) {
                    if (x == 0 || y == 0 || x == MAPSIZEX - 1 || y == MAPSIZEY - 1) {
                        map_add_entity(&m, entity_create_at_pos(ENT_WALL, x, y, 0));
                    }
                    else if (rand() % 20 == 0) {
                       map_add_entity(&m, entity_create_at_pos(ENT_TREE, x, y, 0));
                    }
                }
            }
            break;
        case MAP_STREETS:
            for (int x = 0; x < MAPSIZEX; x++) {
                for (int y = 0; y < MAPSIZEY; y++) {
                    if (x == 0 || y == 0 || x == MAPSIZEX - 1 || y == MAPSIZEY - 1) {
                        map_add_entity(&m, entity_create_at_pos(ENT_WALL, x, y, 0));
                    }
                }
            }
            break;
    }
    return m;
}

void map_unload() {
    Map* m = &worldMap[worldPos.z][worldPos.x][worldPos.y];
    for (int i = 0; i < m->entityCount; ++i) {
        PositionComponent* p = 0;
        MACRO_ComponentFindById(m->entityList[i], pcl, p);
        // todo: probably should throw an error if this is not true - we have problems
        if (p != 0) {
            p->presence = PT_ELSEWHERE;
        }
    }
}

void map_load() {
    Map* m = &worldMap[worldPos.z][worldPos.x][worldPos.y];
    if (m->generated == 0) {
        // todo: determine the map type based on x,y,z location according to some randomly gened worldDescription
        *m = map_gen(MAP_FOREST);
    }
    else {
        for (int i = 0; i < 12; ++i) {
            PositionComponent* p = 0;
            MACRO_ComponentFindById(m->entityList[i], pcl, p);
            if (p != 0) {
                p->presence = PT_HERE;
            }
        }
    }
}

// ===== MAIN =====

int main() {
    srand(time(0));
    entity_create_at_pos(ENT_PLAYER, rand() % MAPSIZEX, rand() % MAPSIZEY, 0);
    world_map_init();
    targeter_init();
    message_log_init();
    worldPos.z = 8;
    worldPos.x = 8;
    worldPos.y = 8;
    map_load();
    entity_create_at_pos(ENT_SLIME, 5, 5, 0);
    int done = 0;
    TCOD_console_init_root(80, 50, "AnchorheadRL", false, TCOD_RENDERER_SDL);
    message_add("You are in a forest. Gnarled trees stick up through the hard earth like broken fingers. It is raining.");
    while (!TCOD_console_is_window_closed() && done == 0) {
        TCOD_console_clear(NULL);
        int moved = system_player_control(pcl.list);
        if (moved == 1) {
            int lastMoved = 0;
            while (lastMoved++ < playerSpeed) {
                ++turns;
                system_ai();
                // heal 1hp every 100 turns
                if (turns % 100 == 0) {
                    bcl.list[0].hp++;
                    if (bcl.list[0].hp > bcl.list[0].maxHp) {
                        bcl.list[0].hp = bcl.list[0].maxHp;
                    }
                }
            }
        }
        system_render(pcl.list);
        targeter_render();
        message_log_display();
        TCOD_console_flush();
    }
    return 0;
}