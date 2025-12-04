#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "physics-rules.h"

static inline vector make_vector(float x, float y) 
{
    vector v;
    v.amplitude = sqrtf(x*x + y*y);
    v.angle = atan2f(y, x);
    return v;
}

static inline vector add_vectors(vector a, vector b) 
{
    float ax = a.amplitude * cosf(a.angle);
    float ay = a.amplitude * sinf(a.angle);
    float bx = b.amplitude * cosf(b.angle);
    float by = b.amplitude * sinf(b.angle);
    return make_vector(ax + bx, ay + by);
}

static inline vector scale_vector(vector v, float s) 
{
    v.amplitude *= s;
    return v;
}

void correct_position(float *coord, int universe_dimensions) 
{
    if (*coord < 0) *coord += universe_dimensions;
    if (*coord >= universe_dimensions) *coord -= universe_dimensions;
}

void new_trash_acceleration(struct planet_stucture planets[], int total_planets,struct trash_stucture trash[], int total_trash){ 
    vector total_vector_force;


    for (int n_trash = 0; n_trash < total_trash; n_trash ++){
        if (trash[n_trash].status == 1)
        {
            total_vector_force.amplitude = 0;
            total_vector_force.angle = 0;
            for (int n_planet = 0; n_planet < total_planets; n_planet ++){
                float force_vector_x = planets[n_planet].x - trash[n_trash].x;
                float force_vector_y = planets[n_planet].y - trash[n_trash].y;
                vector local_vector_force = make_vector(force_vector_x, force_vector_y);
                local_vector_force.amplitude = (planets[n_planet].mass * trash[n_trash].mass)/
                                                pow(local_vector_force.amplitude, 2);
                total_vector_force = add_vectors(local_vector_force, total_vector_force);
        }
            trash[n_trash].acceleration = total_vector_force ; // / trash[n_trash].mass
        } 
        
    }
}

void new_trash_velocity(struct trash_stucture trash[], int total_trash){
    
    for (int n_trash = 0; n_trash < total_trash; n_trash ++){
        if (trash[n_trash].status == 1)
        {
            trash[n_trash].velocity.amplitude *= 0.99; //friction
            trash[n_trash].velocity = add_vectors(trash[n_trash].velocity, trash[n_trash].acceleration);    
        }
    }
}

void new_trash_position(struct trash_stucture trash[], int total_trash,int dims){
    for (int n_trash = 0; n_trash < total_trash; n_trash ++){
        if (trash[n_trash].status == 1)
        {
            trash[n_trash].x += trash[n_trash].velocity.amplitude * cos( trash[n_trash].velocity.angle);
            trash[n_trash].y += trash[n_trash].velocity.amplitude * sin( trash[n_trash].velocity.angle);;
            correct_position(&trash[n_trash].x, dims);
            correct_position(&trash[n_trash].y, dims);
        }
       
    }
}

void check_collisions(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash, int universe_dimensions) {
    for(int i = 0;i < total_trash;i++) 
    {
        if (trash[i].status == 1)
        {
             for(int j = 0;j < total_planets;j++)
            {
                if(sqrt(pow(trash[i].x - planets[j].x, 2) + pow(trash[i].y - planets[j].y, 2)) < 1)
                {
                    for (int i = 0; i < total_trash; i++) 
                    {
                        if (trash[i].status == 0)
                        {
                            trash[i].status = 1;
                            break;
                        }
                    }
                }
            }
        }
       
    }
}

void physics_update(struct planet_stucture planets[], int n_of_planets, struct trash_stucture trash[], int initial_trash, int universe_dimensions)
{
    new_trash_acceleration(planets,n_of_planets,trash,initial_trash);
    new_trash_velocity(trash,initial_trash);
    new_trash_position(trash,initial_trash,universe_dimensions);
    check_collisions(planets,n_of_planets,trash,initial_trash,universe_dimensions);
}