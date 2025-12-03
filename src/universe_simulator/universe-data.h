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
};

struct trash_stucture 
{ 
    float x; 
    float y; 
    int mass; 
    vector velocity; 
    vector acceleration; 
};

void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions);

#endif 
    