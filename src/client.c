/* client.c: Message Queue Client */

#include "mq/client.h"
#include "mq/logging.h"
#include "mq/socket.h"
#include "mq/string.h"

/* Internal Constants */

#define SENTINEL "SHUTDOWN"

/* Internal Prototypes */

void * mq_pusher(void *);
void * mq_puller(void *);

/* External Functions */

/**
 * Create Message Queue withs specified name, host, and port.
 * @param   name        Name of client's queue.
 * @param   host        Address of server.
 * @param   port        Port of server.
 * @return  Newly allocated Message Queue structure.
 */
MessageQueue * mq_create(const char *name, const char *host, const char *port) {
    MessageQueue * mq = (MessageQueue *)malloc(sizeof(MessageQueue));

    if (mq)
    {
        strcpy(mq->name, name);
        strcpy(mq->host, host);
        strcpy(mq->port, port);

        mq->outgoing = queue_create();
        mq->incoming = queue_create();

        mq->shutdown = false;
    }

    return mq;
}

/**
 * Delete Message Queue structure (and internal resources).
 * @param   mq      Message Queue structure.
 */
void mq_delete(MessageQueue *mq) {
    if (mq)
    {
        queue_delete(mq->outgoing);
        queue_delete(mq->incoming);
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
    queue_push(mq->outgoing, ("PUT", uri, body));
}

/**
 * Retrieve one message (by taking Request from incoming queue).
 * @param   mq      Message Queue structure.
 * @return  Newly allocated message body (must be freed).
 */
char * mq_retrieve(MessageQueue *mq) {
    char uri[64];
    sprintf(uri, "/topic/%s", mq->name);
    queue_push(mq->outgoing, ("GET", uri, NULL));

    Request * r = queue_pop(mq->incoming);

    return strdup(r->body);
}

/**
 * Subscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to subscribe to.
 **/
void mq_subscribe(MessageQueue *mq, const char *topic) {
    char uri[64];
    sprintf(uri, "/subscription/%s/%s", mq->name, topic);
    queue_push(mq->outgoing, ("PUT", uri, NULL));
}

/**
 * Unubscribe to specified topic.
 * @param   mq      Message Queue structure.
 * @param   topic   Topic string to unsubscribe from.
 **/
void mq_unsubscribe(MessageQueue *mq, const char *topic) {
    char uri[64];
    sprintf(uri, "/subscription/%s/%s", mq->name, topic);
    queue_push(mq->outgoing, ("DELETE", uri, NULL));
}

/**
 * Start running the background threads:
 *  1. First thread should continuously send requests from outgoing queue.
 *  2. Second thread should continuously receive reqeusts to incoming queue.
 * @param   mq      Message Queue structure.
 */
void mq_start(MessageQueue *mq) {
    Thread pusher, puller;
    thread_create(&pusher, NULL, mq_pusher, NULL);
    thread_create(&puller, NULL, mq_puller, NULL);
}

/**
 * Stop the message queue client by setting shutdown attribute and sending
 * sentinel messages
 * @param   mq      Message Queue structure.
 */
void mq_stop(MessageQueue *mq) {
}

/**
 * Returns whether or not the message queue should be shutdown.
 * @param   mq      Message Queue structure.
 */
bool mq_shutdown(MessageQueue *mq) {
    return false;
}

/* Internal Functions */

/**
 * Pusher thread takes messages from outgoing queue and sends them to server.
 **/
void * mq_pusher(void *arg) {
    while (true)
    {
        // takes message

        // send to server
    }

    return NULL;
}

/**
 * Puller thread requests new messages from server and then puts them in
 * incoming queue.
 **/
void * mq_puller(void *arg) {
    while (true)
    {
        // get messages from server

        // put into queue
    }

    return NULL;
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */
