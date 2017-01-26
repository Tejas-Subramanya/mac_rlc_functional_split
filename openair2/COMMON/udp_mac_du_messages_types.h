#ifndef UDP_MAC_DU_MESSAGES_TYPES_H_
#define UDP_MAC_DU_MESSAGES_TYPES_H_

#define UDP_MAC_DU_INIT(mSGpTR)    	(mSGpTR)->ittiMsg.udp_mac_du_init
#define UDP_MAC_DU_DATA_SEND(mSGpTR)	(mSGpTR)->ittiMsg.udp_mac_du_data_send
#define UDP_MAC_DU_DATA_RECEIVE(mSGpTR)	(mSGpTR)->ittiMsg.udp_mac_du_data_receive   

typedef struct {
  uint32_t  port;
  char     *address;
} udp_mac_du_init_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  buffer_offset;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_mac_du_data_send_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_mac_du_data_receive_t;

#endif
