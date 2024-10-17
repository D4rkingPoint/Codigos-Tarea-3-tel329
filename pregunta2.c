#include "contiki.h"
#include "net/routing/routing.h"
#include "mqtt.h"
#include "mqtt-prop.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-hal.h"
#include "dev/leds.h"
#include "os/sys/log.h"
#include "mqtt-client.h"

#include <string.h>
#include <stdlib.h>  // Para rand() y srand()
#include <time.h>    // Para time()

#define LOG_MODULE "mqtt-client"
#define LOG_LEVEL LOG_LEVEL_DBG

#define TEMP_MIN 15  // Temperatura mínima
#define TEMP_MAX 30  // Temperatura máxima

PROCESS(mqtt_client_process, "MQTT Client");
AUTOSTART_PROCESSES(&mqtt_client_process);

static struct mqtt_connection conn;
static struct etimer publish_periodic_timer;
static char app_buffer[512];
static uint16_t seq_nr_value = 0;
static char pub_topic[64];

// Función para publicar la temperatura
static void publish(void) {
  // Generar una temperatura aleatoria
  int temperature = (rand() % (TEMP_MAX - TEMP_MIN + 1)) + TEMP_MIN;

  // Publicar el mensaje con la temperatura
  int len;
  int remaining = sizeof(app_buffer);
  char *buf_ptr = app_buffer;

  seq_nr_value++;

  len = snprintf(buf_ptr, remaining,
                 "{"
                 "\"Temperature\":%d,"  // Incluir temperatura
                 "\"Seq #\":%d,"
                 "\"Uptime (sec)\":%lu"
                 "}",
                 temperature, seq_nr_value, clock_seconds());

  if(len < 0 || len >= remaining) {
    LOG_ERR("Buffer too short. Have %d, need %d + \\0\n", remaining, len);
    return;
  }

  mqtt_publish(&conn, NULL, "temperature", (uint8_t *)app_buffer,
               strlen(app_buffer), MQTT_QOS_LEVEL_0, MQTT_RETAIN_OFF);

  LOG_DBG("Published temperature: %d\n", temperature);  // Imprimir la temperatura
  printf("Temperature sent: %d\n", temperature);      // Imprimir en consola
}

// Función para inicializar la configuración
static int init_config() {
  // Definir el tema de publicación
  snprintf(pub_topic, sizeof(pub_topic), "temperature");
  return 1;
}

// Proceso principal
PROCESS_THREAD(mqtt_client_process, ev, data) {
  PROCESS_BEGIN();

  srand(time(NULL));  // Inicializa el generador de números aleatorios

  printf("MQTT Client Process\n");

  if(init_config() != 1) {
    PROCESS_EXIT();
  }

  etimer_set(&publish_periodic_timer, 30 * CLOCK_SECOND);  // Intervalo de publicación

  while(1) {
    PROCESS_YIELD();

    if(ev == PROCESS_EVENT_TIMER && data == &publish_periodic_timer) {
      publish();  // Publicar la temperatura aleatoria
      etimer_reset(&publish_periodic_timer);  // Reiniciar el temporizador
    }
  }

  PROCESS_END();
}
