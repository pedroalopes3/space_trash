#include "Communication.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>

#include "protoc/Communication.pb-c.h" 



// allocate and initialize CommHandle struct

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

    fprintf(stderr, "Client connected to %s\n", COMM_ENDPOINT);

    return h;
}

int comm_client_send(CommHandle *h, const void *data, size_t size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_send(h->socket, data, size, 0);  
    if (rc == -1) {
        int zerr = zmq_errno();
        fprintf(stderr, "comm_client_send: zmq_send failed: %s (%d)\n", zmq_strerror(zerr), zerr);
    }
    return rc;
}

int comm_client_recv(CommHandle *h, void *buffer, size_t max_size) {
    if (!h || !h->socket) return -1;

    int rc = zmq_recv(h->socket, buffer, max_size, 0);
    if (rc == -1) {
        int zerr = zmq_errno();
        
        fprintf(stderr, "comm_client_recv: zmq_recv failed: %s (%d)\n", zmq_strerror(zerr), zerr);
    }
    return rc;  // number of bytes received
}

void send_join_command(CommHandle *comm) {

     printf("sending join.\n");
    
    Trashsimulator__Join msg = TRASHSIMULATOR__JOIN__INIT;  

    size_t len = trashsimulator__join__get_packed_size(&msg); 
    uint8_t *buf = malloc(len);  
    if (!buf) {
        
        return;
    }

    trashsimulator__join__pack(&msg, buf);
    comm_client_send(comm, buf, len);
    free(buf); 

     printf("sent.\n");
}

void receive_client_id(CommHandle *comm, int *client_id){


    uint8_t buffer[1024];  

    int len = comm_client_recv(comm, buffer, sizeof(buffer));
    if (len > 0) {
        Trashsimulator__JoinResponse *msg = trashsimulator__join_response__unpack(NULL, len, buffer);
        if (msg == NULL) {
            return;
        }

        *client_id = msg->client_id;

        trashsimulator__join_response__free_unpacked(msg, NULL);
    }
}

void send_move_command(CommHandle *comm, int client_id, int key_pressed) {
    
    Trashsimulator__MoveCommand msg = TRASHSIMULATOR__MOVE_COMMAND__INIT;  
    msg.client_id = client_id;
    msg.command = key_pressed;;

    size_t len = trashsimulator__move_command__get_packed_size(&msg); 
    uint8_t *buf = malloc(len);  
    if (!buf) {
        
        return;
    }

    trashsimulator__move_command__pack(&msg, buf);
    comm_client_send(comm, buf, len);
    free(buf); 
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
        int zerr = zmq_errno();
        fprintf(stderr, "comm_server_init: zmq_bind failed: %s (%d)\n", zmq_strerror(zerr), zerr);
        zmq_close(h->socket);
        zmq_ctx_term(h->context);
        free(h);
        return NULL;
    }

    fprintf(stderr, "Server bound to %s\n", COMM_ENDPOINT);

    return h;
}

int comm_server_recv(CommHandle *h, void *buffer, size_t max_size) {
    if (!h || !h->socket) return -1;

    int rc = zmq_recv(h->socket, buffer, max_size, 0);  // BLOCKING
    if (rc == -1) {
        int zerr = zmq_errno();
        fprintf(stderr, "comm_server_recv: zmq_recv failed: %s (%d)\n", zmq_strerror(zerr), zerr);
        return -1;
    }
    return rc;
}

int comm_server_send(CommHandle *h, const void *data, size_t size) {
    if (!h || !h->socket) return -1;
    int rc = zmq_send(h->socket, data, size, 0);  // blocking
    if (rc == -1) {
        int zerr = zmq_errno();
        fprintf(stderr, "comm_server_send: zmq_send failed: %s (%d)\n", zmq_strerror(zerr), zerr);
    }
    return rc;
}

void join_received_message(CommHandle *comm, uint8_t *buffer, int len, int* client_id) {
    if (len <= 0) return;
    printf("Server: received %d bytes\n", len);

    
    Trashsimulator__Join *join_msg = trashsimulator__join__unpack(NULL, len, buffer);
    if (join_msg) {
        
        printf("Server: received JOIN\n");
        
        Trashsimulator__JoinResponse reply = TRASHSIMULATOR__JOIN_RESPONSE__INIT;
        reply.client_id = *client_id;
        (*client_id)++;
        
        size_t rlen = trashsimulator__join_response__get_packed_size(&reply);
        uint8_t *buf = malloc(rlen);
        if (buf) {
            trashsimulator__join_response__pack(&reply, buf);
            comm_server_send(comm, buf, rlen);
            free(buf);
        }
        
        trashsimulator__join__free_unpacked(join_msg, NULL);
        printf("Server: sent JoinResponse with ID %d\n", (*client_id) - 1);
        return;
    }

    Trashsimulator__MoveCommand *move_msg = trashsimulator__move_command__unpack(NULL, len, buffer);
    if (move_msg) {
        printf("Client ID: %d | Command: %d\n", move_msg->client_id, move_msg->command);
       
        const char *ack = "ACK";
        comm_server_send(comm, ack, strlen(ack));
        trashsimulator__move_command__free_unpacked(move_msg, NULL);
        return;
    }

    {
        const char *unk = "UNKNOWN";
        comm_server_send(comm, unk, strlen(unk));
    }
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

void send_hello(CommHandle *comm) {
    printf("sending hello.\n");
    
    const char *msg = "HELLO";
    comm_client_send(comm, msg, strlen(msg) + 1);
    
    printf("hello sent.\n");
}

void receive_hello(CommHandle *comm) {
    uint8_t buffer[1024];
    
    int len = comm_client_recv(comm, buffer, sizeof(buffer));
    if (len > 0) {
        buffer[len] = '\0';
        printf("Client received: %s\n", (char *)buffer);
    }
}