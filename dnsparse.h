


#pragma pack (1)
typedef struct dnsheader {
    uint16_t id, flags;
    uint16_t qdcount, ancount, nscount, arcount;
} dnsheader;
#pragma pack ()

void print_hex(char *buffer, size_t n) ;
int parse_label(char *buffer, char *label);
void parse_dns(char *buffer, size_t n);
