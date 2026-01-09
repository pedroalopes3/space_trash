#include "universe-data.h"
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
        ship[i].mass = SHIP_MASS;
        ship[i].input = 0;
        snprintf(ship[i].name, sizeof(ship[i].name), "%c%d", 'A' + (i % 26), i / 26);
    }

    for (int i = 0; i < n_of_planets; i++) 
    {
        planets[i].x = rand() % universe_dimensions;
        planets[i].y = rand() % universe_dimensions;
        planets[i].mass = PLANET_MASS;
        snprintf(planets[i].name, sizeof(planets[i].name), "%c%d", 'A' + (i % 26), i / 26);
        planets[i].isrecycle = 0;
        planets[i].score = 0;
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

//Goes through each position of the ship array to find the index of the ship with the given ID
int find_ship_index_by_id(struct trash_ship ship[], int n, int id)
{
    for (int i = 0; i < n; i++) 
    {
        if (ship[i].ID == id) 
        {
            return i;
        }
    }
    return -1;
}

//Finds the first available slot in the ship array, assigns it a random unique ID and gives the ship a random position
int allocate_ship_slot(struct trash_ship ship[], int n, int universe_dimensions)
{
    int free_idx = -1;

    for (int i = 0; i < n; i++) 
    {
        if (ship[i].ID == 0) 
        {
            free_idx = i;
            break;
        }
    }

    if (free_idx < 0) 
    return -1;

    int new_id = 0;
    int attempts = 0;

    do 
    {
        new_id = (rand() % 1000000) + 1; 
        attempts++;
    } while (find_ship_index_by_id(ship, n, new_id) >= 0 && attempts < 16);

    if (find_ship_index_by_id(ship, n, new_id) >= 0) 
    {
        return -1; 
    }

    ship[free_idx].ID = new_id;
    ship[free_idx].x = rand() % universe_dimensions;
    ship[free_idx].y = rand() % universe_dimensions;
    ship[free_idx].velocity.amplitude = 0;
    ship[free_idx].velocity.angle = 0;
    ship[free_idx].input = 0;
    return free_idx;
}

// Returns 0 if all ship IDs are 0, returns 1 otherwise
int are_all_ships_empty(struct trash_ship ship[], int n)
{
    for (int i = 0; i < n; i++) {
        if (ship[i].ID != 0) {
            return 1;
        }
    }
    return 0;
}