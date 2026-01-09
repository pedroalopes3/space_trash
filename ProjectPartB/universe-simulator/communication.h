#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stddef.h>   // size_t
#include "universe-data.h"

typedef struct {
    void *context;   
    void *socket;    
} CommHandle;

// Get endpoints from config file (caches after first call)
const char *comm_get_endpoint(void);
const char *comm_get_pubsub_endpoint(void);

///////////////////////////////////////
// ---------- REQ side ----------
///////////////////////////////////////

CommHandle *comm_client_init(void);

int comm_client_send(CommHandle *h, const void *data, size_t size);

int comm_client_recv(CommHandle *h, void *buffer, size_t max_size);

/////////////////////////////////////////
// ---------- REP side ----------
/////////////////////////////////////////

CommHandle *comm_server_init(void);

int comm_server_recv(CommHandle *h, void *buffer, size_t max_size);

int comm_server_send(CommHandle *h, const void *data, size_t size);

/////////////////////////////////////////
// ---------- PUB side ----------
/////////////////////////////////////////

CommHandle *comm_publisher_init(void);

int comm_publisher_send(CommHandle *h, const void *data, size_t size);

/////////////////////////////////////////
// ---------- SUB side ----------
/////////////////////////////////////////

CommHandle *comm_subscriber_init(void);

int comm_subscriber_recv(CommHandle *h, void *buffer, size_t max_size);

///////////////////////////////////////
// ---------- Common ----------
///////////////////////////////////////

void comm_close(CommHandle *h);

///////////////////////////////////////////
void* receive_universe_updates(void* arg);
void * input_communication(void* shipstructarg);
void send_update(CommHandle *pub, struct planet_stucture *planets, int n_planets,struct trash_stucture *trash, int max_trash,struct trash_ship *ship);
void send_score_update(CommHandle *pub, struct planet_stucture *planets, int n_planets, struct trash_stucture *trash, int max_trash, struct trash_ship *ship);
void * broadcast_universe(void *arg);   
#endif
