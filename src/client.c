/* client.c: Message Queue Client */
#include <fcntl.h>
#include <unistd.h>

#include "mq/client.h"
#include "mq/logging.h"
#include "mq/socket.h"
#include "mq/string.h"

/* Internal Constants */

#define SENTINEL "SHUTDOWN"

/* Internal Prototypes */

void *mq_pusher(void *);
void *mq_puller(void *);

void check_response(MessageQueue *, char **body);
Request *get_response(MessageQueue *mq);

/* External Functions */

/**
 * Create Message Queue withs specified name, host, and port.
 * @param   name        Name of client's queue.
 * @param   host        Address of server.
 * @param   port        Port of server.
 * @return  Newly allocated Message Queue structure.
 */
MessageQueue *mq_create(const char *name, const char *host, const char *port) {
    MessageQueue *mq = (MessageQueue *)malloc(sizeof(MessageQueue));

    if (mq) {
        strcpy(mq->name, name);
        strcpy(mq->host, host);
        strcpy(mq->port, port);

        mq->outgoing = queue_create();
        mq->incoming = queue_create();

        mq->shutdown = false;
        mq->fs = socket_connect(mq->host, mq->port);

        int fd = fileno(mq->fs);
        int flags = fcntl(fd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);

        mutex_init(&mq->sd_lock, NULL);
        mutex_init(&mq->fs_lock, NULL);
        cond_init(&mq->retv_cond, NULL);
    }

    return mq;
}

/**
 * Delete Message Queue structure (and internal resources).
 * @param   mq      Message Queue structure.
 */
void mq_delete(MessageQueue *mq) {
    if (mq) {
        queue_delete(mq->outgoing);
        queue_delete(mq->incoming);
        mutex_destroy(&mq->sd_lock);
        mutex_destroy(&mq->fs_lock);
        cond_destroy(&mq->retv_cond);
    }

    free(mq);
}

/**
 * Publish one message to topic (by placing new Request in outgoing queue).
 * @param   mq      Message Queue structure.
 * @param   topic   Topic to publish to.
 * @param   body    Message body to publish.
 */
void mq_publish(MessageQueue *mq, const char *topic, const char *body) {
    char uri[64];
    sprintf(uri, "/topic/%s", topic);
    queue_push(mq->outgoing, request_create("PUT", uri, body));
    // check_response(mq, NULL);
}

/**
 * Retrieve one message (by taking Request from incoming queue).
 * @param   mq      Message Queue structure.
 * @return  Newly allocated message body (must be freed).
 */
char *mq_retrieve(MessageQueue *mq) {
    char uri[64];
    sprintf(uri, "/queue/%s", mq->name);
    queue_push(mq->outgoing, request_create("GET", uri, NULL));

    char *msg;
    mutex_lock(&mq->fs_lock);

    check_response(mq, &msg);
    return msg;
}

/**
 * Subscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to subscribe to.
 **/
void mq_subscribe(MessageQueue *mq, const char *topic) {
    char uri[64];
    sprintf(uri, "/subscription/%s/%s", mq->name, topic);
    queue_push(mq->outgoing, request_create("PUT", uri, NULL));
}

/**
 * Unubscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to unsubscribe from.
 **/
void mq_unsubscribe(MessageQueue *mq, const char *topic) {
    char uri[64];
    sprintf(uri, "/subscription/%s/%s", mq->name, topic);
    queue_push(mq->outgoing, request_create("DELETE", uri, NULL));

    // mutex_lock(&mq->fs_lock);
    // Request * r = get_response(mq);
    // printf("%s\n", __func__);
    // mutex_unlock(&mq->fs_lock);
}

/**
 * Start running the background threads:
 *  1. First thread should continuously send requests from outgoing queue.
 *  2. Second thread should continuously receive reqeusts to incoming queue.
 * @param   mq      Message Queue structure.
 */
void mq_start(MessageQueue *mq) {
    thread_create(&mq->pusher, NULL, mq_pusher, mq);
    thread_create(&mq->puller, NULL, mq_puller, mq);
}

/**
 * Stop the message queue client by setting shutdown attribute and sending
 * sentinel messages
 * @param   mq      Message Queue structure.
 */
void mq_stop(MessageQueue *mq) {
    mutex_lock(&mq->sd_lock);
    mq->shutdown = true;
    mutex_unlock(&mq->sd_lock);

    // send sentinel messages
    queue_push(mq->outgoing, request_create(SENTINEL, NULL, NULL));

    thread_join(mq->pusher, NULL);
    thread_join(mq->puller, NULL);
    printf("stop\n");
}

/**
 * Returns whether or not the message queue should be shutdown.
 * @param   mq      Message Queue structure.
 */
bool mq_shutdown(MessageQueue *mq) {
    mutex_lock(&mq->sd_lock);
    bool res = mq->shutdown;
    mutex_unlock(&mq->sd_lock);

    return res;
}

/* Internal Functions */

/**
 * Pusher thread takes messages from outgoing queue and sends them to server.
 **/
void *mq_pusher(void *arg) {
    MessageQueue *mq = (MessageQueue *)arg;
    Request *r;

    while (!mq_shutdown(mq)) {
        // takes message
        printf("start\n");
        r = queue_pop(mq->outgoing);

        if (streq(r->method, SENTINEL))
            break;

        // send to server
        mutex_lock(&mq->fs_lock);
        printf("send\n");
        request_write(r, mq->fs);
        printf("%s %s\n", r->method, r->uri);
        if (!streq(r->method, "GET")) {
            printf("get response\n");
            Request * resp = get_response(mq);
            printf("body: %s\n", resp->body);
            request_delete(resp);
        } else {
            cond_signal(&mq->retv_cond);
        }
        mutex_unlock(&mq->fs_lock);
        request_delete(r);
        printf("end\n");
    }

    return NULL;
}

/**
 * Puller thread requests new messages from server and then puts them in
 * incoming queue.
 **/
void *mq_puller(void *arg) {
    MessageQueue *mq = (MessageQueue *)arg;
    Request *r;

    char buffer[512];
    char http[64];
    char code[64];
    char code_msg[64];

    while (!mq_shutdown(mq)) {
        mutex_lock(&mq->fs_lock);

        cond_wait(&mq->retv_cond, &mq->fs_lock);
        
        r = get_response(mq);
        // put into queue
        if (r) {
            printf("incoming %s\n", r->method);
            queue_push(mq->incoming, r);
        }
        r = NULL;
        mutex_unlock(&mq->fs_lock);
    }

    return NULL;
}

/* my aux-function */
void check_response(MessageQueue *mq, char **body) {
    Request *r = queue_pop(mq->incoming);

    if (streq(r->method, "200"))
        printf("OK\n");
    else
        printf("%s\n", r->uri);

    if (body)
        *body = strdup(r->body);
    request_delete(r);
}

Request *get_response(MessageQueue *mq) {
    char buffer[512];
    Request * r;

    while (!fgets(buffer, 512, mq->fs))
        continue;

    while (fgets(buffer, 512, mq->fs) && !streq(buffer, "\r\n")) {
        continue;
    }
    char *succ = fgets(buffer, 512, mq->fs);
    r = request_create(NULL, NULL, buffer);

    return r;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
