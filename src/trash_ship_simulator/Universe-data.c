#include "Universe-data.h"
#include <time.h>

void universe_data_init(struct planet_stucture planets[], int n_of_planets,struct trash_stucture trash[], int initial_trash, int universe_dimensions)
{
    srand((unsigned)time(NULL));
    
    for (int i = 0; i < n_of_planets; i++) 
    {
        planets[i].x = rand() % universe_dimensions;
        planets[i].y = rand() % universe_dimensions;
        planets[i].mass = PLANET_MASS;
        planets[i].name = 'A' + (i % 26);
    }

    for (int i = 0; i < initial_trash; i++) 
    {
        trash[i].x = rand() % universe_dimensions;
        trash[i].y = rand() % universe_dimensions;
        trash[i].mass = TRASH_MASS;
        trash[i].velocity.amplitude = 0;
        trash[i].velocity.angle = 0;
        trash[i].acceleration.amplitude = 0;
        trash[i].acceleration.angle = 0;
    }
}