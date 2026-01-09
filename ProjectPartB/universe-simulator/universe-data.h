#ifndef UNIVERSE_DATA_H
#define UNIVERSE_DATA_H

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define PLANET_RADIUS 20
#define PLANET_MASS 10
#define TRASH_MASS 1
#define SHIP_MASS 1
#define G 1.0f


typedef struct 
{ 
    float amplitude; 
    float angle; 
} vector;

struct planet_stucture 
{ 
    char name[3]; 
    float x; 
    float y; 
    int mass;
    int isrecycle;
    int score; 
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
    char name[3];
    float x; 
    float y; 
    int capacity;
    int ID;
    int mass;
    vector velocity; 
    vector acceleration;
    int input;
};


//This struct is used to pass variables into threads created by the server
typedef struct 
{
    struct planet_stucture *planets;
    int n_planets;
    struct trash_stucture *trash;
    int max_trash;
    struct trash_ship *ship;
    int universe_dimensions;
    pthread_mutex_t mutex;
    int stop_flag;
} UniverseContext;

//This struct is used to pass variables into threads created by the client
typedef struct {
    struct planet_stucture *planets;
    int n_planets;
    struct trash_stucture *trash;
    int max_trash;
    struct trash_ship *ship;
    int n_ships;
    int universe_dimensions;
    pthread_mutex_t mutex;
    int stop_flag;
} ClientContext;

void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions,int max_trash, struct trash_ship ship[]);
int update_trash_count(struct trash_stucture trash[], int max_trash);
int find_ship_index_by_id(struct trash_ship ship[], int n, int id);
int allocate_ship_slot(struct trash_ship ship[], int n, int universe_dimensions);
int are_all_ships_empty(struct trash_ship ship[], int n);

#endif 
    