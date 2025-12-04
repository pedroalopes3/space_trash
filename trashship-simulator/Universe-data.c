#include "Universe-data.h"
#include <time.h>

void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions, int max_trash, struct trash_ship ship[])
{
    srand((unsigned)time(NULL));
    
    for (int i = 0; i < n_of_planets; i++) 
    {
        ship[i].x = rand() % universe_dimensions;
        ship[i].y = rand() % universe_dimensions;
        ship[i].capacity = 0;
        ship[i].connected = 0;
        ship[i].ID = i;
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

void check_collisions(struct planet_stucture planets[], int total_planets, struct trash_stucture trash[], int total_trash, int universe_dimensions,struct trash_ship ship[],int max_capacity) {
    
    for(int i = 0;i < total_planets;i++)
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

void add_trash(struct trash_stucture trash[], int max_trash, int universe_dimensions) {
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
