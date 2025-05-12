#include "platform.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "queue.h"
#include "leds.h"
#include "gpio.h"
#include "timer.h"

#define BUFF_SIZE 128
#define BUTTON_PIN 13

Queue rx_queue;

volatile int flag = 0;
volatile int time_flag = 0;
volatile int time_flag_200 = 0;
volatile int time_flag_500 = 0;
volatile int stop = 0;
volatile int count = 0;
volatile int button = 0;
volatile int led_locked = 0;

// Simulated GPIO (replace with actual GPIO read/write in your platform)
void setLedOn() {
    if (!button) {
        flag = 1;
        leds_set(1, 0, 0);
        uart_print("LED is ON!\r\n");
    } else {
        uart_print("Tried to turn the LED ON, but it is locked!\r\n");
    }
}

void setLedOff() {
    if (!button) {
        flag = 0;
        leds_set(0, 0, 0);
        uart_print("LED is OFF!\r\n");
    } else {
        uart_print("Tried to turn the LED OFF, but it is locked!\r\n");
    }
}

void ledBlinker(int number, int current_flag) {
		if (led_locked) {
			uart_print("Interrupt: Button pressed. LED lock toggled.\r\n");
			return;
		}
    if (number % 2 == 0) {
        setLedOn();
        while (!time_flag_200) {
            __WFI();
        }
        time_flag_200 = 0;
        setLedOff();
        while (!time_flag_200) {
            __WFI();
        }
        time_flag_200 = 0;
    } else {
        if (current_flag == 1)
            setLedOff();
        else
            setLedOn();
    }
}

// UART interrupt handler
void uart_rx_isr(uint8_t rx) {
    if (rx >= 0x00 && rx <= 0x7F) {
        queue_enqueue(&rx_queue, rx);
    }
}

// Simulated button read
int read_button() {
    return !gpio_get(P_SW);
}

void button_interrupt() {
    if (gpio_get(PC_13)) {
        count++;
        //uart_print("Interrupt: Button pressed. LED lock toggled.\r\n");
				led_locked = !led_locked; // Freeze LED state
        char msg[64];
        sprintf(msg, "Interrupt: Button pressed %d times.\r\n", count);
        uart_print(msg);
    }
}

void analysis() {
    time_flag++;

    if (time_flag % 2 == 0) { // e.g., 200ms if timer runs at 10ms
        time_flag_200 = 1;
    }
    if (time_flag % 5 == 0) { // 500ms
        time_flag_500 = 1;
    }

    if (time_flag >= 1000) {
        time_flag = 0;
    }
}

int main() {
    uint8_t rx_char = 0;
    char buff[BUFF_SIZE];
    uint32_t buff_index;

    queue_init(&rx_queue, 128);
    uart_init(115200);
    uart_set_rx_callback(uart_rx_isr);
    uart_enable();
    __enable_irq();
	
		gpio_set_mode(PC_13, Input);
    gpio_set_mode(PC_13, PullUp);
    gpio_set_trigger(PC_13, Rising);
    gpio_set_callback(PC_13, button_interrupt);
	
    gpio_set_mode(P_LED_R, Output);
	
		timer_init(CLK_FREQ / 100);
    timer_set_callback(analysis);
   
    uart_print("\r\n");

    while (1) {
        uart_print("Give me a number:\r\n");
        buff_index = 0;
        memset(buff, 0, BUFF_SIZE);

        // Input loop
        do {
            while (!queue_dequeue(&rx_queue, &rx_char))
                __WFI();

            if (rx_char == 0x7F && buff_index > 0) {
                buff_index--;
                uart_tx(rx_char);
            } else if (rx_char != '\r') {
                buff[buff_index++] = (char)rx_char;
                uart_tx(rx_char);
            }
        } while (rx_char != '\r' && buff_index < BUFF_SIZE);
				
				
        uart_print("\r\n");
				timer_enable();
        if (buff_index >= BUFF_SIZE) {
            uart_print("Stop trying to overflow my buffer! I resent that!\r\n");
            continue;
        }

        buff[buff_index] = '\0';

        // Handle special case where last char is '-'
        int special_case = 0;
        if (buff_index > 0 && buff[buff_index - 1] == '-') {
            special_case = 1;
            buff[buff_index - 1] = '\0'; // Remove the '-'
        }

        int result = 0;
        for (int i = 0; buff[i] != '\0'; i++) {
            if (buff[i] >= '0' && buff[i] <= '9') {
                result = result * 10 + (buff[i] - '0');
                ledBlinker(buff[i] - '0', flag);
                while (!time_flag_500) {
                    __WFI();
                }
                time_flag_500 = 0;
								time_flag_200 = 0;
            }

            if (stop) {
                break;
            }
        }
				timer_disable();
    }
}
