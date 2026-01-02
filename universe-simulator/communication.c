#include "communication.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zmq.h>



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
//            Client side 
///////////////////////////////////////

CommHandle *comm_client_init(void) {
    CommHandle *h = comm_alloc();
    if (!h) return NULL;

    h->context = zmq_ctx_new();
    if (!h->context) {
        fprintf(stderr, "comm_client_init: zmq_ctx_new failed\n");
        free(h);
        return NULL;
    }

    h->socket = zmq_socket(h->context, ZMQ_REQ); // REQ socket
    if (!h->socket) {
        fprintf(stderr, "comm_client_init: zmq_socket failed\n");
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    int rc = zmq_connect(h->socket, COMM_ENDPOINT);
    if (rc != 0) {
        fprintf(stderr, "comm_client_init: zmq_connect failed\n");
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    return h;
}

int comm_client_send(CommHandle *h, const void *data, size_t size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_send(h->socket, data, size, 0);  // blocking send
    if (rc == -1) {
        perror("comm_client_send: zmq_send");
    }
    return rc;
}

int comm_client_recv(CommHandle *h, void *buffer, size_t max_size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_recv(h->socket, buffer, max_size, 0);
    if (rc == -1) {
        perror("comm_client_recv: zmq_recv");
    }
    return rc;  // number of bytes received
}

/////////////////////////////////////////
//             Server side 
/////////////////////////////////////////

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

    int rc = zmq_bind(h->socket, COMM_ENDPOINT);
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
    int rc = zmq_send(h->socket, data, size, 0);  // blocking
    if (rc == -1) {
        perror("comm_server_send: zmq_send");
    }
    return rc;
}

/////////////////////////////////////////
//          Publisher side 
/////////////////////////////////////////

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

    int rc = zmq_bind(h->socket, COMM_PUBSUB_ENDPOINT);
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
//          Subscriber side 
/////////////////////////////////////////

CommHandle *comm_subscriber_init(void) {
    CommHandle *h = comm_alloc();
    if (!h) return NULL;

    h->context = zmq_ctx_new();
    if (!h->context) {
        fprintf(stderr, "comm_subscriber_init: zmq_ctx_new failed\n");
        free(h);
        return NULL;
    }

    h->socket = zmq_socket(h->context, ZMQ_SUB); // SUB socket
    if (!h->socket) {
        fprintf(stderr, "comm_subscriber_init: zmq_socket failed\n");
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    int rc = zmq_connect(h->socket, COMM_PUBSUB_ENDPOINT);
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