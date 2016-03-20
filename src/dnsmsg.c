#include "dnsmsg.h"
#include "wrappers.h"
#include "dnstypes.h"

#include <stdio.h>
#include <string.h>

#define MAX_NAME_LEN 256

void append_to_buf8(uint8_t **buf, uint8_t val);
void append_to_buf16(uint8_t **buf, uint16_t val);
uint16_t dnsmsg_header_get_opt(struct dnsmsg_header *);

struct dnsmsg_header *new_dnsmsg_header(uint16_t options) {
    struct dnsmsg_header *result = _malloc(sizeof(struct dnsmsg_header));
    result->id = rand();
    result->qr = (options & DNSMSG_OPT_QR) > 0;
    result->tc = (options & DNSMSG_OPT_TC) > 0;
    result->rd = (options & DNSMSG_OPT_RD) > 0;
    result->ra = (options & DNSMSG_OPT_RA) > 0;
    return result;
}

struct dnsmsg_query *new_dnsmsg_query(void) {
    struct dnsmsg_query *result = _malloc(sizeof(struct dnsmsg_query));
    return result;
}

dnsmsg_t *dnsmsg_new(uint16_t options) {
    dnsmsg_t *result = _malloc(sizeof(dnsmsg_t));
    result->header = new_dnsmsg_header(options);
    result->query = new_dnsmsg_query();
    result->answer = NULL;
    result->auth = NULL;
    result->additional = NULL;
    return result;
}

void dnsmsg_print(dnsmsg_t *msg) {
    printf("-----------------------------------------------\n");
    printf(" DNS PACKET HEADER\n");
    printf("   Response Code: 0x%x\n", msg->header->rcode);
    printf("   Flags: 0x%x\n", dnsmsg_header_get_opt(msg->header));
    printf("   Questions: %d\n", msg->header->qdcount);
    printf("   Answers: %d\n", msg->header->ancount);
    printf("   Authority: %d\n", msg->header->nscount);
    printf("   Additional: %d\n", msg->header->arcount);
    printf("-----------------------------------------------\n");
}

void dnsmsg_free(dnsmsg_t *msg) {
    if (msg->header) free(msg->header); 
    if (msg->query) {
        if (msg->query->qname) free(msg->query->qname);
        free(msg->query);
    }
    free(msg);
}

dnsmsg_t *dnsmsg_construct(uint8_t *buf, ssize_t buf_len) {
    return NULL;
}

dnsmsg_t *dnsmsg_create_query(const char *name, int qtype, int qclass) {
    dnsmsg_t *result = dnsmsg_new(DNSMSG_OPT_RD);
    result->header->opcode = DNS_OPCODE_QUERY;
    result->header->qdcount = 1;
    
    result->query->qname = dnsmsg_create_name(name);
    result->query->qtype = qtype;
    result->query->qclass = qclass;
    return result;
}

uint8_t *dnsmsg_create_name(const char *name) {
    int i = 0, 
        next_len = 0,
        buf_index = 1,
        label_len = 0;
    uint8_t *buf = malloc(sizeof(uint8_t) * MAX_NAME_LEN);
    
    size_t namelen = strlen(name);
    for (; i <= namelen; i++) {
        if (name[i] == '.' || name[i] == '\0') {
            buf[next_len] = label_len;
            label_len = 0;
            next_len = buf_index++;
        } else {
            buf[buf_index++] = name[i];
            label_len++;
        }
    }
    buf[buf_index++] = 0;
    
    return buf;
}

int dnsmsg_to_bytes(dnsmsg_t *msg, uint8_t **buf) {
    int num_bytes = 0;
    
    num_bytes += dnsmsg_to_bytes_header(msg, buf);    
    num_bytes += dnsmsg_to_bytes_query(msg, buf);
    
    return num_bytes;
}

int dnsmsg_to_bytes_header(dnsmsg_t *msg, uint8_t **buf) {
    int num_bytes = 0;
    
    struct dnsmsg_header *head = msg->header;

    append_to_buf16(buf, head->id);
    append_to_buf16(buf, dnsmsg_header_get_opt(head));
    append_to_buf16(buf, head->qdcount);
    append_to_buf16(buf, head->ancount);
    append_to_buf16(buf, head->nscount);
    append_to_buf16(buf, head->arcount);
    
    num_bytes += 12;
    
    return num_bytes;
}

int dnsmsg_to_bytes_query(dnsmsg_t *msg, uint8_t **buf) {
    int num_bytes = 0,
        i = 0;
    
    // Convert qname
    uint8_t *qname = msg->query->qname;
    for (; qname[i] != 0; i++) {
        append_to_buf8(buf, qname[i]);
        num_bytes++;
    }
    append_to_buf8(buf, qname[i]); // Add the last 0 in there too
    num_bytes++;
    
    // Convert qtype
    append_to_buf16(buf, msg->query->qtype);
    num_bytes += 2;
    
    // Convert qclass
    append_to_buf16(buf, msg->query->qclass);
    num_bytes += 2;
    
    return num_bytes;
}

void append_to_buf8(uint8_t **buf, uint8_t val) {
    *(*buf)++ = val;
}

void append_to_buf16(uint8_t **buf, uint16_t val) {
    append_to_buf8(buf, (val & 0xff00) >> 8);
    append_to_buf8(buf, val & 0xff);
}

uint16_t dnsmsg_header_get_opt(struct dnsmsg_header *h) {
    return  h->qr << 15 | h->opcode << 14 | h->aa << 10 | h->tc << 9 | 
        h->rd << 8 | h->ra << 7 |  h->z << 6 | h->rcode;
}
