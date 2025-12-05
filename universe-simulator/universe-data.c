#include "universe-data.h"
#include <time.h>

//Initilializes planets and trash with random positions; randomly picks a recycle planet; "enables" the initial amount of trash
void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions, int max_trash)
{
    srand((unsigned)time(NULL));
    
    for (int i = 0; i < n_of_planets; i++) 
    {
        planets[i].x = rand() % universe_dimensions;
        planets[i].y = rand() % universe_dimensions;
        planets[i].mass = PLANET_MASS;
        planets[i].name = 'A' + (i % 26);
        planets[i].isrecycle = 0;
    }
    
    planets[rand() % n_of_planets].isrecycle = 1; 

    for (int i = 0; i < max_trash; i++) 
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

//Simply counts how many trash are "enabled"
int update_trash_count(struct trash_stucture trash[], int max_trash)
{
    int count = 0;
    for (int i = 0; i < max_trash; i++) 
    {
        if (trash[i].status == 1)
        {
            count++;
        }
    }
    return count;
}