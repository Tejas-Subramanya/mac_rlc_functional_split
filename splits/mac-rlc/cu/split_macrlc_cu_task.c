// License to be added FBK CREATE-NET

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "queue.h"
#include "intertask_interface.h"
#include "assertions.h"
#include "split_macrlc_cu_task.h"

#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "msc.h"


#define IPV4_ADDR    "%u.%u.%u.%u"
#define IPV4_ADDR_FORMAT(aDDRESS)               \
    (uint8_t)((aDDRESS)  & 0x000000ff),         \
    (uint8_t)(((aDDRESS) & 0x0000ff00) >> 8 ),  \
    (uint8_t)(((aDDRESS) & 0x00ff0000) >> 16),  \
    (uint8_t)(((aDDRESS) & 0xff000000) >> 24)


struct udp_rlc_cu_socket_desc_s {
  int       sd;              /* Socket descriptor to use */

  pthread_t listener_thread; /* Thread affected to recv */

  char     *local_address;   /* Local ipv4 address to use */
  uint16_t  local_port;      /* Local port to use */

  task_id_t task_id;         /* Task who has requested the new endpoint */

  STAILQ_ENTRY(udp_rlc_cu_socket_desc_s) entries;
};

static STAILQ_HEAD(udp_rlc_cu_socket_list_s, udp_rlc_cu_socket_desc_s) udp_rlc_cu_socket_list;
static pthread_mutex_t udp_rlc_cu_socket_list_mutex = PTHREAD_MUTEX_INITIALIZER;


static
struct udp_rlc_cu_socket_desc_s *
udp_rlc_cu_get_socket_desc(task_id_t task_id);

void udp_rlc_cu_process_file_descriptors(
  struct epoll_event *events,
  int nb_events);

static
int
udp_rlc_cu_create_socket(
  int port,
  char *ip_addr,
  task_id_t task_id);

int
udp_rlc_cu_send_to(
  int sd,
  uint16_t port,
  uint32_t address,
  const uint8_t *buffer,
  uint32_t length);

void udp_rlc_cu_receiver(struct udp_rlc_cu_socket_desc_s *udp_sock_pP);

void *udp_rlc_cu_task(void *args_p);

int udp_rlc_cu_init(const Enb_properties_t *enb_config_p);
/* @brief Retrieve the descriptor associated with the task_id
 */
static
struct udp_rlc_cu_socket_desc_s *udp_rlc_cu_get_socket_desc(task_id_t task_id)
{
  struct udp_rlc_cu_socket_desc_s *udp_sock_p = NULL;

#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
  LOG_T(SPLIT_MACRLC_CU, "Looking for task %d\n", task_id);
#endif

  STAILQ_FOREACH(udp_sock_p, &udp_rlc_cu_socket_list, entries) {
    if (udp_sock_p->task_id == task_id) {
#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
      LOG_T(SPLIT_MACRLC_CU, "Found matching task desc\n");
#endif
      break;
    }
  }
  return udp_sock_p;
}

void udp_rlc_cu_process_file_descriptors(struct epoll_event *events, int nb_events)
{
  int                       i;
  struct udp_rlc_cu_socket_desc_s *udp_sock_p = NULL;

  if (events == NULL) {
    return;
  }

  for (i = 0; i < nb_events; i++) {
    STAILQ_FOREACH(udp_sock_p, &udp_rlc_cu_socket_list, entries) {
      if (udp_sock_p->sd == events[i].data.fd) {
#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
        LOG_D(SPLIT_MACRLC_CU, "Found matching task desc\n");
#endif
        udp_rlc_cu_receiver(udp_sock_p);
        break;
      }
    }
  }
}

static
int udp_rlc_cu_create_socket(int port, char *ip_addr, task_id_t task_id)
{

  struct udp_rlc_cu_socket_desc_s  *udp_socket_desc_p = NULL;
  int                       sd, rc;
  struct sockaddr_in        sin;

  LOG_I(SPLIT_MACRLC_CU, "Initializing UDP RLC CU for local address %s with port %d\n", ip_addr, port);

  sd = socket(AF_INET, SOCK_DGRAM, 0);
  AssertFatal(sd > 0, "UDP: Failed to create new socket: (%s:%d)\n", strerror(errno), errno);

  int enable = 1;
  if ((rc = setsockopt (sd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof (int))) < 0) {
    close(sd);
    AssertFatal(rc >= 0, "Error setting socket options\n");
  }

  memset(&sin, 0, sizeof(struct sockaddr_in));
  sin.sin_family      = AF_INET;
  sin.sin_port        = htons(port);

  if (ip_addr == NULL) {
    sin.sin_addr.s_addr = inet_addr(INADDR_ANY);
  } else {
    sin.sin_addr.s_addr = inet_addr(ip_addr);
  }

  if ((rc = bind(sd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in))) < 0) {
    close(sd);
    AssertFatal(rc >= 0, "UDP RLC CU: Failed to bind socket: (%s:%d) address %s port %u\n",
                strerror(errno), errno, ip_addr, port);
  }

  /* Create a new descriptor for this connection */
  udp_socket_desc_p = calloc(1, sizeof(struct udp_rlc_cu_socket_desc_s));

  DevAssert(udp_socket_desc_p != NULL);

  udp_socket_desc_p->sd            = sd;
  udp_socket_desc_p->local_address = ip_addr;
  udp_socket_desc_p->local_port    = port;
  udp_socket_desc_p->task_id       = task_id;

  LOG_I(SPLIT_MACRLC_CU, "Inserting new descriptor for task %d, sd %d\n", udp_socket_desc_p->task_id, udp_socket_desc_p->sd);
  pthread_mutex_lock(&udp_rlc_cu_socket_list_mutex);
  STAILQ_INSERT_TAIL(&udp_rlc_cu_socket_list, udp_socket_desc_p, entries);
  pthread_mutex_unlock(&udp_rlc_cu_socket_list_mutex);

  itti_subscribe_event_fd(TASK_SPLIT_MACRLC_CU, sd);
  LOG_I(SPLIT_MACRLC_CU, "Initializing UDP RLC CU for local address %s with port %d: DONE\n", ip_addr, port);
  return sd;
}

int
udp_rlc_cu_send_to(
  int sd,
  uint16_t port,
  uint32_t address,
  const uint8_t *buffer,
  uint32_t length)
{
  struct sockaddr_in to;
  socklen_t          to_length;

  if (sd <= 0 || ((buffer == NULL) && (length > 0))) {
    LOG_E(SPLIT_MACRLC_CU, "udp_rlc_cu_send_to: bad param\n");
    return -1;
  }

  memset(&to, 0, sizeof(struct sockaddr_in));
  to_length = sizeof(to);

  to.sin_family      = AF_INET;
  to.sin_port        = htons(port);
  to.sin_addr.s_addr = address;

  if (sendto(sd, (void *)buffer, (size_t)length, 0, (struct sockaddr *)&to,
             to_length) < 0) {
    LOG_E(SPLIT_MACRLC_CU,
          "[SD %d] Failed to send data to "IPV4_ADDR" on port %d, buffer size %u\n",
          sd, IPV4_ADDR_FORMAT(address), port, length);
    return -1;
  }

#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
  LOG_I(SPLIT_MACRLC_CU, "[SD %d] Successfully sent to "IPV4_ADDR
        " on port %d, buffer size %u, buffer address %x\n",
        sd, IPV4_ADDR_FORMAT(address), port, length, buffer);
#endif
  return 0;
}


void udp_rlc_cu_receiver(struct udp_rlc_cu_socket_desc_s *udp_sock_pP)
{
  uint8_t                   l_buffer[2048];
  int                n;
  socklen_t          from_len;
  struct sockaddr_in addr;
  MessageDef               *message_p        = NULL;
  udp_rlc_cu_data_receive_t           *udp_rlc_cu_data_receive_p   = NULL;
  uint8_t                  *forwarded_buffer = NULL;

  if (1) {
    from_len = (socklen_t)sizeof(struct sockaddr_in);

    if ((n = recvfrom(udp_sock_pP->sd, l_buffer, sizeof(l_buffer), 0,
                      (struct sockaddr *)&addr, &from_len)) < 0) {
      LOG_E(SPLIT_MACRLC_CU, "Recvfrom failed %s\n", strerror(errno));
      return;
    } else if (n == 0) {
      LOG_W(SPLIT_MACRLC_CU, "Recvfrom returned 0\n");
      return;
    } else {
      forwarded_buffer = itti_malloc(TASK_SPLIT_MACRLC_CU, udp_sock_pP->task_id, n*sizeof(uint8_t));
      DevAssert(forwarded_buffer != NULL);
      memcpy(forwarded_buffer, l_buffer, n);
      message_p = itti_alloc_new_message(TASK_SPLIT_MACRLC_CU, UDP_DATA_IND);
      DevAssert(message_p != NULL);
      udp_rlc_cu_data_receive_p = &message_p->ittiMsg.udp_rlc_cu_data_receive;
      udp_rlc_cu_data_receive_p->buffer        = forwarded_buffer;
      udp_rlc_cu_data_receive_p->buffer_length = n;
      udp_rlc_cu_data_receive_p->peer_port     = htons(addr.sin_port);
      udp_rlc_cu_data_receive_p->peer_address  = addr.sin_addr.s_addr;

#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
      LOG_I(SPLIT_MACRLC_CU, "Msg of length %d received from %s:%u\n",
            n, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif

      if (itti_send_msg_to_task(udp_sock_pP->task_id, INSTANCE_DEFAULT, message_p) < 0) {
        LOG_I(SPLIT_MACRLC_CU, "Failed to send message %d to task %d\n",
              UDP_DATA_IND,
              udp_sock_pP->task_id);
        return;
      }
    }
  }
}


void *udp_rlc_cu_task(void *args_p)
{
  int                 nb_events;
  struct epoll_event *events;
  MessageDef         *received_message_p    = NULL;
  udp_rlc_cu_init(NULL);

  itti_mark_task_ready(TASK_SPLIT_MACRLC_CU);
  MSC_START_USE();

  while(1) {
    itti_receive_msg(TASK_SPLIT_MACRLC_CU, &received_message_p);
   
#if defined(LOG_UDP) && LOG_UDP > 0
    LOG_D(SPLIT_MACRLC_CU, "Got message %p\n", &received_message_p);
#endif

    if (received_message_p != NULL) {

      switch (ITTI_MSG_ID(received_message_p)) {
      case UDP_RLC_CU_INIT: {
        LOG_D(SPLIT_MACRLC_CU, "Received UDP_RLC_CU_INIT\n");
        udp_rlc_cu_init_t *udp_rlc_cu_init_p;
        udp_rlc_cu_init_p = &received_message_p->ittiMsg.udp_rlc_cu_init;
        udp_rlc_cu_create_socket(
          udp_rlc_cu_init_p->port,
          udp_rlc_cu_init_p->address,
          ITTI_MSG_ORIGIN_ID(received_message_p));
      }
      break;

      case UDP_RLC_CU_DATA_SEND: {
#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
        LOG_D(SPLIT_MACRLC_CU, "Received UDP_RLC_CU_DATA_SEND\n");
#endif
        int     udp_sd = -1;
        ssize_t bytes_written;

        struct udp_rlc_cu_socket_desc_s *udp_sock_p = NULL;
        udp_rlc_cu_data_send_t           *udp_rlc_cu_data_send_p;
        struct sockaddr_in        peer_addr;

        udp_rlc_cu_data_send_p = &received_message_p->ittiMsg.udp_rlc_cu_data_send;

        memset(&peer_addr, 0, sizeof(struct sockaddr_in));

        peer_addr.sin_family       = AF_INET;
        peer_addr.sin_port         = htons(udp_rlc_cu_data_send_p->peer_port);
        peer_addr.sin_addr.s_addr  = udp_rlc_cu_data_send_p->peer_address;

        pthread_mutex_lock(&udp_rlc_cu_socket_list_mutex);
        udp_sock_p = udp_rlc_cu_get_socket_desc(ITTI_MSG_ORIGIN_ID(received_message_p));

        if (udp_sock_p == NULL) {
          LOG_E(SPLIT_MACRLC_CU,
                "Failed to retrieve the udp socket descriptor "
                "associated with task %d\n",
                ITTI_MSG_ORIGIN_ID(received_message_p));
          pthread_mutex_unlock(&udp_rlc_cu_socket_list_mutex);

          if (udp_rlc_cu_data_send_p->buffer) {
            itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), udp_rlc_cu_data_send_p->buffer);
          }

          goto on_error;
        }

        udp_sd = udp_sock_p->sd;
        pthread_mutex_unlock(&udp_rlc_cu_socket_list_mutex);

#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
        LOG_D(SPLIT_MACRLC_CU, "[%d] Sending message of size %u to "IPV4_ADDR" and port %u\n",
              udp_sd,
              udp_rlc_cu_data_send_p->buffer_length,
              IPV4_ADDR_FORMAT(udp_rlc_cu_data_send_p->peer_address),
              udp_rlc_cu_data_send_p->peer_port);
#endif
        bytes_written = sendto(
                          udp_sd,
                          &udp_rlc_cu_data_send_p->buffer[udp_rlc_cu_data_send_p->buffer_offset],
                          udp_rlc_cu_data_send_p->buffer_length,
                          0,
                          (struct sockaddr *)&peer_addr,
                          sizeof(struct sockaddr_in));

        if (bytes_written != udp_rlc_cu_data_send_p->buffer_length) {
          LOG_E(SPLIT_MACRLC_CU, "There was an error while writing to socket %u rc %d"
                "(%d:%s)\n",
                udp_sd, bytes_written, errno, strerror(errno));
        }

        itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), udp_rlc_cu_data_send_p->buffer);
      }
      break;

      case TERMINATE_MESSAGE: {
        LOG_W(SPLIT_MACRLC_CU, "Received TERMINATE_MESSAGE\n");
        itti_exit_task();
      }
      break;

      case MESSAGE_TEST: {
      } break;

      default: {
        LOG_W(SPLIT_MACRLC_CU, "Unkwnon message ID %d:%s\n",
              ITTI_MSG_ID(received_message_p),
              ITTI_MSG_NAME(received_message_p));
      }
      break;
      }

on_error:
      itti_free (ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
      received_message_p = NULL;
    }

    nb_events = itti_get_events(TASK_SPLIT_MACRLC_CU, &events);

    /* Now handle notifications for other sockets */
    if (nb_events > 0) {
#if defined(LOG_SPLIT_MACRLC_CU) && LOG_SPLIT_MACRLC_CU > 0
      LOG_D(SPLIT_MACRLC_CU, "UDP RLC CU task Process %d events\n",nb_events);
#endif
      udp_rlc_cu_process_file_descriptors(events, nb_events);
    }

    //VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UDP_ENB_TASK, VCD_FUNCTION_OUT);
  }

  LOG_N(SPLIT_MACRLC_CU, "Task UDP RLC CU exiting\n");
  return NULL;
}

int udp_rlc_cu_init(const Enb_properties_t *enb_config_p)
{
  LOG_I(SPLIT_MACRLC_CU, "Initializing UDP RLC CU task \n");
  STAILQ_INIT(&udp_rlc_cu_socket_list);
  LOG_I(SPLIT_MACRLC_CU, "Initializing UDP RLC CU task : DONE\n");
  return 0;
}
