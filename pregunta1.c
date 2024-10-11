#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "coap-blocking-api.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL  LOG_LEVEL_APP

/* Dirección del nodo raíz */
#define SERVER_EP "coap://[fd00::201:1:1:1]"

#define TOGGLE_INTERVAL 10

PROCESS(er_example_client, "Erbium Example Client");
AUTOSTART_PROCESSES(&er_example_client);

static struct etimer et;

/* Función que maneja las respuestas del servidor CoAP */
void client_chunk_handler(coap_message_t *response) {
  const uint8_t *chunk;

  if(response == NULL) {
    puts("Request timed out");
    return;
  }

  int len = coap_get_payload(response, &chunk);

  printf("|%.*s", len, (char *)chunk);
}

PROCESS_THREAD(er_example_client, ev, data)
{
  static coap_endpoint_t server_ep;
  static coap_message_t request[1];  /* El paquete puede tratarse como puntero. */
  PROCESS_BEGIN();

  /* Parsear la dirección del servidor (nodo raíz) */
  coap_endpoint_parse(SERVER_EP, strlen(SERVER_EP), &server_ep);

  /* Iniciar el timer para que el cliente envíe datos periódicamente */
  etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);

  while(1) {
    PROCESS_YIELD();

    if(etimer_expired(&et)) {
      printf("--Toggle timer--\n");

      /* Generar una temperatura aleatoria entre 15 y 35 grados */
      int temp_value = 15 + (random_rand() % 21);
      printf("Generated temperature: %d\n", temp_value);  // Depuración

      /* Enviar como cadena de texto */
      char temp_str[16];
      snprintf(temp_str, sizeof(temp_str), "%d", temp_value);

      /* Preparar la solicitud, TID se establece en COAP_BLOCKING_REQUEST() */
      coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
      coap_set_header_uri_path(request, "/sensors/temperature"); /* Enviar a /sensor/temperature */

      coap_set_payload(request, (uint8_t *)temp_str, strlen(temp_str));

      /* Mostrar información de depuración */
      LOG_INFO_COAP_EP(&server_ep);
      LOG_INFO_("\n");

      /* Enviar la solicitud */
      COAP_BLOCKING_REQUEST(&server_ep, request, client_chunk_handler);

      printf("Temperature sent");
      printf("\n--Done--\n");

      /* Reiniciar el timer */
      etimer_reset(&et);
    }
  }

  PROCESS_END();
}
