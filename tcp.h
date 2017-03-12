// Constants
#define TCP_HEADER_LEN 20
#define TCP_MAX_DATA_LEN 1004
#define RECV_WNDW_DEFAULT 5120

// Masks
#define ACK_FLAG_MASK 0x80
#define SYN_FLAG_MASK 0x40
#define FIN_FLAG_MASK 0x20

typedef struct {
  char flag_set;
} header_flags;

typedef struct {
  short src_port;
  short dest_port;
  int seq_num;
  int ack_num;
  char header_len;
  header_flags flags;
  short recv_wndw;
  short check_sum;
  short urgent_data_pointer;
} tcp_header;

typedef struct {
  tcp_header header;
  char[1004] data;
} tcp_packet; // 1024 bytes

void header_flags_init(header_flags *flags, int ack, int syn, int fin);
int ack_flagged(header_flags* flag);
int syn_flagged(header_flags* flag);
int fin_flagged(header_flags* flag);
void tcp_header_init(tcp_header *header, short src_port, short dest_port, int seq_num, int ack_num, header_flags flags, short recv_wndw);
void tcp_packet_int(tcp_packet *packet, tcp_header *header, void *data);

int send_tcp_packet(tcp_packet* send_packet, int sock_fd, int byte_to_write);
int recv_tcp_packet(tcp_packet* recv_packet, int sock_fd);
