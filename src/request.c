/* request.c: Request structure */

#include "mq/request.h"

#include <stdlib.h>
#include <string.h>

/**
 * Create Request structure.
 * @param   method      Request method string.
 * @param   uri         Request uri string.
 * @param   body        Request body string.
 * @return  Newly allocated Request structure.
 */
Request * request_create(const char *method, const char *uri, const char *body) {
    Request * pr = (Request *)malloc(sizeof(Request));
    if (pr)
    {
        pr->method = method ? strdup(method) : NULL;
        pr->uri = uri ? strdup(uri) : NULL;
        pr->body = body ? strdup(body) : NULL;
        pr->next = NULL;
    }

    return pr;
}

/**
 * Delete Request structure.
 * @param   r           Request structure.
 */
void request_delete(Request *r) {
    if (r)
    {
        free(r->method);
        free(r->uri);
        free(r->body);
    }
    free(r);
}

/**
 * Write HTTP Request to stream:
 *  
 *  $METHOD $URI HTTP/1.0\r\n
 *  Content-Length: Length($BODY)\r\n
 *  \r\n
 *  $BODY
 *      
 * @param   r           Request structure.
 * @param   fs          Socket file stream.
 */
void request_write(Request *r, FILE *fs) {
    fprintf(fs, "%s %s HTTP/1.0\r\n", r->method, r->uri);
    if (r->body)
        fprintf(fs, "Content-Length: %ld\r\n", strlen(r->body));
    fprintf(fs, "\r\n");
    if (r->body)
        fprintf(fs, "%s", r->body);
}

/* vim: set expandtab sts=4 sw=4 ts=8 ft=c: */ 
