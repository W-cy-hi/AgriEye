/**
 ****************************************************************************************************
 * @file        main.c
 * @brief       调度器 + 串口1收发
 *              接收ESP32天气数据 → 采集DHT22/BH1750/TDS传感器 → 发送给ESP32
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
#include "./BSP/KEY/key.h"
#include "./BSP/DHT22/dht22.h"
#include "./BSP/BH1750/bh1750.h"
#include "./BSP/TDS/tds.h"
#include <string.h>
#include <stdio.h>

uint8_t current_screen = 0;

char g_weather_text[32] = "Waiting";
char g_temperature[8] = "--";
char g_humidity[8] = "--";
char g_temp_dht[8] = "--";
char g_light[8] = "--";
char g_soil[8] = "--";

uint8_t dht22_ok = 0;
uint8_t bh1750_ok = 0;

static float last_temp = 25.0;
static float last_humi = 50.0;

void display_screen_weather(void);
void display_screen_temp_hum(void);
void display_screen_light_soil(void);
void refresh_current_screen(void);
void parse_weather_data(char *data);
void read_and_send_sensors(void);

void display_screen_weather(void)
{
    char buf[64];
    lcd_clear(WHITE);
    lcd_show_string(30, 10, 200, 24, 24, "Weather Station", RED);
    lcd_draw_line(0, 40, lcddev.width, 40, BLACK);
    lcd_show_string(30, 60, 200, 16, 16, "Weather:", BLUE);
    lcd_show_string(30, 100, 200, 16, 16, "Temp:", BLUE);
    lcd_show_string(100, 60, 200, 16, 16, g_weather_text, RED);
    sprintf(buf, "%s C", g_temperature);
    lcd_show_string(100, 100, 200, 16, 16, buf, RED);
    lcd_show_string(30, 150, 200, 16, 16, "Status: Receiving...", BLUE);
    lcd_show_string(30, 200, 200, 16, 16, "KEY0: Next Screen", GREEN);
    lcd_show_string(30, 220, 200, 16, 16, "Page 1/3", BLACK);
}

void display_screen_temp_hum(void)
{
    char buf[64];
    lcd_clear(WHITE);
    lcd_show_string(30, 10, 200, 24, 24, "Temp & Humidity", RED);
    lcd_draw_line(0, 40, lcddev.width, 40, BLACK);
    lcd_show_string(30, 60, 200, 16, 16, "Humidity:", BLUE);
    lcd_show_string(30, 100, 200, 16, 16, "Temperature:", BLUE);
    sprintf(buf, "%s %%", g_humidity);
    lcd_show_string(140, 60, 200, 16, 16, buf, RED);
    sprintf(buf, "%s C", g_temp_dht);
    lcd_show_string(160, 100, 200, 16, 16, buf, RED);
    if (dht22_ok)
        lcd_show_string(30, 140, 200, 16, 16, "DHT22: OK", GREEN);
    else
        lcd_show_string(30, 140, 200, 16, 16, "DHT22: Error", RED);
    lcd_show_string(30, 180, 200, 16, 16, "KEY0: Next Screen", GREEN);
    lcd_show_string(30, 200, 200, 16, 16, "Page 2/3", BLACK);
}

void display_screen_light_soil(void)
{
    char buf[64];
    lcd_clear(WHITE);
    lcd_show_string(30, 10, 200, 24, 24, "Light & Soil", RED);
    lcd_draw_line(0, 40, lcddev.width, 40, BLACK);
    lcd_show_string(30, 60, 200, 16, 16, "Light:", BLUE);
    lcd_show_string(30, 100, 200, 16, 16, "Soil Fertility:", BLUE);
    sprintf(buf, "%s Lux", g_light);
    lcd_show_string(120, 60, 200, 16, 16, buf, RED);
    sprintf(buf, "%s ppm", g_soil);
    lcd_show_string(180, 100, 200, 16, 16, buf, RED);
    if (bh1750_ok)
        lcd_show_string(30, 140, 200, 16, 16, "BH1750: OK", GREEN);
    else
        lcd_show_string(30, 140, 200, 16, 16, "BH1750: Error", RED);
    lcd_show_string(30, 180, 200, 16, 16, "KEY0: Next Screen", GREEN);
    lcd_show_string(30, 200, 200, 16, 16, "Page 3/3", BLACK);
}

void refresh_current_screen(void)
{
    switch (current_screen) {
        case 0: display_screen_weather(); break;
        case 1: display_screen_temp_hum(); break;
        case 2: display_screen_light_soil(); break;
        default: current_screen = 0; display_screen_weather(); break;
    }
}

void parse_weather_data(char *data)
{
    char *comma = strchr(data, ',');
    if (comma != NULL) {
        int len = comma - data;
        if (len > 31) len = 31;
        strncpy(g_weather_text, data, len);
        g_weather_text[len] = '\0';
        char *temp_str = comma + 1;
        while (*temp_str == ' ') temp_str++;
        strcpy(g_temperature, temp_str);
    } else {
        strcpy(g_weather_text, "Error");
        strcpy(g_temperature, "?");
    }
    if (current_screen == 0) display_screen_weather();
}

void read_and_send_sensors(void)
{
    float temp_val, humi_val;
    uint16_t light_val, soil_val;

    if (DHT22_Read_Data(&temp_val, &humi_val) == 0) {
        dht22_ok = 1;
        last_temp = temp_val;
        last_humi = humi_val;
    } else {
        dht22_ok = 0;
        temp_val = last_temp;
        humi_val = last_humi;
    }

    light_val = BH1750_Read_Light();
    bh1750_ok = (light_val > 0 && light_val < 65535) ? 1 : 0;

    soil_val = TDS_Read_Value();

    sprintf(g_temp_dht, "%.1f", temp_val);
    sprintf(g_humidity, "%.1f", humi_val);
    sprintf(g_light, "%d", light_val);
    sprintf(g_soil, "%d", soil_val);

    printf("DATA,TEMP,%.1f,HUMI,%.1f,LIGHT,%d,SOIL,%d\r\n",
           temp_val, humi_val, light_val, soil_val);
}

int main(void)
{
    uint8_t key;

    sys_stm32_clock_init(336, 8, 2, 7);
    delay_init(168);
    usart_init(84, 115200);

    led_init();
    lcd_init();
    key_init();

    DHT22_Init();
    delay_ms(2000);

    BH1750_Init();
    delay_ms(100);
    BH1750_Send_Cmd(BH1750_POWER_ON);
    delay_ms(10);
    BH1750_Send_Cmd(BH1750_RESET);
    delay_ms(10);
    BH1750_Send_Cmd(BH1750_CONT_H_MODE);
    delay_ms(200);

    TDS_Adc_Init();

    display_screen_weather();
    lcd_show_string(30, 240, 200, 16, 16, "UART1 Ready", GREEN);

    while (1)
    {
        key = key_scan(0);
        if (key == KEY0_PRES) {
            current_screen = (current_screen + 1) % 3;
            refresh_current_screen();
        }

        static uint16_t sensor_cnt = 0;
        if (++sensor_cnt >= 200) {
            sensor_cnt = 0;
            read_and_send_sensors();
            if (current_screen == 1) display_screen_temp_hum();
            else if (current_screen == 2) display_screen_light_soil();
        }

        if (g_usart_rx_sta & 0x8000) {
            uint16_t len = g_usart_rx_sta & 0x3FFF;
            if (len > 0) parse_weather_data((char *)g_usart_rx_buf);
            g_usart_rx_sta = 0;
        }

        static uint16_t led_cnt = 0;
        if (++led_cnt >= 200) { led_cnt = 0; LED0_TOGGLE(); }

        delay_ms(10);
    }
}
