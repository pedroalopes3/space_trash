#include "Universe-data.h"
#include <time.h>

//Initilializes planets,trash and ships with random positions; randomly picks a recycle planet; "enables" the initial amount of trash
void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions, int max_trash, struct trash_ship ship[])
{
    srand((unsigned)time(NULL));
    
    for (int i = 0; i < n_of_planets; i++) 
    {
        ship[i].x = rand() % universe_dimensions;
        ship[i].y = rand() % universe_dimensions;
        ship[i].capacity = 0;
        ship[i].ID = 0;
    }

    for (int i = 0; i < n_of_planets; i++) 
    {
        planets[i].x = rand() % universe_dimensions;
        planets[i].y = rand() % universe_dimensions;
        planets[i].mass = PLANET_MASS;
        planets[i].name = 'A' + (i % 26);
        planets[i].isrecycle = 0;
    }
    
    planets[rand() % n_of_planets].isrecycle = 1; 

    for (int i = 0; i < initial_trash; i++) 
    {
        trash[i].x = rand() % universe_dimensions;
        trash[i].y = rand() % universe_dimensions;
        trash[i].mass = TRASH_MASS;
        trash[i].velocity.amplitude = 0;
        trash[i].velocity.angle = 0;
        trash[i].acceleration.amplitude = 0;
        trash[i].acceleration.angle = 0;
        trash[i].status = 0;
    }

    for (int i = 0; i < initial_trash; i++) 
    {
        trash[i].status = 1;
    }
}

//Checks collisions between ships and planets/trash: If a ship has an ID different than 0 (AKA is being used by a client)
//it checks it's distance to all planets; if it's less than 10, it "unloads" the ship if it's on a recycle planet, or throws it all
//back to the universe otherwise. Then, it checks the distance to all trash; if it's less than 10, it "collects" the trash
//if the ship isn't at full capacity yet.
void check_collisions(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash, int universe_dimensions,struct trash_ship ship[],int max_capacity) {
    
    for(int i = 0;i < total_planets;i++)
    {
        if(ship[i].ID)
        {
            for (int j = 0;j < total_planets;j++)
            {
                if(sqrt(pow(ship[i].x - planets[j].x, 2) + pow(ship[i].y - planets[j].y, 2)) < 10)
                {
                    if(!planets[j].isrecycle)
                    {
                        for (int k = 0; k < ship[i].capacity; k++) 
                        {
                            add_trash(trash, total_trash,universe_dimensions);
                        }
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

//Enables trash on a new random position. Called after a ship crashes onto a non-recycle planet
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
