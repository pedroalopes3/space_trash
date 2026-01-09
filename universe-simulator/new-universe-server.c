#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <libconfig.h>
#include <pthread.h>
#include <zmq.h>
#include "universe-data.h"
#include "physics-rules.h"
#include "display.h"
#include "communication.h"
#include "messages.pb-c.h"

#define SHIP_THRUST 0.001

void * input_communication(void* shipstructarg);
static int find_ship_index_by_id(struct trash_ship ship[], int n, int id);
static int allocate_ship_slot(struct trash_ship ship[], int n, int universe_dimensions);
void send_update(CommHandle *pub, struct planet_stucture *planets, int n_planets,struct trash_stucture *trash, int max_trash,struct trash_ship *ship);
void * broadcast_universe(void *arg);

typedef struct 
{
    struct planet_stucture *planets;
    int n_planets;
    struct trash_stucture *trash;
    int max_trash;
    struct trash_ship *ship;
    int universe_dimensions;
    pthread_mutex_t lock;
} UniverseContext;

Uint32 timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 2;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

Uint32 trash_timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 3;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

Uint32 physics_timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 4;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

Uint32 planet_timer_callback(Uint32 interval, void* param){
    SDL_Event timer_event;
    SDL_zero(timer_event); 
    timer_event.type = SDL_USEREVENT;
    timer_event.user.code = 5;
    timer_event.user.data1 = NULL;
    timer_event.user.data2 = NULL;
    SDL_PushEvent(&timer_event);
    return interval;
}

int main()
{
    
    // Read and save variables from the config file
    config_t cfg;
    config_init(&cfg);

    if(!config_read_file(&cfg, "universe-simulator.conf")) 
    {
        fprintf(stderr, "Config file error\n");
        config_destroy(&cfg);
        return 1;
    }

    int universe_dimensions;
    config_lookup_int(&cfg, "universe_dimensions", &universe_dimensions);

    int n_of_planets;
    config_lookup_int(&cfg, "n_of_planets", &n_of_planets);

    int max_trash;
    config_lookup_int(&cfg, "max_trash", &max_trash);

    int initial_trash;
    config_lookup_int(&cfg, "initial_trash", &initial_trash);

    int ship_capacity;
    config_lookup_int(&cfg, "ship_capacity", &ship_capacity);

    config_destroy(&cfg);

    //Allocate space for the planet and trash structures
    struct planet_stucture *planets = malloc(sizeof(*planets) * n_of_planets);
    struct trash_stucture *trash = malloc(sizeof(*trash) * max_trash);
    struct trash_ship *ship = malloc(sizeof(*ship)*n_of_planets);
    if (!planets || !trash || !ship) 
    {
        fprintf(stderr, "Allocation failed\n");
        return 1;
    }

    universe_data_init(planets, n_of_planets, trash, initial_trash, universe_dimensions, max_trash, ship);

    SDL_Window *win = NULL;
    SDL_Renderer *rend = NULL;
    universe_display_init(&win, &rend, universe_dimensions);
    if (!win || !rend) 
    {
        fprintf(stderr, "Failed to create window/renderer\n");
        free(planets);
        free(trash);
        free(ship);
        return 1;
    }

    int close = 0;

    SDL_TimerID timer_id = 0;
    timer_id = SDL_AddTimer(33,(SDL_TimerCallback)timer_callback, NULL);
    SDL_TimerID trash_timer_id = 0;
    trash_timer_id = SDL_AddTimer(10000,(SDL_TimerCallback)trash_timer_callback, NULL);
    SDL_TimerID physics_timer_id = 0;
    physics_timer_id = SDL_AddTimer(10,(SDL_TimerCallback)physics_timer_callback, NULL);
    SDL_TimerID planet_timer_id = 0;
    planet_timer_id = SDL_AddTimer(30000,(SDL_TimerCallback)planet_timer_callback, NULL);
    
    int trash_count = initial_trash;

    UniverseContext *ctx = malloc(sizeof(UniverseContext));
    ctx->planets = planets;
    ctx->n_planets = n_of_planets;
    ctx->trash = trash;
    ctx->max_trash = max_trash;
    ctx->ship = ship;
    ctx->universe_dimensions = universe_dimensions;
    
    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        fprintf(stderr, "Mutex init failed\n");
        return 1;
    }

    pthread_t thread_id, broadcast_id;

    pthread_create(&thread_id, NULL, input_communication, (void*)ctx);
    pthread_create(&broadcast_id, NULL, broadcast_universe, (void*)ctx);

    while (!close) 
    {
        if (trash_count >= max_trash) 
        {
            close = 1;
            printf("Universe imploded from too much trash\n");
            break;
        }

        SDL_Event event;
        SDL_WaitEvent(&event);

        switch (event.type) 
        {
            case SDL_QUIT:
                close = 1; 
                break;
            case SDL_USEREVENT:
                if (event.user.code == 2) 
                {
                    draw_universe(max_trash, n_of_planets, planets, trash, rend, ship);
                }

                if (event.user.code == 3)
                {
                    int active_ships = 0;
                    for (int i = 0; i < n_of_planets; i++) {
                        if (ship[i].ID != 0) {
                        active_ships++;
                        }
                    }

                    if (active_ships > 0) {
                        add_trash(trash, max_trash, universe_dimensions);
                        trash_count = update_trash_count(trash, max_trash);
                     }
 
                }

                if(event.user.code == 4)
                {
                    pthread_mutex_lock(&ctx->lock);
                    physics_update(planets, n_of_planets, trash, max_trash, universe_dimensions, ship, ship_capacity);
                    trash_count = update_trash_count(trash, max_trash);
                    pthread_mutex_unlock(&ctx->lock);
                }

                if(event.user.code == 5)
                {
                    for (int i = 0; i < n_of_planets; i++) 
                    {
                        planets[i].isrecycle = 0;
                    }                    
                    planets[rand() % n_of_planets].isrecycle = 1;
                }

                break;
            default:
                break;
        }
    }
    destroy_universe(rend, win);
    free(planets);
    free(trash);
    free(ship);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
    return 0;
}

void * input_communication(void* arg)
{
    UniverseContext *ctx = (UniverseContext*)arg;
    int n_of_planets = ctx->n_planets;
    int universe_dimensions = ctx->universe_dimensions;
    struct trash_ship* ship = ctx->ship;

       
    CommHandle *comm = comm_server_init();
    if (!comm) 
    {
        fprintf(stderr, "Failed to init server communication\n");
        return NULL;
    }

    while(1)
    {
        uint8_t buffer[256];
        int n = comm_server_recv(comm, buffer, sizeof(buffer));
        
        if (n > 0)
        {
            // Unpack ClientMessage
            Trashship__ClientMessage *msg = trashship__client_message__unpack(NULL, n, buffer);
            if (!msg) 
            {
                fprintf(stderr, "Failed to unpack message (size=%d)\n", n);
                Trashship__ServerMessage reply_msg = TRASHSHIP__SERVER_MESSAGE__INIT;
                reply_msg.has_status = 1;
                reply_msg.status = -4;  
                size_t reply_len = trashship__server_message__get_packed_size(&reply_msg);
                uint8_t reply_buf[256];
                trashship__server_message__pack(&reply_msg, reply_buf);
                comm_server_send(comm, reply_buf, reply_len);
                continue;
            }

            uint32_t ship_id = msg->has_ship_id ? msg->ship_id : 0;
            uint32_t command = msg->has_command ? msg->command : 0;
            int32_t status = 0;
            uint32_t reply_ship_id = ship_id;
            
            Trashship__ServerMessage reply_msg = TRASHSHIP__SERVER_MESSAGE__INIT;

            // If the Client sends ship_id == 0, that it means it's a new client and a random ID needs to be assigned
            if (ship_id == 0) 
            {
                int slot = allocate_ship_slot(ship, n_of_planets, universe_dimensions);
                if (slot >= 0) 
                {
                    reply_ship_id = ship[slot].ID;
                    
                    // Send initial universe state to new client
                    Trashship__InitialSetupMessage *init_setup = malloc(sizeof(Trashship__InitialSetupMessage));
                    trashship__initial_setup_message__init(init_setup);
                    
                    // Populate planets
                    Trashship__Planet **planet_ptrs = malloc(sizeof(Trashship__Planet*) * n_of_planets);
                    for (int i = 0; i < n_of_planets; i++) {
                        planet_ptrs[i] = malloc(sizeof(Trashship__Planet));
                        trashship__planet__init(planet_ptrs[i]);
                        planet_ptrs[i]->has_x = 1;
                        planet_ptrs[i]->x = ctx->planets[i].x;
                        planet_ptrs[i]->has_y = 1;
                        planet_ptrs[i]->y = ctx->planets[i].y;
                        planet_ptrs[i]->has_isrecycle = 1;
                        planet_ptrs[i]->isrecycle = ctx->planets[i].isrecycle;
                    }
                    init_setup->planets = planet_ptrs;
                    init_setup->n_planets = n_of_planets;
                    
                    // Populate trash
                    int trash_count = update_trash_count(ctx->trash, ctx->max_trash);
                    Trashship__Trash **trash_ptrs = malloc(sizeof(Trashship__Trash*) * trash_count);
                    int trash_idx = 0;
                    for (int i = 0; i < ctx->max_trash; i++) {
                        if (ctx->trash[i].status != 0) {
                            trash_ptrs[trash_idx] = malloc(sizeof(Trashship__Trash));
                            trashship__trash__init(trash_ptrs[trash_idx]);
                            trash_ptrs[trash_idx]->has_x = 1;
                            trash_ptrs[trash_idx]->x = ctx->trash[i].x;
                            trash_ptrs[trash_idx]->has_y = 1;
                            trash_ptrs[trash_idx]->y = ctx->trash[i].y;
                            trash_ptrs[trash_idx]->has_status = 1;
                            trash_ptrs[trash_idx]->status = ctx->trash[i].status;
                            trash_idx++;
                        }
                    }
                    init_setup->trash = trash_ptrs;
                    init_setup->n_trash = trash_count;
                    
                    // Populate ships
                    Trashship__Ship **ship_ptrs = malloc(sizeof(Trashship__Ship*) * n_of_planets);
                    for (int i = 0; i < n_of_planets; i++) {
                        ship_ptrs[i] = malloc(sizeof(Trashship__Ship));
                        trashship__ship__init(ship_ptrs[i]);
                        ship_ptrs[i]->has_x = 1;
                        ship_ptrs[i]->x = ship[i].x;
                        ship_ptrs[i]->has_y = 1;
                        ship_ptrs[i]->y = ship[i].y;
                        ship_ptrs[i]->has_id = 1;
                        ship_ptrs[i]->id = ship[i].ID;
                    }
                    init_setup->ships = ship_ptrs;
                    init_setup->n_ships = n_of_planets;
                    
                    reply_msg.initial_setup = init_setup;
                    reply_msg.has_ship_id = 1;
                    reply_msg.ship_id = reply_ship_id;
                    reply_msg.has_status = 1;
                    reply_msg.status = 0;
                    
                    // Send reply with initial setup
                    size_t reply_len = trashship__server_message__get_packed_size(&reply_msg);
                    uint8_t *reply_buf = malloc(reply_len);
                    trashship__server_message__pack(&reply_msg, reply_buf);
                    comm_server_send(comm, reply_buf, reply_len);
                    
                    // Cleanup
                    free(reply_buf);
                    for (int i = 0; i < n_of_planets; i++) free(planet_ptrs[i]);
                    free(planet_ptrs);
                    for (int i = 0; i < trash_count; i++) free(trash_ptrs[i]);
                    free(trash_ptrs);
                    for (int i = 0; i < n_of_planets; i++) free(ship_ptrs[i]);
                    free(ship_ptrs);
                    free(init_setup);
                    
                    trashship__client_message__free_unpacked(msg, NULL);
                    continue;
                } 
                else 
                {
                    status = -2; 
                }
            } 
            else 
            {
                // If the Client sends a ship_id != 0, we go through the list of ID's until we find the matching one
                int idx = find_ship_index_by_id(ship, n_of_planets, ship_id);
                if (idx < 0) 
                {
                    status = -1; 
                } 
                else 
                {

                    pthread_mutex_lock(&ctx->lock);
                    vector thrust;
                    thrust.amplitude = 0;
                    thrust.angle = 0;

                    switch (command) 
                    {
                        case 5: // RIGHT
                            thrust.amplitude = SHIP_THRUST;
                            thrust.angle = 0;
                            break;
                        case 3: // LEFT
                            thrust.amplitude = SHIP_THRUST;
                            thrust.angle = 3.14159; // PI
                            break;
                        case 4: // DOWN
                            thrust.amplitude = SHIP_THRUST;
                            thrust.angle = 1.5708; // PI/2
                            break;
                        case 2: // UP
                            thrust.amplitude = SHIP_THRUST;
                            thrust.angle = -1.5708; // -PI/2
                            break;
                        case 1: // DISCONNECT
                            ship[idx].velocity.amplitude = 0;
                            ship[idx].ID = 0; // Remove ship
                            break;
                        default:
                            // status = -3;
                            break;
                    }

                
                if (command >= 2 && command <= 5) {
  
                    ship[idx].velocity = add_vectors(ship[idx].velocity, thrust);
    

                    if (ship[idx].velocity.amplitude > 5.0) {
                        ship[idx].velocity.amplitude = 5.0;
                    }
        }
                    pthread_mutex_unlock(&ctx->lock);
                }
            }
            
            reply_msg.has_status = 1;
            reply_msg.status = status;
            reply_msg.has_ship_id = 1;
            reply_msg.ship_id = reply_ship_id;
            reply_msg.ship_id = reply_ship_id;
            size_t reply_len = trashship__server_message__get_packed_size(&reply_msg);
            uint8_t reply_buf[256];
            trashship__server_message__pack(&reply_msg, reply_buf);
            comm_server_send(comm, reply_buf, reply_len);
            
            trashship__client_message__free_unpacked(msg, NULL);

            
        }            
    }
}

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
    if (free_idx < 0) return -1;

    int new_id = 0;
    int attempts = 0;

    do {
        new_id = (rand() % 1000000) + 1; 
        attempts++;
    } while (find_ship_index_by_id(ship, n, new_id) >= 0 && attempts < 16);

    if (find_ship_index_by_id(ship, n, new_id) >= 0) {
        return -1; 
    }

    ship[free_idx].ID = new_id;
    ship[free_idx].x = rand() % universe_dimensions;
    ship[free_idx].y = rand() % universe_dimensions;
    ship[free_idx].velocity.amplitude = 0;
    ship[free_idx].velocity.angle = 0;
    return free_idx;
}

void send_update(CommHandle *pub, struct planet_stucture *planets, int n_planets,struct trash_stucture *trash, int max_trash,struct trash_ship *ship) 
{
    Trashship__UpdateMessage msg = TRASHSHIP__UPDATE_MESSAGE__INIT;
    Trashship__PlanetUpdate **planet_ptrs = malloc(sizeof(Trashship__PlanetUpdate*) * n_planets);
    int planet_count = 0;

    for (int i = 0; i < n_planets; i++) 
    {
        planet_ptrs[planet_count] = malloc(sizeof(Trashship__PlanetUpdate));
        trashship__planet_update__init(planet_ptrs[planet_count]);
        planet_ptrs[planet_count]->has_planet_index = 1;
        planet_ptrs[planet_count]->planet_index = i;
        planet_ptrs[planet_count]->has_isrecycle = 1;
        planet_ptrs[planet_count]->isrecycle = planets[i].isrecycle;
        planet_ptrs[planet_count]->has_recycled_count = 1;
        planet_ptrs[planet_count]->recycled_count = planets[i].recycled_trash;
        planet_count++;
    }

    msg.planets = planet_ptrs;
    msg.n_planets = planet_count;
    
    int trash_count = update_trash_count(trash, max_trash);
    Trashship__Trash **trash_ptrs = malloc(sizeof(Trashship__Trash*) * trash_count);

    int trash_idx = 0;
    for (int i = 0; i < max_trash; i++) {
        if (trash[i].status != 0) {
            trash_ptrs[trash_idx] = malloc(sizeof(Trashship__Trash));
            trashship__trash__init(trash_ptrs[trash_idx]);
            trash_ptrs[trash_idx]->has_x = 1;
            trash_ptrs[trash_idx]->x = trash[i].x;
            trash_ptrs[trash_idx]->has_y = 1;
            trash_ptrs[trash_idx]->y = trash[i].y;
            trash_ptrs[trash_idx]->has_status = 1;
            trash_ptrs[trash_idx]->status = trash[i].status;
            trash_idx++;
        }
    }

    msg.trash = trash_ptrs;
    msg.n_trash = trash_count;
    
    Trashship__Ship **ship_ptrs = malloc(sizeof(Trashship__Ship*) * n_planets);

    for (int i = 0; i < n_planets; i++) {
        ship_ptrs[i] = malloc(sizeof(Trashship__Ship));
        trashship__ship__init(ship_ptrs[i]);
        ship_ptrs[i]->has_x = 1;
        ship_ptrs[i]->x = ship[i].x;
        ship_ptrs[i]->has_y = 1;
        ship_ptrs[i]->y = ship[i].y;
        ship_ptrs[i]->has_id = 1;
        ship_ptrs[i]->id = ship[i].ID;
        ship_ptrs[i]->has_cargo = 1;
        ship_ptrs[i]->cargo = ship[i].capacity;
    }
    msg.ships = ship_ptrs;
    msg.n_ships = n_planets;
    
    
    size_t len = trashship__update_message__get_packed_size(&msg);
    uint8_t *buffer = malloc(len);
    trashship__update_message__pack(&msg, buffer);
    comm_publisher_send(pub, buffer, len);
    
    
    free(buffer);
    for (int i = 0; i < n_planets; i++) free(planet_ptrs[i]);
    free(planet_ptrs);
    for (int i = 0; i < trash_count; i++) free(trash_ptrs[i]);
    free(trash_ptrs);
    for (int i = 0; i < n_planets; i++) free(ship_ptrs[i]);
    free(ship_ptrs);
}

void* broadcast_universe(void* arg) 
{
    CommHandle *pub = comm_publisher_init();
    UniverseContext *ctx = (UniverseContext*)arg;
    int n_planets = ctx->n_planets;
    struct trash_ship* ship = ctx->ship;
    struct planet_stucture* planets = ctx->planets;
    struct trash_stucture* trash = ctx->trash;
    int max_trash = ctx->max_trash;
    
    while(1) 
    {
        usleep(33000);  
        pthread_mutex_lock(&ctx->lock);
        send_update(pub, planets, n_planets, trash, max_trash, ship);
        pthread_mutex_unlock(&ctx->lock);
    }
}