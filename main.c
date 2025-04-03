#include <stdio.h>
#include <time.h>

char *str = "Hello, World!";
void setLedOn(){
    printf("LED is ON! \n");
};
void ledBlinker(int number, int flag){
    if(number % 2 == 0){
        setLedOn();
        sleep(0.2);
    }
    else{
        setLedOn();
    }
}

void main(){
    int flag = 0;
    while(1){
        
        sleep(0.5);
    };
}