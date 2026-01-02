#ifndef PHYSICS_RULES_H
#define PHYSICS_RULES_H

#include "universe-data.h"

void correct_position(float *coord, int universe_dimensions);
void new_trash_acceleration(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash);
void new_trash_velocity(struct trash_stucture trash[], int total_trash);
void new_trash_position(struct trash_stucture trash[], int total_trash, int universe_dimensions);
void check_collisions(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash, int universe_dimensions, struct trash_ship ship[], int ship_capacity);
void physics_update(struct planet_stucture planets[], int n_of_planets, struct trash_stucture trash[], int initial_trash, int universe_dimensions, struct trash_ship ship[],int ship_capacity);
void add_trash(struct trash_stucture trash[], int max_trash, int universe_dimensions);
void new_ship_acceleration(struct planet_stucture planets[], int total_planets,struct trash_ship ship[]);
void new_ship_velocity(struct trash_ship ship[], int total_planets);
void new_ship_position(struct trash_ship ship[], int total_planets,int dims);

#endif 