//Bibliotecas Padrão C
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

//Bibliotecas IDF
#include "driver/uart.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h" 
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"

//Bibliotecas Próprias
#include "gps.h"
#include "acel.h"
#include "mag.h"
#include "ble.h"
//#include "DM860.h"
//#include "A4988.h"

//Funções de Controle
#define Step_size 0.1125 //graus
#define Step_time 0.01 //s


struct estado{ //estado do sistema
    double alt; //altitue
    double azim; //azimute
};

struct motor{
  bool dir;//sentido(true-horário)/false(anti-horário)
  int n_steps;//número de passos
  double act_time;//tempo de ativção do sinal
};

double mmod(double a,double n){
  return a - floor(a/n) * n;
}

void set_move(double coord_atual,double coord_alvo, struct motor *Nema ){
double a=coord_alvo-coord_atual;
double turn_angle=mmod(a+180.0,360.0)-180;
if(turn_angle > 0){
  Nema->dir=true;
  
}
else{
  Nema->dir=false;
}

Nema->n_steps=abs(turn_angle)/Step_size;
Nema->act_time=Nema->n_steps * Step_time;

return;
}

void app_main(void){
    struct estado estado_atual,estado_alvo;
    struct motor Nema17,Nema34;//azim(17)/alt(34)

    //Informações genéricas para teste

    //Estado retirado da leitura dos sesnores
    estado_atual.alt= 12.5;
    estado_atual.azim=58.8;

    //posição de um corpo qualquer
    estado_alvo.alt=47.6;
    estado_alvo.azim=123.3;

    set_move(estado_atual.alt,estado_alvo.alt, &Nema34);
    set_move(estado_atual.azim,estado_alvo.azim, &Nema17);

    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), WiFi%s%s, ",
            CONFIG_IDF_TARGET,
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
