#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_ARG 1
#define ESP_ERR_INVALID_STATE 2
#define ESP_ERR_NO_MEM 3
#define ESP_ERR_TIMEOUT 4
#define ESP_ERR_NOT_SUPPORTED 5
#define ESP_ERR_INVALID_SIZE 6
#define ESP_RETURN_ON_FALSE(c,e,...) do {if (!(c)) return (e);} while(0)
#define ESP_RETURN_ON_ERROR(c,...) do {esp_err_t e_=(c); if(e_) return e_;} while(0)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
#define APP_EPAPER_DEBUG_LOGI(...) ((void)0)
#define APP_EPAPER_DEBUG_LOGD(...) ((void)0)
#define APP_EPAPER_DEBUG_LOGE(...) ((void)0)
#define APP_EPAPER_DEBUG_LOGW(...) ((void)0)
#define IRAM_ATTR
#define ESP_INTR_FLAG_IRAM 0
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_8BIT 2
#define MALLOC_CAP_INTERNAL 4
#define MALLOC_CAP_DMA 8
#define SPI_TRANS_USE_RXDATA 1
#define GPIO_INTR_DISABLE 0
#define GPIO_INTR_NEGEDGE 1
#define GPIO_INTR_POSEDGE 2
#define GPIO_MODE_OUTPUT 1
#define GPIO_MODE_INPUT 2
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_PULLUP_DISABLE 0
#define pdFALSE 0
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
#define portYIELD_FROM_ISR(x) ((void)(x))
typedef int gpio_num_t;
typedef int gpio_int_type_t;
typedef int BaseType_t;
typedef unsigned TickType_t;
typedef void *SemaphoreHandle_t;
typedef void *spi_device_handle_t;
typedef struct {int intr_type,mode,pull_down_en,pull_up_en; uint64_t pin_bit_mask;} gpio_config_t;
typedef struct {unsigned flags; size_t length,rxlength; const void *tx_buffer; uint8_t rx_data[4];} spi_transaction_t;
void *heap_caps_malloc(size_t,unsigned);
void *heap_caps_malloc_prefer(size_t,size_t,...);
void heap_caps_free(void *);
int64_t esp_timer_get_time(void);
const char *esp_err_to_name(int);
int gpio_config(const gpio_config_t *);
int gpio_set_level(int,int);
int gpio_get_level(int);
int gpio_set_intr_type(int,int);
int gpio_install_isr_service(int);
int gpio_isr_handler_add(int,void (*)(void *),void *);
int gpio_isr_handler_remove(int);
int spi_device_transmit(void *,spi_transaction_t *);
void vTaskDelay(unsigned);
void *xSemaphoreCreateBinary(void);
int xSemaphoreTake(void *,unsigned);
void xSemaphoreGiveFromISR(void *,int *);
void vSemaphoreDelete(void *);
