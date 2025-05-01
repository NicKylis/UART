#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int flag = 0;   // 0 = LED OFF, 1 = LED ON
int stop = 0;   // Should activate when a new number is typed
int count = 0;  // Counter for the number of times the button is pressed
int button = 0; // 0 = not pressed, 1 = pressed

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
    char BUFFER_INIT[10]; // Buffer for user input

    while (1) {
        printf("Give me a number: ");
        if (fgets(BUFFER_INIT, sizeof(BUFFER_INIT), stdin) == NULL) {
            printf("Invalid input! Try again.\n");
            continue;
        }
    
        int result = 0;  // To store the accumulated number
        int special_case = 0;  // Flag for string ending with '-'
        int len = sizeof(BUFFER_INIT);
        
        // Check if string ends with '-' (excluding newline)
        if (len > 1 && BUFFER_INIT[len-1] == '\n' && BUFFER_INIT[len-2] == '-') {
            special_case = 1;
            BUFFER_INIT[len-2] = '\0';  // Remove the '-' from processing
        } else if (BUFFER_INIT[len-1] == '-') {
            special_case = 1;
            BUFFER_INIT[len-1] = '\0';
        }
    
        for (int i = 0; BUFFER_INIT[i] != '\0' && BUFFER_INIT[i] != '\n'; i++) {
        
            // Custom condition for ending in '-'
            if (special_case);
            else {
                if(button == 1){
                    button = 0;
                    count++;
                    printf("Interrupt: Button pressed. LED locked. Count = %d\n", count);
                    while(1){
                        if(button == 1) break;
                    }
                }
                if (stop == 1) {
                        break;
                    }
            }    
                // Only process numeric characters
                if (BUFFER_INIT[i] >= '0' && BUFFER_INIT[i] <= '9') {
                    result = result * 10 + (BUFFER_INIT[i] - '0');
                    ledBlinker(BUFFER_INIT[i] - '0', flag); // Blink for each digit
                    usleep(500000);                    // 500ms
                }
            }
    }
}