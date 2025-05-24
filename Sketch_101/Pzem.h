#include <PZEM004Tv30.h>

#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17


PZEM004Tv30 pzem(Serial2, PZEM_RX_PIN, PZEM_TX_PIN);
