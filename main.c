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
volatile int new_input_flag = 0;
volatile int button_interrupt_flag = 0;
volatile int repeat_mode = 0;
char repeat_buff[BUFF_SIZE];

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
				new_input_flag = 1;
    }
}

void button_interrupt() {
    if (gpio_get(PC_13)) {
        count++;
				led_locked = !led_locked; // Freeze LED state
        char msg[64];
        sprintf(msg, "Interrupt: Button pressed %d times.\r\n", count);
        uart_print(msg);
				button_interrupt_flag = 1;
    }
}

void analysis() {
    time_flag++;

    if (time_flag % 2 == 0) { // 200ms if timer runs at 10ms
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
        new_input_flag = 0;  // reset at start

        // Input loop
        do {
            while (!queue_dequeue(&rx_queue, &rx_char))
                __WFI();

            if (rx_char == 0x7F && buff_index > 0) {
                buff_index--;
                uart_tx(rx_char);
            } else if ((rx_char >= '0' && rx_char <= '9') || rx_char == '-') {
                buff[buff_index++] = (char)rx_char;
                uart_tx(rx_char);
            }

            new_input_flag = 0;  // reset once consumed
        } while (rx_char != '\r' && buff_index < BUFF_SIZE);

        uart_print("\r\n");

        if (buff_index >= BUFF_SIZE) {
            uart_print("Stop trying to overflow my buffer! I resent that!\r\n");
            continue;
        }

        buff[buff_index] = '\0';

        repeat_mode = 0;
        if (buff_index > 0 && buff[buff_index - 1] == '-') {
            repeat_mode = 1;
            buff[buff_index - 1] = '\0';
            strcpy(repeat_buff, buff);
        }

        timer_enable();

        do {
            const char* input = repeat_mode ? repeat_buff : buff;

            for (int i = 0; input[i] != '\0'; i++) {
                // Button interrupt has priority
                if (button_interrupt_flag) {
                    button_interrupt_flag = 0;
                    uart_print("Button interrupt occurred - priority handling.\r\n");
                }

                // Check for new UART input mid-analysis
                if (new_input_flag) {
                    new_input_flag = 0;
                    repeat_mode = 0;
                    goto restart_loop;
                }

                if (input[i] >= '0' && input[i] <= '9') {
                    ledBlinker(input[i] - '0', flag);
                    while (!time_flag_500) {
                        if (new_input_flag) break;
                        __WFI();
                    }
                    time_flag_500 = 0;
                    time_flag_200 = 0;
                }

                if (new_input_flag)
                    break;
            }

            if (!repeat_mode || new_input_flag)
                break;

        } while (repeat_mode);

        timer_disable();
        continue;

    restart_loop:
        timer_disable();
        continue;
    }

}
