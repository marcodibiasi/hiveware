#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdatomic.h>
#include <uuid/uuid.h>
#include "tcp.h"
#include "node.h"
#include "job.h"
#include "job_registry.h"
#include "utils.h"


static int send_all(int fd, const void* buf, size_t len){
    const uint8_t* p = buf;
    size_t sent = 0;
    while(sent < len){
        ssize_t n = send(fd, p + sent, len - sent, 0);
        if(n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}


static int recv_all(int fd, void* buf, size_t len){
    uint8_t* p = buf;
    size_t got = 0;
    while(got < len){
        ssize_t n = recv(fd, p + got, len - got, 0);
        if(n <= 0) return -1; // 0 = peer ha chiuso, <0 = errore
        got += (size_t)n;
    }
    return 0;
}


int tcp_send_frame(int sockfd, MessageType type, const void* body, uint32_t body_len){
    FrameHeader hdr;
    hdr.magic = HIWA_MAGIC;
    hdr.version = HIWA_PROTO_VERSION;
    hdr.msg_type = (uint8_t)type;
    hdr.payload_len = body_len;

    if(send_all(sockfd, &hdr, sizeof(hdr)) != 0) return -1;
    if(body_len > 0 && send_all(sockfd, body, body_len) != 0) return -1;

    return 0;
}


int tcp_recv_frame(int sockfd, MessageType* type, void** body, uint32_t* body_len){
    FrameHeader hdr;
    if(recv_all(sockfd, &hdr, sizeof(hdr)) != 0) return -1;

    if(hdr.magic != HIWA_MAGIC || hdr.version != HIWA_PROTO_VERSION)
        return -1; // frame non valido o versione protocollo incompatibile

    if(hdr.payload_len > MAX_FRAME_PAYLOAD)
        return -1;

    void* buf = NULL;
    if(hdr.payload_len > 0){
        buf = malloc(hdr.payload_len);
        if(!buf) return -1;

        if(recv_all(sockfd, buf, hdr.payload_len) != 0){
            free(buf);
            return -1;
        }
    }

    *type = (MessageType)hdr.msg_type;
    *body = buf;
    *body_len = hdr.payload_len;
    return 0;
}


int tcp_send_task_assign(int sockfd, const uuid_t job_id, uint32_t task_id,
                          const void* payload, size_t payload_len){
    size_t body_len = sizeof(TaskFrameHeader) + payload_len;
    uint8_t* body = malloc(body_len);
    if(!body) return -1;

    TaskFrameHeader tfh;
    memcpy(tfh.job_id, job_id, sizeof(uuid_t));
    tfh.task_id = task_id;

    memcpy(body, &tfh, sizeof(tfh));
    if(payload_len > 0)
        memcpy(body + sizeof(tfh), payload, payload_len);

    int rc = tcp_send_frame(sockfd, MSG_TASK_ASSIGN, body, (uint32_t)body_len);
    free(body);
    return rc;
}


int tcp_send_task_result(int sockfd, const uuid_t job_id, uint32_t task_id,
                          const void* result, size_t result_len){
    size_t body_len = sizeof(TaskFrameHeader) + result_len;
    uint8_t* body = malloc(body_len);
    if(!body) return -1;

    TaskFrameHeader tfh;
    memcpy(tfh.job_id, job_id, sizeof(uuid_t));
    tfh.task_id = task_id;

    memcpy(body, &tfh, sizeof(tfh));
    if(result_len > 0)
        memcpy(body + sizeof(tfh), result, result_len);

    int rc = tcp_send_frame(sockfd, MSG_TASK_RESULT, body, (uint32_t)body_len);
    free(body);
    return rc;
}


int tcp_send_task_error(int sockfd, const uuid_t job_id, uint32_t task_id){
    TaskFrameHeader tfh;
    memcpy(tfh.job_id, job_id, sizeof(uuid_t));
    tfh.task_id = task_id;

    return tcp_send_frame(sockfd, MSG_TASK_ERROR, &tfh, sizeof(tfh));
}


void init_c_socket(Node* node){
    if((node->c_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        exit_error("c_socket");

    int opt = 1;
    if(setsockopt(node->c_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        exit_error("setsockopt SO_REUSEADDR (c_socket)");

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(node->config.c_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(node->c_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        exit_error("c_socket bind");

    if(listen(node->c_socket, TCP_BACKLOG) < 0)
        exit_error("c_socket listen");

    printf("c_socket listening on port %d\n", node->config.c_port);
}


typedef struct {
    Node* node;
    int conn_fd;
} ConnCtx;


void* tcp_conn_handler(void* arg){
    ConnCtx* ctx = (ConnCtx*)arg;
    Node* node = ctx->node;
    int fd = ctx->conn_fd;
    free(ctx);

    while(atomic_load(&node->running)){
        MessageType type;
        void* body = NULL;
        uint32_t body_len = 0;

        if(tcp_recv_frame(fd, &type, &body, &body_len) != 0)
            break; // peer disconnesso o frame non valido: chiudiamo la connessione

        if(body_len < sizeof(TaskFrameHeader)){
            free(body);
            continue; // frame troppo corto per contenere job_id/task_id, ignoralo
        }

        TaskFrameHeader tfh;
        memcpy(&tfh, body, sizeof(tfh));

        switch(type){

        case MSG_TASK_ASSIGN: {
            const void* task_payload = (uint8_t*)body + sizeof(tfh);
            size_t task_payload_len = body_len - sizeof(tfh);

            void* result = NULL;
            size_t result_len = 0;
            int ok = node->task_executor &&
                     node->task_executor(task_payload, task_payload_len, &result, &result_len) == 0;

            if(ok)
                tcp_send_task_result(fd, tfh.job_id, tfh.task_id, result, result_len);
            else
                tcp_send_task_error(fd, tfh.job_id, tfh.task_id);

            free(result);
            break;
        }

        case MSG_TASK_RESULT: {
            Job* job = job_registry_find(node->job_registry, tfh.job_id);
            if(job){
                size_t result_len = body_len - sizeof(tfh);
                void* result = malloc(result_len);
                if(result){
                    memcpy(result, (uint8_t*)body + sizeof(tfh), result_len);
                    job_task_completed(job, tfh.task_id, result, result_len);
                }
            }
            break;
        }

        case MSG_TASK_ERROR: {
            Job* job = job_registry_find(node->job_registry, tfh.job_id);
            if(job)
                job_reassign_task(job, tfh.task_id);
            break;
        }

        default:
            break; // tipo sconosciuto, ignoralo (compatibilita' futura)
        }

        free(body);
    }

    close(fd);
    return NULL;
}


void* tcp_accept_daemon(void* arg){
    Node* node = (Node*)arg;

    while(atomic_load(&node->running)){
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);

        int conn_fd = accept(node->c_socket, (struct sockaddr*)&peer_addr, &addr_len);
        if(conn_fd < 0){
            if(atomic_load(&node->running))
                perror("accept");
            continue; // se running e' false il socket e' stato chiuso di proposito, si esce al prossimo controllo
        }

        ConnCtx* ctx = malloc(sizeof(ConnCtx));
        if(!ctx){ close(conn_fd); continue; }
        ctx->node = node;
        ctx->conn_fd = conn_fd;

        pthread_t handler;
        if(pthread_create(&handler, NULL, tcp_conn_handler, ctx) != 0){
            free(ctx);
            close(conn_fd);
            continue;
        }
        // Connessioni gestite in modo indipendente: il thread si stacca
        // (detach) e chiude da solo il socket alla disconnessione. NOTA:
        // allo shutdown del nodo queste connessioni non vengono joinate
        // esplicitamente, restano legate al solo controllo su node->running.
        pthread_detach(handler);
    }

    return NULL;
}


int tcp_connect_to_peer(Node* node, DiscNode* peer){
    (void)node; // non serve piu': la porta viene dal peer stesso, non dalla propria config

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr = peer->addr.sin_addr;        // stesso IP da cui e' arrivato l'HELLO
    addr.sin_port = htons(peer->c_port);         // porta TCP reale annunciata dal peer nell'HELLO

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) return -1;

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        close(fd);
        return -1;
    }

    return fd;
}
