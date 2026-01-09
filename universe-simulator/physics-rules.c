#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "physics-rules.h"

vector make_vector(float x, float y) 
{
    vector v;
    v.amplitude = sqrtf(x*x + y*y);
    v.angle = atan2f(y, x);
    return v;
}

vector add_vectors(vector a, vector b) 
{
    float ax = a.amplitude * cosf(a.angle);
    float ay = a.amplitude * sinf(a.angle);
    float bx = b.amplitude * cosf(b.angle);
    float by = b.amplitude * sinf(b.angle);
    return make_vector(ax + bx, ay + by);
}

vector scale_vector(vector v, float s) 
{
    v.amplitude *= s;
    return v;
}

//Function that allows objects to "wrap around" the universe borders
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

void new_ship_acceleration(struct planet_stucture planets[], int total_planets,struct trash_ship ship[]){ 
    vector total_vector_force;


    for (int n_ships = 0; n_ships < total_planets; n_ships ++){
        if (ship[n_ships].ID)
        {
            total_vector_force.amplitude = 0;
            total_vector_force.angle = 0;
            for (int n_planet = 0; n_planet < total_planets; n_planet ++){
                float force_vector_x = planets[n_planet].x - ship[n_ships].x;
                float force_vector_y = planets[n_planet].y - ship[n_ships].y;
                vector local_vector_force = make_vector(force_vector_x, force_vector_y);
                local_vector_force.amplitude = (planets[n_planet].mass * ship[n_ships].mass)/
                                                pow(local_vector_force.amplitude, 2);
                total_vector_force = add_vectors(local_vector_force, total_vector_force);
        }
            ship[n_ships].acceleration = total_vector_force ; // / trash[n_trash].mass
        } 
        
    }
}

void new_ship_velocity(struct trash_ship ship[], int total_planets){
    
    for (int n_ships = 0; n_ships < total_planets; n_ships ++){
        if (ship[n_ships].ID)
        {
            ship[n_ships].velocity.amplitude *= 0.99; //friction
            ship[n_ships].velocity = add_vectors(ship[n_ships].velocity, ship[n_ships].acceleration);    
        }
    }
}

void new_ship_position(struct trash_ship ship[], int total_planets,int dims){
    for (int n_ships = 0; n_ships < total_planets; n_ships ++){
        if (ship[n_ships].ID)
        {
            ship[n_ships].x += ship[n_ships].velocity.amplitude * cos( ship[n_ships].velocity.angle);
            ship[n_ships].y += ship[n_ships].velocity.amplitude * sin( ship[n_ships].velocity.angle);;
            correct_position(&ship[n_ships].x, dims);
            correct_position(&ship[n_ships].y, dims);
        }
       
    }
}

//Checks all collisions within the universe: It goes through each trash;
//if a trash is enabled, it checks it's distance to all planets; if it's less than 1, it "enables" a new trash
void check_collisions(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash, int universe_dimensions,struct trash_ship ship[],int max_capacity) {
    
    for(int i = 0;i < total_planets;i++)
    {
        if(ship[i].ID)
        {
            for (int j = 0;j < total_planets;j++)
            {
               
                if(sqrt(pow(ship[i].x - planets[j].x, 2) + pow(ship[i].y - planets[j].y, 2)) < 10)
                {
                    // Nao e reciclegem lixo espalha
                    if(!planets[j].isrecycle)
                    {
                        for (int k = 0; k < ship[i].capacity; k++) {
                            add_trash(trash, total_trash, universe_dimensions);
                        }
                    }
                    // Se for reciclegem esvazia a nava
                    else 
                    {
                        planets[j].recycled_trash += ship[i].capacity;
                    }

                    ship[i].capacity = 0; 
                }
                
            }
            
            for (int j = 0;j < total_trash;j++)
            {
                if(trash[j].status == 1)
                {
                    if(sqrt(pow(ship[i].x - trash[j].x, 2) + pow(ship[i].y - trash[j].y, 2)) < 10)
                    {
                        if(ship[i].capacity < max_capacity)
                        {
                            ship[i].capacity += 1;
                            trash[j].status = 0;
                        }
                    }
                }
            }
        }
                    
    }          
}

//All emcopassing function that updates the physics of the universe
void physics_update(struct planet_stucture planets[], int n_of_planets, struct trash_stucture trash[], int initial_trash, int universe_dimensions, struct trash_ship ship[],int ship_capacity)
{
    new_ship_acceleration(planets,n_of_planets,ship);
    new_ship_velocity(ship,n_of_planets);
    new_ship_position(ship,n_of_planets,universe_dimensions);
    new_trash_acceleration(planets,n_of_planets,trash,initial_trash);
    new_trash_velocity(trash,initial_trash);
    new_trash_position(trash,initial_trash,universe_dimensions);
    check_collisions(planets,n_of_planets,trash,initial_trash,universe_dimensions,ship,ship_capacity);
}

void add_trash(struct trash_stucture trash[], int max_trash, int universe_dimensions) 
{
    for (int i = 0; i < max_trash; i++) 
    {
        if (trash[i].status == 0) 
        {
            trash[i].x = rand() % universe_dimensions;
            trash[i].y = rand() % universe_dimensions;
            trash[i].mass = TRASH_MASS;
            trash[i].velocity.amplitude = 0;
            trash[i].velocity.angle = 0;
            trash[i].acceleration.amplitude = 0;
            trash[i].acceleration.angle = 0;
            trash[i].status = 1;
            break;
        }
    }
}