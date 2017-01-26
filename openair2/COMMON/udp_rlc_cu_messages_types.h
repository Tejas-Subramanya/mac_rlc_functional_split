#ifndef UDP_RLC_CU_MESSAGES_TYPES_H_
#define UDP_RLC_CU_MESSAGES_TYPES_H_

#define UDP_RLC_CU_INIT(mSGpTR)    	(mSGpTR)->ittiMsg.udp_rlc_cu_init
#define UDP_RLC_CU_DATA_SEND(mSGpTR)	(mSGpTR)->ittiMsg.udp_rlc_cu_data_send
#define UDP_RLC_CU_DATA_RECEIVE(mSGpTR)	(mSGpTR)->ittiMsg.udp_rlc_cu_data_receive   

typedef struct {
  uint32_t  port;
  char     *address;
} udp_rlc_cu_init_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  buffer_offset;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_rlc_cu_data_send_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_rlc_cu_data_receive_t;

#endif
