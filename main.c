#include "platform.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "uart.h"
#include "queue.h"

#define BUFF_SIZE 128

Queue rx_queue;

volatile int flag = 0;
volatile int stop = 0;
volatile int count = 0;
volatile int button = 0;

// Simulated GPIO (replace with actual GPIO read/write in your platform)
void setLedOn() {
    flag = 1;
    gpio_write_led(1);
    uart_print("LED is ON!\r\n");
}

void setLedOff() {
    flag = 0;
    gpio_write_led(0);
    uart_print("LED is OFF!\r\n");
}

void ledBlinker(int number, int flag) {
    if (number % 2 == 0) {
        setLedOn();
        platform_delay(200); // 200 ms
        setLedOff();
        platform_delay(200);
    } else {
        if (flag == 1)
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
    return gpio_read_button(); // Platform-specific
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
                platform_delay(500);
            }

            // Simulate button press
            if (read_button()) {
                button = 1;
                count++;
                uart_print("Interrupt: Button pressed. LED locked.\r\n");

                // Wait for button release to unlock
                while (read_button());
            }

            if (stop) {
                break;
            }
        }
    }
}
