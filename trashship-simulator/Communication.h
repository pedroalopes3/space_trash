#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stddef.h>   
#include "protoc/Communication.pb-c.h" 

// Common server endpoint 
#define COMM_ENDPOINT "tcp://127.0.0.1:5555"

typedef struct {
    void *context;   // ZMQ context
    void *socket;    // ZMQ socket
} CommHandle;

///////////////////////////////////////
// ---------- Client side ----------
///////////////////////////////////////

// Create context + REQ socket and connect to server
CommHandle *comm_client_init(void);

// Send a request (blocking)
int comm_client_send(CommHandle *h, const void *data, size_t size);

// Receive reply (blocking)
int comm_client_recv(CommHandle *h, void *buffer, size_t max_size);

void send_join_command(CommHandle *comm);

void receive_client_id(CommHandle *comm, int *client_id);

void send_move_command(CommHandle *comm, int client_id,int key_pressed);

void send_hello(CommHandle *comm);
void receive_hello(CommHandle *comm);

/////////////////////////////////////////
// ---------- Server side ----------
/////////////////////////////////////////

// Create context + REP socket and bind on ENDPOINT
CommHandle *comm_server_init(void);

// Receive a request (non-blocking)
int comm_server_recv(CommHandle *h, void *buffer, size_t max_size);

// Send a reply (blocking)
int comm_server_send(CommHandle *h, const void *data, size_t size);

void join_received_message(CommHandle *comm, uint8_t *buffer, int len, int* client_id) ;

Trashsimulator__MoveCommand *process_received_message(CommHandle *comm);

///////////////////////////////////////
// ---------- Common ----------
///////////////////////////////////////

// Close socket + context and free handle
void comm_close(CommHandle *h);

#endif
