#include "contiki.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-debug.h"
//#include "net/uip-ds6.h"
#include "net/rpl/rpl.h"
#include "dev/leds.h"

#include <stdio.h>

PROCESS(intermediate_router_process, "Intermediate Router");
AUTOSTART_PROCESSES(&intermediate_router_process);

PROCESS_THREAD(intermediate_router_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t ipaddr;

  PROCESS_BEGIN();

  printf("Intermediate Router started\n");

  /* Define o endereço IP global com base no prefixo da rede */
  uip_ip6addr(&ipaddr, 0xaaaa,0,0,0,0,0,0,0x2); // Exemplo: roteador intermediário .2
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
  printf("Global IPv6 address set: ");
  uip_debug_ipaddr_print(&ipaddr);
  printf("\n");

  /* Timer para checar o DAG periodicamente */
  etimer_set(&periodic_timer, CLOCK_SECOND * 60);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    printf("RPL status:\n");

    rpl_dag_t *dag = rpl_get_any_dag();
    if(dag != NULL) {
      printf("Joined DAG with instance ID: %u\n", dag->instance->instance_id);
      printf("Rank: %u\n", dag->rank);
	

      printf("Routing table:\n");
      uip_ds6_route_t *r;
      for(r = uip_ds6_route_head(); r != NULL; r = uip_ds6_route_next(r)) {
        uip_ipaddr_t *nexthop = uip_ds6_route_nexthop(r);
        printf("- Destination: ");
        uip_debug_ipaddr_print(&r->ipaddr);
        printf(" | Prefix length: %u", r->length);
        printf(" | Next hop: ");
        uip_debug_ipaddr_print(nexthop);
        printf("\n");
    }
    } else {
      printf("Not part of any DAG\n");
    }

    etimer_reset(&periodic_timer);
  }

  PROCESS_END();
}

