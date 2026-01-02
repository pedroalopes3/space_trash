#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stddef.h>   // size_t

// Common server endpoints
#define COMM_ENDPOINT "tcp://127.0.0.1:5555"
#define COMM_PUBSUB_ENDPOINT "tcp://127.0.0.1:5556"

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

// Receive a request (blocking)
int comm_server_recv(CommHandle *h, void *buffer, size_t max_size);

// Send a reply (blocking)
int comm_server_send(CommHandle *h, const void *data, size_t size);

/////////////////////////////////////////
// ---------- Publisher side ----------
/////////////////////////////////////////

// Create context + PUB socket and bind on ENDPOINT
CommHandle *comm_publisher_init(void);

// Publish a message (non-blocking for PUB)
int comm_publisher_send(CommHandle *h, const void *data, size_t size);

/////////////////////////////////////////
// ---------- Subscriber side ----------
/////////////////////////////////////////

// Create context + SUB socket, connect and subscribe to all messages
CommHandle *comm_subscriber_init(void);

// Receive a published message (blocking)
int comm_subscriber_recv(CommHandle *h, void *buffer, size_t max_size);

///////////////////////////////////////
// ---------- Common ----------
///////////////////////////////////////

// Close socket + context and free handle
void comm_close(CommHandle *h);

#endif
