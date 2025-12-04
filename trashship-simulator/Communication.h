#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stddef.h>   // size_t

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

/////////////////////////////////////////
// ---------- Server side ----------
/////////////////////////////////////////

// Create context + REP socket and bind on ENDPOINT
CommHandle *comm_server_init(void);

// Receive a request (non-blocking)
int comm_server_recv(CommHandle *h, void *buffer, size_t max_size, int timeout_ms);

// Send a reply (blocking)
int comm_server_send(CommHandle *h, const void *data, size_t size);

///////////////////////////////////////
// ---------- Common ----------
///////////////////////////////////////

// Close socket + context and free handle
void comm_close(CommHandle *h);

#endif
