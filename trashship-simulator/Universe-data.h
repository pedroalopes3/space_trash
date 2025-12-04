#ifndef UNIVERSE_DATA_H
#define UNIVERSE_DATA_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#define PLANET_RADIUS 20
#define PLANET_MASS 10
#define TRASH_MASS 1
#define G 1.0f

typedef struct 
{ 
    float amplitude; 
    float angle; 
} vector;

struct planet_stucture 
{ 
    char name; 
    float x; 
    float y; 
    int mass;
    int isrecycle; 
};

struct trash_stucture 
{ 
    float x; 
    float y; 
    int mass; 
    vector velocity; 
    vector acceleration;
    int status; 
};

struct trash_ship
{ 
    float x; 
    float y; 
    int capacity;
    int connected; 
    int ID;
};
void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions,int max_trash, struct trash_ship ship[]);

void check_collisions(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash, int universe_dimensions,struct trash_ship ship[], int ship_capacity);

void add_trash(struct trash_stucture trash[], int max_trash, int universe_dimensions);

#endif 
    