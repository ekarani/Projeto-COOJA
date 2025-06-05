#include "contiki.h"
#include "contiki-net.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
#include "net/ip/uip-udp-packet.h"
#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_PORT_CLIENT 8765
#define UDP_PORT_SERVER 5678

static struct uip_udp_conn *udp_conn;

PROCESS(intermediate_router_process, "Intermediate Router");
AUTOSTART_PROCESSES(&intermediate_router_process);

/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  if(uip_newdata()) {
    uip_ipaddr_t *source = &UIP_IP_BUF->srcipaddr;

    // Log origem
    printf("Intermediate Router: received data from ");
    uip_debug_ipaddr_print(source);
    printf("\n");

    // Adiciona rota reversa (útil para reply do servidor)
    uip_ds6_route_add(source, 128, source);

    // Define endereço do servidor manualmente
    uip_ipaddr_t server_ipaddr;
    uiplib_ipaddrconv("aaaa::ff:fe00:1", &server_ipaddr);

    // Encaminha para servidor
    uip_udp_packet_sendto(udp_conn, uip_appdata, uip_datalen(),
                          &server_ipaddr, UIP_HTONS(UDP_PORT_SERVER));

    printf("Intermediate Router: Forwarded to server: ");
    uip_debug_ipaddr_print(&server_ipaddr);
    printf("\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(intermediate_router_process, ev, data)
{
  static uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();

  printf("Starting 'Intermediate Router'\n");

  // Delay para aguardar DAG RPL
  static struct etimer join_timer;
  etimer_set(&join_timer, CLOCK_SECOND * 10);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&join_timer));

  // Define endereço global manualmente
  uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0xc30c, 0, 0, 1);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);

  printf("Global IPv6 address set: ");
  uip_debug_ipaddr_print(&ipaddr);
  printf("\n");

  // Inicializa socket UDP
  udp_conn = udp_new(NULL, UIP_HTONS(UDP_PORT_CLIENT), NULL);
  if(udp_conn == NULL) {
    printf("No UDP connection available, exiting process!\n");
    PROCESS_EXIT();
  }

  udp_bind(udp_conn, UIP_HTONS(UDP_PORT_CLIENT));
  printf("Listening on UDP port %u\n", UDP_PORT_CLIENT);

  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }
  }

  PROCESS_END();
}

