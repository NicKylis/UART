#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int flag = 0;
int stop = 0;

void setLedOn() {
    flag = 1;
    printf("LED is ON!\n");
}

void setLedOff() {
    flag = 0;
    printf("LED is OFF!\n");
}

void ledBlinker(int number, int flag) {
    if (number % 2 == 0) {
        setLedOn();
        usleep(200000); // 200ms
        setLedOff();
        usleep(200000);
    } else {
        if (flag == 1) {
            setLedOff();
        } else {
            setLedOn();
        }
    }
}

void main() {
    char number[10]; // Buffer for user input

    while (1) {
        printf("Give me a number: ");
        if (fgets(number, sizeof(number), stdin) == NULL) {
            printf("Invalid input! Try again.\n");
            continue;
        }

        for (int i = 0; number[i] != '\0' && number[i] != '\n'; i++) {
            if (stop == 1) {
                break;
            }
            ledBlinker(number[i] - '0', flag); // Cast char to int
            usleep(500000);                    // 500ms
        }
    }
}
