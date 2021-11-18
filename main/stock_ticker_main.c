/* ESP32 Custom Stock Ticker LED Grid

See README.md for more info
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "api_config.h"

char buf[512];
char stockResult[512];
char stockPrice[6];
uint32_t stockBufLen = 0;
float stockPriceFlt;

extern const uint8_t server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t server_root_cert_pem_end[]   asm("_binary_server_root_cert_pem_end");


static void https_get_request()
{
    bool recording = false;
    stockBufLen = 0;
    int ret, len;

    esp_tls_cfg_t cfg = {
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    struct esp_tls *tls = esp_tls_conn_http_new(WEB_URL, &cfg);
    sleep(1);

    if (tls != NULL) {
        ESP_LOGI(TAG, "Connection established...");
    } else {
        ESP_LOGE(TAG, "Connection failed...");
        goto exit;
    }

    size_t written_bytes = 0;
    do {
        ret = esp_tls_conn_write(tls,
                                 REQUEST + written_bytes,
                                 sizeof(REQUEST) - written_bytes);
        if (ret >= 0) {
            ESP_LOGI(TAG, "%d bytes written", ret);
            written_bytes += ret;
        } else if (ret != ESP_TLS_ERR_SSL_WANT_READ  && ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
            ESP_LOGE(TAG, "esp_tls_conn_write  returned: [0x%02X](%s)", ret, esp_err_to_name(ret));
            goto exit;
        }
    } while (written_bytes < sizeof(REQUEST));

    ESP_LOGI(TAG, "Reading HTTP response...");

    do {
        len = sizeof(buf) - 1;
        bzero(buf, sizeof(buf));
        ret = esp_tls_conn_read(tls, (char *)buf, len);

        if (ret == ESP_TLS_ERR_SSL_WANT_WRITE  || ret == ESP_TLS_ERR_SSL_WANT_READ) {
            continue;
        }

        if (ret < 0) {
            ESP_LOGE(TAG, "esp_tls_conn_read  returned [-0x%02X](%s)", -ret, esp_err_to_name(ret));
            break;
        }

        if (ret == 0) {
            ESP_LOGI(TAG, "connection closed");
            break;
        }

        len = ret;
        ESP_LOGD(TAG, "%d bytes read", len);
        /* Print response directly to stdout as it is read */
        for (int i = 0; i < len; i++) {
            putchar(buf[i]);
            if (buf[i] == '{' && buf[i - 1] == '\n') {
                recording = true;
            } else if (buf[i] == '}') {
                stockResult[stockBufLen++] = buf[i];
                stockResult[stockBufLen] = '\0';
                recording = false;
            }

            if (recording) {
                stockResult[stockBufLen++] = buf[i];
            }
        }
        putchar('\n'); // JSON output doesn't have a newline at end
    } while (1);

exit:
    esp_tls_conn_delete(tls);
}

static bool https_request_task()
{
    bool result = false;
    ESP_LOGI(TAG, "Start https_request example");
    
    ESP_LOGI(TAG, "https_request using crt bundle");
    https_get_request();

    printf("got string: %s\n", stockResult);

    char *needle = "\"c\":";
    char *substr = strstr(stockResult, needle);
    if (substr) {
        printf("found price... ");
        uint16_t idx = substr - stockResult + strlen(needle);
        strncpy(stockPrice, &stockResult[idx], 5);
        stockPrice[5] = '\0';
        printf("%s\n", stockPrice);
        result = true;
    }

    ESP_LOGI(TAG, "Finish https_request");
    return result;
}

void http_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
}

#define BLACK_NEGATIVE  16 // bb U 8
#define WHITE_NEGATIVE  17 // bb U 9
#define BLUE_NEGATIVE    5 // bb U 10
#define PURPLE_NEGATIVE 18 // bb U 11
#define GREY_NEGATIVE   19 // bb U 12

#define NEGATIVE_PIN_LEN 5

#define BROWN_POSITIVE  13 // bb L 5
#define RED_POSITIVE    12 // bb L 7 
#define ORANGE_POSITIVE 14 // bb L 8
#define YELLOW_POSITIVE 27 // bb L 9
#define GREEN_POSITIVE  26 // bb L 10
#define BLUE_POSITIVE   25 // bb L 11
#define PURPLE_POSITIVE 33 // bb L 12
#define GRAY_POSITIVE   32 // bb L 13


#define WHITE_POSITIVE  15 // bb U 4
#define BLACK_POSITIVE   2 // bb U 5
#define GREEN2_POSITIVE  0 // bb U 6
#define YELLO2_POSITIVE  4 // bb U 7

#define ORANG2_POSITIVE 21 // bb U 14
#define RED2_POSITIVE    3 // bb U 15
#define BROWN2_POSITIVE 22 // bb U 17
#define BLACK2_POSITIVE 23 // bb U 18

// can't use pin 1 :(

// #define WHITE2_POSITIVE 22 // bb U 16
// #define YELLO3_POSITIVE 23 // bb U 17
// #define ORANG3_POSITIVE 23 // 

#define POSITIVE_PIN_LEN 16

#define BAR_DELAY 12

uint16_t yPins[NEGATIVE_PIN_LEN] = { 
    BLUE_NEGATIVE,
    PURPLE_NEGATIVE,
    GREY_NEGATIVE,
    WHITE_NEGATIVE, 
    BLACK_NEGATIVE,
};

uint16_t xPins[POSITIVE_PIN_LEN] = {
    BROWN_POSITIVE,
    RED_POSITIVE,
    ORANGE_POSITIVE,
    YELLOW_POSITIVE,
    GREEN_POSITIVE,
    BLUE_POSITIVE,
    PURPLE_POSITIVE,
    GRAY_POSITIVE,
    WHITE_POSITIVE,
    BLACK_POSITIVE,
    GREEN2_POSITIVE,
    YELLO2_POSITIVE,
    ORANG2_POSITIVE,
    RED2_POSITIVE ,
    BROWN2_POSITIVE,
    BLACK2_POSITIVE
};

#define xxxxx    true, true, true, true, true
#define xxxx_    true, true, true, true, false
#define __x_x    false, false, true, false, true
#define _x_x_    false, true, false, true, false
#define x_x_x    true, false, true, false, true
#define x___x    true, false, false, false, true
#define _xxx_    false, true, true, true, false
#define xxx__    true, true, true, false, false
#define xxx_x    true, true, true, false, true
#define __x__    false, false, true, false, false
#define xx___    true, true, false, false, false
#define x____    true, false, false, false, false
#define xx_xx    true, true, false, true, true
#define ___x_    false, false, false, true, false
#define _x___    false, true, false, false, false
#define __xxx    false, false, true, true, true
#define x_xxx    true, false, true, true, true
#define ____x    false, false, false, false, true
#define _xxxx    false, true, true, true, true
#define ___xx    false, false, false, true, true
#define _____    false, false, false, false, false
#define xx__x    true, true, false, false, true
#define x__xx    true, false, false, true, true
#define __xx_    false, false, true, true, false
#define _x__x    false, true, false, false, true
#define x__x_    true, false, false, true, false

bool pinStatuses[POSITIVE_PIN_LEN][NEGATIVE_PIN_LEN] = {
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____},
    {_____}
};

uint64_t pinArrayToMask(uint16_t pinArray[], uint16_t pinArrayLength) {
    uint64_t mask = 0;
    for (uint16_t i = 0; i < pinArrayLength; i++) {
        mask |= ((uint64_t)1 << pinArray[i]);
    }
    return mask;
}

void configurePins(uint64_t mask, uint8_t isPulldown, uint8_t isPullup) {
    gpio_config_t standardConf;
    standardConf.intr_type = GPIO_INTR_DISABLE;
    standardConf.mode = GPIO_MODE_OUTPUT;
    standardConf.pull_up_en = (gpio_pullup_t)isPullup;
    standardConf.pull_down_en = (gpio_pulldown_t)isPulldown;
    standardConf.pin_bit_mask = mask;
    gpio_config(&standardConf);
}

void pinSetup() {
    uint64_t pulldownPins = pinArrayToMask(yPins, NEGATIVE_PIN_LEN);
    uint64_t normalPins = pinArrayToMask(xPins, POSITIVE_PIN_LEN);

    configurePins(pulldownPins, 0, 0);
    configurePins(normalPins, 1, 0);
}

void setToInput(uint64_t mask) {
    gpio_config_t inputConfig;
    inputConfig.intr_type = GPIO_INTR_DISABLE;
    inputConfig.mode = GPIO_MODE_INPUT;
    inputConfig.pull_up_en = 0;
    inputConfig.pull_down_en = 0;
    inputConfig.pin_bit_mask = mask;
    gpio_config(&inputConfig);
}

void resetLEDs() {
    for (uint16_t i = 0; i < NEGATIVE_PIN_LEN; i++) {
        gpio_set_level((gpio_num_t)yPins[i], 0);
    }

    for (uint16_t i = 0; i < POSITIVE_PIN_LEN; i++) {
        gpio_set_level((gpio_num_t)xPins[i], 1);
    }
}

void turnOnLED(uint8_t x, uint8_t y) {
    uint16_t positivePin = xPins[x];
    uint16_t negativePin = yPins[y];

    gpio_set_level((gpio_num_t)negativePin, 1);
    gpio_set_level((gpio_num_t)positivePin, 0);
}

void turnOffLED(uint8_t x, uint8_t y) {
    uint16_t positivePin = xPins[x];
    uint16_t negativePin = yPins[y];

    gpio_set_level((gpio_num_t)negativePin, 0);
    gpio_set_level((gpio_num_t)positivePin, 1);
}

bool isEnabled(uint8_t x, uint8_t y) {
    return pinStatuses[x][y];
}

void enableLED(uint8_t x, uint8_t y) {
    pinStatuses[x][y] = true;
}

void disableLED(uint8_t x, uint8_t y) {
    pinStatuses[x][y] = false;
}

void dumpPins() {
    printf("dumping pins:\n");
    for (uint8_t i = 0; i < POSITIVE_PIN_LEN; i++) {
        for (uint8_t j = 0; j < NEGATIVE_PIN_LEN; j++) {
            if (isEnabled(i, j)) {
                printf("X");
            } else {
                printf("-");
            }
        }
        printf("\n");
    }
}

void addBar(bool y0, bool y1, bool y2, bool y3, bool y4) {
    for (uint8_t idx = POSITIVE_PIN_LEN - 1; idx > 0; idx--) {
        for (uint8_t yidx = 0; yidx < NEGATIVE_PIN_LEN; yidx++) {
            pinStatuses[idx][yidx] = pinStatuses[idx - 1][yidx];
        }
    }
    pinStatuses[0][0] = y0;
    pinStatuses[0][1] = y1;
    pinStatuses[0][2] = y2;
    pinStatuses[0][3] = y3;
    pinStatuses[0][4] = y4;
    vTaskDelay(BAR_DELAY);
}

void addLetterOptionalEndSpace(char c, bool endSpace) {

    printf("(%d) adding char: %c\n", c, c);
    switch(c) {
        case 'A':
            addBar(xxxx_);
            addBar(__x_x);
            addBar(xxxx_);
        break;
        case 'B':
            addBar(xxxxx);
            addBar(x_x_x);
            addBar(_x_x_);
        break;
        case 'C':
            addBar(xxxxx);
            addBar(x___x);
            addBar(x___x);
        break;
        case 'D':
            addBar(xxxxx);
            addBar(x___x);
            addBar(_xxx_);
        break;
        case 'E':
            addBar(xxxxx);
            addBar(x_x_x);
            addBar(x___x);
        break;
        case 'F':
            addBar(xxxxx);
            addBar(__x_x);
            addBar(__x_x);
        break;
        case 'G':
        case '6':
            addBar(xxxxx);
            addBar(x_x_x);
            addBar(xxx_x);
        break;
        case 'H':
            addBar(xxxxx);
            addBar(__x__);
            addBar(xxxxx);
        break;
        case 'I':
            addBar(x___x);
            addBar(xxxxx);
            addBar(x___x);
        break;
        case 'J':
            addBar(xx___);
            addBar(x____);
            addBar(xxxxx);
        break;
        case 'K':
            addBar(xxxxx);
            addBar(__x__);
            addBar(xx_xx);
        break;
        case 'L':
            addBar(xxxxx);
            addBar(x____);
            addBar(x____);
        break;
        case 'M':
            addBar(xxxxx);
            addBar(___x_);
            addBar(__x__);
            addBar(___x_);
            addBar(xxxxx);
        break;
        case 'N':
            addBar(xxxxx);
            addBar(___x_);
            addBar(__x__);
            addBar(xxxxx);
        break;
        case 'O':
        case '0':
            addBar(xxxxx);
            addBar(x___x);
            addBar(xxxxx);
        break;
        case 'P':
            addBar(xxxxx);
            addBar(__x_x);
            addBar(__xxx);
            
        break;
        case 'Q':
            addBar(xxxxx);
            addBar(x___x);
            addBar(xxxxx);
            addBar(x____);
        break;
        case 'R':
            addBar(xxxxx);
            addBar(__x_x);
            addBar(xx_xx);
        break;
        case 'S':   
        case '5':
            addBar(x_xxx);
            addBar(x_x_x);
            addBar(xxx_x);
        break;
        case 'T':
            addBar(____x);
            addBar(xxxxx);
            addBar(____x);
        break;
        case 'U':
            addBar(xxxxx);
            addBar(x____);
            addBar(xxxxx);
        break;
        case 'V':
            addBar(_xxxx);
            addBar(x____);
            addBar(_xxxx);
        break;
        case 'W':
            addBar(_xxxx);
            addBar(x____);
            addBar(_xxxx);
            addBar(x____);
            addBar(_xxxx);
        break;
        case 'X':
            addBar(xx_xx);
            addBar(__x__);
            addBar(xx_xx);
        break;
        case 'Y':
            addBar(___xx);
            addBar(xxx__);
            addBar(___xx);
        break;
        case 'Z':
            addBar(xx__x);
            addBar(x_x_x);
            addBar(x__xx);
        break;
        case ' ':
            addBar(_____);
        break;
        case '&':
            addBar(__xx_);
            addBar(_x__x);
            addBar(x__x_);
            addBar(_x__x);
            addBar(__xx_);
        break;
        case '1':
            addBar(_____);
            addBar(xxxxx);
        break;
        break;
        case '3':
            addBar(x___x);
            addBar(x_x_x);
            addBar(xxxxx);
        break;
        case '4':
            addBar(__xxx);
            addBar(__x__);
            addBar(xxxxx);
        break;
        case '7':
            addBar(____x);
            addBar(____x);
            addBar(xxxxx);
        break;
        case '8':
            addBar(xxxxx);
            addBar(x_x_x);
            addBar(xxxxx);
        break;
        case '9':
            addBar(__xxx);
            addBar(__x_x);
            addBar(xxxxx);
        break;
        case '.':
            addBar(x____);
        break;
        case '2':
            addBar(xxx_x);
            addBar(x_x_x);
            addBar(x_xxx);
        break;
        case 'u':
            addBar(_xxx_);
            addBar(__xxx);
            addBar(_xxx_);
        break;
        case 'd':
            addBar(__xxx);
            addBar(_xxx_);
            addBar(__xxx);
        break;
    }

    if (endSpace) {
        addBar(_____);
    }
}

void addLetter(char c) {
    addLetterOptionalEndSpace(c, true);
}

void addWordOptionalEndSpace(char* word, bool endSpace) {
    uint16_t len = strlen(word);
    for (uint16_t i = 0; i < len; i++) {
        char c = word[i];
        if (i == (len - 1)) {
            addLetterOptionalEndSpace(c, false);
        } else {
            addLetter(c);
        }
    }
    if (endSpace) {
       addLetter(' ');
    }
}

void addWord(char* word) {
    addWordOptionalEndSpace(word, true);
}

static void mainTask(void *pvParameters) {
    while(1) {
        for (uint8_t i = 0; i < POSITIVE_PIN_LEN; i++) {
            for (uint8_t j = 0; j < NEGATIVE_PIN_LEN; j++) {
                resetLEDs();
                if (isEnabled(i, j)) {
                    turnOnLED(i, j);
                } else {
                    turnOffLED(i, j);
                }
                usleep(5);
            }
        }
    }
}

static void secondTask(void *pvParameters) {
    while(1) {
        if (https_request_task()) {
            float curPrice = atof(stockPrice);
            for (uint8_t i = 0; i < 3; i++) {
                addWord("    ");
                addWord(STOCK_SYMBOL);
                if (curPrice > stockPriceFlt) {
                    addLetter('u');
                } else if (stockPriceFlt < curPrice) {
                    addLetter('d');
                }
                addWord(stockPrice);
                if (curPrice > stockPriceFlt) {
                    addLetter('u');
                } else if (stockPriceFlt < curPrice) {
                    addLetter('d');
                }
            }
            addWord("    ");
            addWord(STOCK_SYMBOL);
            addWordOptionalEndSpace(stockPrice, false);
            stockPriceFlt = curPrice;
        }
        sleep(1);
    }
}


void app_main(void)
{
    printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
    pinSetup();
    http_init();

    xTaskCreate(&mainTask, "main_task", 4096, NULL, 5, NULL);
    xTaskCreate(&secondTask, "second_task", 8192, NULL, 5, NULL);
}
