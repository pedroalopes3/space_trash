#include "communication.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <zmq.h>
#include <libconfig.h>
#include <pthread.h>
#include "messages.pb-c.h"
#include "universe-data.h"

static const char *cached_endpoint = NULL;
static const char *cached_pubsub_endpoint = NULL;

//Search the config file for the communication endpoints or use defaults if none are found; cache them for future calls 
static void load_endpoints_from_config(void) {
    
    if (cached_endpoint != NULL) 
    {
        return;  
    }
    
    config_t cfg;
    config_init(&cfg);
    
    if (!config_read_file(&cfg, "universe-simulator.conf")) 
    {
        fprintf(stderr, "Warning: Could not read config file, using defaults\n");
        cached_endpoint = "tcp://127.0.0.1:5555";
        cached_pubsub_endpoint = "tcp://127.0.0.1:5556";
        config_destroy(&cfg);
        return;
    }
    
    const char *temp_endpoint = NULL;
    const char *temp_pubsub = NULL;
    
    if (!config_lookup_string(&cfg, "comm_endpoint", &temp_endpoint)) 
    {
        temp_endpoint = "tcp://127.0.0.1:5555";
    }

    if (!config_lookup_string(&cfg, "pubsub_endpoint", &temp_pubsub)) 
    {
        temp_pubsub = "tcp://127.0.0.1:5556";
    }
    
    // Make copies since config_destroy will free the strings
    cached_endpoint = strdup(temp_endpoint);
    cached_pubsub_endpoint = strdup(temp_pubsub);
    
    config_destroy(&cfg);
}

// Get endpoints from config file (caches after first call)
const char *comm_get_endpoint(void) 
{
    load_endpoints_from_config();
    return cached_endpoint;
}

const char *comm_get_pubsub_endpoint(void) 
{
    load_endpoints_from_config();
    return cached_pubsub_endpoint;
}



static CommHandle *comm_alloc(void) {
    CommHandle *h = (CommHandle *)malloc(sizeof(CommHandle));
    if (!h) {
        fprintf(stderr, "error alloccating CommHandle\n");
        return NULL;
    }
    h->context = NULL;
    h->socket = NULL;
    return h;
}

///////////////////////////////////////
//            REQ side 
///////////////////////////////////////
// Create context and a REQ socket that will connect to the server
CommHandle *comm_client_init(void) {
    CommHandle *h = comm_alloc();
    if (!h) return NULL;

    h->context = zmq_ctx_new();
    if (!h->context) {
        fprintf(stderr, "comm_client_init: zmq_ctx_new failed\n");
        free(h);
        return NULL;
    }

    h->socket = zmq_socket(h->context, ZMQ_REQ);
    if (!h->socket) {
        fprintf(stderr, "comm_client_init: zmq_socket failed\n");
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    int rc = zmq_connect(h->socket, comm_get_endpoint());
    if (rc != 0) {
        fprintf(stderr, "comm_client_init: zmq_connect failed\n");
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    // Set receive timeout to 5 seconds (5000 ms) in case the server isn't open,since the very first recv from the client will block indefinitely otherwise
    int timeout = 5000;
    rc = zmq_setsockopt(h->socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    if (rc != 0) {
        fprintf(stderr, "comm_client_init: failed to set timeout\n");
    }

    return h;
}

int comm_client_send(CommHandle *h, const void *data, size_t size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_send(h->socket, data, size, 0);  
    if (rc == -1) {
        perror("comm_client_send: zmq_send");
    }
    return rc;
}

int comm_client_recv(CommHandle *h, void *buffer, size_t max_size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_recv(h->socket, buffer, max_size, 0);
    if (rc == -1) {
        if (errno == EAGAIN) {
            // Timeout occurred
            fprintf(stderr, "comm_client_recv: timeout waiting for server response\n");
        } else {
            perror("comm_client_recv: zmq_recv");
        }
    }
    return rc;  // number of bytes received
}

/////////////////////////////////////////
//             REP side 
/////////////////////////////////////////
// Create context and a REP socket that will wait for client requests
CommHandle *comm_server_init(void) {
    CommHandle *h = comm_alloc();
    if (!h) return NULL;

    h->context = zmq_ctx_new();
    if (!h->context) {
        fprintf(stderr, "comm_server_init: zmq_ctx_new failed\n");
        free(h);
        return NULL;
    }

    h->socket = zmq_socket(h->context, ZMQ_REP); // REP socket
    if (!h->socket) {
        fprintf(stderr, "comm_server_init: zmq_socket failed\n");
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    int rc = zmq_bind(h->socket, comm_get_endpoint());
    if (rc != 0) {
        fprintf(stderr, "comm_server_init: zmq_bind failed\n");
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    return h;
}

int comm_server_recv(CommHandle *h, void *buffer, size_t max_size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_recv(h->socket, buffer, max_size, 0); 
    if (rc == -1) {
        perror("comm_server_recv: zmq_recv");
    }
    return rc;
}

int comm_server_send(CommHandle *h, const void *data, size_t size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_send(h->socket, data, size, 0);  
    if (rc == -1) {
        perror("comm_server_send: zmq_send");
    }
    return rc;
}

/////////////////////////////////////////
//          PUB side 
/////////////////////////////////////////
// Create context and a PUB socket that will broadcast messages to subscribers
CommHandle *comm_publisher_init(void) {
    CommHandle *h = comm_alloc();
    if (!h) return NULL;

    h->context = zmq_ctx_new();
    if (!h->context) {
        fprintf(stderr, "comm_publisher_init: zmq_ctx_new failed\n");
        free(h);
        return NULL;
    }

    h->socket = zmq_socket(h->context, ZMQ_PUB); // PUB socket
    if (!h->socket) {
        fprintf(stderr, "comm_publisher_init: zmq_socket failed\n");
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    int rc = zmq_bind(h->socket, comm_get_pubsub_endpoint());
    if (rc != 0) {
        fprintf(stderr, "comm_publisher_init: zmq_bind failed\n");
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    return h;
}

int comm_publisher_send(CommHandle *h, const void *data, size_t size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_send(h->socket, data, size, 0);
    if (rc == -1) {
        perror("comm_publisher_send: zmq_send");
    }
    return rc;
}

/////////////////////////////////////////
//          SUB side 
/////////////////////////////////////////
// Create context and a SUB socket, connect and subscribe to all messages
CommHandle *comm_subscriber_init(void) {
    CommHandle *h = comm_alloc();
    if (!h) return NULL;

    h->context = zmq_ctx_new();
    if (!h->context) {
        fprintf(stderr, "comm_subscriber_init: zmq_ctx_new failed\n");
        free(h);
        return NULL;
    }

    h->socket = zmq_socket(h->context, ZMQ_SUB); 
    if (!h->socket) {
        fprintf(stderr, "comm_subscriber_init: zmq_socket failed\n");
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    int rc = zmq_connect(h->socket, comm_get_pubsub_endpoint());
    if (rc != 0) {
        fprintf(stderr, "comm_subscriber_init: zmq_connect failed\n");
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    // Subscribe to all messages (empty filter)
    rc = zmq_setsockopt(h->socket, ZMQ_SUBSCRIBE, "", 0);
    if (rc != 0) {
        fprintf(stderr, "comm_subscriber_init: zmq_setsockopt failed\n");
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    // Set receive timeout to 5 seconds (5000 ms) in case the server is unresponsive/fails to send a shutdown message
    int timeout = 5000;
    rc = zmq_setsockopt(h->socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    if (rc != 0) {
        fprintf(stderr, "comm_subscriber_init: failed to set timeout\n");
    }

    return h;
}

int comm_subscriber_recv(CommHandle *h, void *buffer, size_t max_size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_recv(h->socket, buffer, max_size, 0);
    if (rc == -1) {
        perror("comm_subscriber_recv: zmq_recv");
    }
    return rc;
}

//////////////////////////////////
//           Common
//////////////////////////////////
//Close any socket,context and free handle
void comm_close(CommHandle *h) {
    if (!h) return;
    if (h->socket) {
        zmq_close(h->socket);
        h->socket = NULL;
    }
    if (h->context) {
        zmq_ctx_term(h->context);
        h->context = NULL;
    }
    free(h);
}


////////////////////////////////////////////////////////////////////
//Function ran by the universe-server thread responsible for handling client inputs and sending the initial universe setup message with a REP socket
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

            // If the Client sends a message with ship_id == 0, that means it's a new client and a random ID needs to be assigned
            if (ship_id == 0) 
            {
                pthread_mutex_lock(&ctx->mutex);
                int slot = allocate_ship_slot(ship, n_of_planets, universe_dimensions);
                if (slot >= 0) 
                {
                    reply_ship_id = ship[slot].ID;
                    
                    // Send an initial universe state to the new client, containing info on planets, trash and ships
                    Trashship__InitialSetupMessage *init_setup = malloc(sizeof(Trashship__InitialSetupMessage));
                    trashship__initial_setup_message__init(init_setup);
                    
                    
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
                    pthread_mutex_unlock(&ctx->mutex);
                    
                    trashship__client_message__free_unpacked(msg, NULL);
                    continue;
                } 
                else 
                {
                    status = -2; 
                    pthread_mutex_unlock(&ctx->mutex);
                }
            } 
            else 
            {
                // If the Client sends a ship_id != 0, we go through the list of ID's until we find the matching one
                pthread_mutex_lock(&ctx->mutex);
                int idx = find_ship_index_by_id(ship, n_of_planets, ship_id);
                if (idx < 0) 
                {
                    status = -1; 
                } 
                else 
                {
                    //take note of the command input sent by the client for that ship; in the case the client closed the window (input == 1) get rid of the ship
                    //by setting it's ID back to 0.
                    if(ship[idx].input == 1)
                    {
                        ship[idx].ID = 0;
                    }
                    else
                    {
                        ship[idx].input = command;
                    }
                }
                pthread_mutex_unlock(&ctx->mutex);
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

//Function used by the server's "broadcast_universe" function; Sends the usual universe-state updates to trashships
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
    }
    msg.ships = ship_ptrs;
    msg.n_ships = n_planets;
    msg.has_status = 1;
    msg.status = 0;  
    
    
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

//Function used by the server's "broadcast_universe" function; Sends the necessary universe info to the dashboard
void send_score_update(CommHandle *pub, struct planet_stucture *planets, int n_planets, struct trash_stucture *trash, int max_trash, struct trash_ship *ship) 
{
    Trashship__ScoreUpdate msg = TRASHSHIP__SCORE_UPDATE__INIT;
    
    // Trash count
    int trash_count = update_trash_count(trash, max_trash);
    msg.has_trash_count = 1;
    msg.trash_count = trash_count;
    
    // Ship capacities
    Trashship__ShipCapacity **ship_caps = malloc(sizeof(Trashship__ShipCapacity*) * n_planets);
    int ship_count = 0;
    for (int i = 0; i < n_planets; i++) {
        if (ship[i].ID != 0) {
            ship_caps[ship_count] = malloc(sizeof(Trashship__ShipCapacity));
            trashship__ship_capacity__init(ship_caps[ship_count]);
            ship_caps[ship_count]->has_capacity = 1;
            ship_caps[ship_count]->capacity = ship[i].capacity;
            ship_caps[ship_count]->name = strdup(ship[i].name);
            ship_count++;
        }
    }
    msg.ships = ship_caps;
    msg.n_ships = ship_count;
    
    // Planet scores
    Trashship__PlanetScore **planet_scores = malloc(sizeof(Trashship__PlanetScore*) * n_planets);
    for (int i = 0; i < n_planets; i++) {
        planet_scores[i] = malloc(sizeof(Trashship__PlanetScore));
        trashship__planet_score__init(planet_scores[i]);
        planet_scores[i]->name = strdup(planets[i].name);
        planet_scores[i]->has_score = 1;
        planet_scores[i]->score = planets[i].score;
    }
    msg.planet_scores = planet_scores;
    msg.n_planet_scores = n_planets;
    
    // Pack and send with topic prefix
    size_t len = trashship__score_update__get_packed_size(&msg);
    size_t total_len = strlen("SCORE:") + len;
    uint8_t *buffer = malloc(total_len);
    
    // Write topic prefix
    memcpy(buffer, "SCORE:", strlen("SCORE:"));
    
    // Write message payload
    trashship__score_update__pack(&msg, buffer + strlen("SCORE:"));
    comm_publisher_send(pub, buffer, total_len);
    
    // Cleanup
    free(buffer);
    for (int i = 0; i < ship_count; i++) {
        free(ship_caps[i]->name);
        free(ship_caps[i]);
    }
    free(ship_caps);
    for (int i = 0; i < n_planets; i++) {
        free(planet_scores[i]->name);
        free(planet_scores[i]);
    }
    free(planet_scores);
}

//Function used by the server's "broadcast_universe" function; Alerts the clients of a shutdown, be it by quitting or implosion from too much trash
static void send_shutdown_message(CommHandle *pub, struct planet_stucture *planets, int n_planets,struct trash_stucture *trash, int max_trash, struct trash_ship *ship, int status_code)
{
    Trashship__UpdateMessage msg = TRASHSHIP__UPDATE_MESSAGE__INIT;
    msg.has_status = 1;
    msg.status = status_code;  // -3 for SDL_QUIT, -4 for implosion
    msg.n_planets = 0;
    msg.n_trash = 0;
    msg.n_ships = 0;
    
    size_t len = trashship__update_message__get_packed_size(&msg);
    uint8_t *buffer = malloc(len);
    trashship__update_message__pack(&msg, buffer);
    comm_publisher_send(pub, buffer, len);
    free(buffer);
}

//Function ran by the universe-server thread responsible for broadcasting universe state updates to all connected trashship clients, broadcasting universe info
//to the dashboard and warning clients about server shutdowns, all via a PUB socket
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
        pthread_mutex_lock(&ctx->mutex);
        
        // Check for stop_flag (from implosion or SDL_QUIT)
        if (ctx->stop_flag) {
            // Send shutdown notification to clients (status -3)
            send_shutdown_message(pub, planets, n_planets, trash, max_trash, ship, -3);
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }
        
        // Check for universe implosion from too much trash
        int trash_count = update_trash_count(trash, max_trash);
        if (trash_count >= max_trash) {
            ctx->stop_flag = 1;  // Signal main loop to stop
            fprintf(stderr, "Universe imploded from too much trash\n");
            send_shutdown_message(pub, planets, n_planets, trash, max_trash, ship, -4);
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }
        
        send_update(pub, planets, n_planets, trash, max_trash, ship);
        send_score_update(pub, planets, n_planets, trash, max_trash, ship);
        pthread_mutex_unlock(&ctx->mutex);
    }
    return NULL;
}

//Function ran by the trashship-client thread responsible for receiving universe state updates from the server via a SUB socket
void* receive_universe_updates(void* arg) 
{
    ClientContext *ctx = (ClientContext*)arg;
    int n_planets = ctx->n_planets;
    int universe_dimensions = ctx->universe_dimensions;
    struct trash_ship* ship = ctx->ship;
    struct planet_stucture* planets = ctx->planets;
    struct trash_stucture* trash = ctx->trash;
    int max_trash = ctx->max_trash;
    
    
    CommHandle *sub = comm_subscriber_init();
    if (!sub) {
        fprintf(stderr, "Failed to init subscriber\n");
        return NULL;
    }
    
    uint8_t buffer[65536];
    
    while (1) 
    {
        int n = comm_subscriber_recv(sub, buffer, sizeof(buffer));
        if (n <= 0) {
            if (errno == EAGAIN) {
                fprintf(stderr, "Server not responding (timeout)\n");
            }
            continue;
        }
        
        Trashship__UpdateMessage *update_msg = trashship__update_message__unpack(NULL, n, buffer);

        if (update_msg) 
        {
            // Check for server shutdown notifications
            if (update_msg->has_status && update_msg->status < 0) 
            {
                if (update_msg->status == -3) 
                {
                    fprintf(stderr, "Server is shutting down\n");
                } 
                else if (update_msg->status == -4) 
                {
                    fprintf(stderr, "Universe imploded from too much trash\n");
                }
                ctx->stop_flag = 1;  
                trashship__update_message__free_unpacked(update_msg, NULL);
                break;  
            }
            
            pthread_mutex_lock(&ctx->mutex);
            for (size_t i = 0; i < update_msg->n_planets; i++) 
            {
                int idx = update_msg->planets[i]->planet_index;
                if (idx >= 0 && idx < ctx->n_planets) 
                {
                    ctx->planets[idx].isrecycle = update_msg->planets[i]->isrecycle;
                }
            }
            
            for (int i = 0; i < ctx->max_trash; i++) 
            {
                ctx->trash[i].status = 0; 
            }

            for (size_t i = 0; i < update_msg->n_trash && i < ctx->max_trash; i++) 
            {
                ctx->trash[i].x = update_msg->trash[i]->x;
                ctx->trash[i].y = update_msg->trash[i]->y;
                ctx->trash[i].status = update_msg->trash[i]->status;
            }
            
            for (size_t i = 0; i < update_msg->n_ships && i < ctx->n_ships; i++) 
            {
                ctx->ship[i].x = update_msg->ships[i]->x;
                ctx->ship[i].y = update_msg->ships[i]->y;
                ctx->ship[i].ID = update_msg->ships[i]->id;
            }
            pthread_mutex_unlock(&ctx->mutex);
                
            trashship__update_message__free_unpacked(update_msg, NULL);
        }
    }
}