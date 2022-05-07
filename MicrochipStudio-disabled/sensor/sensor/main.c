#include <atmel_start.h>

#include "app.h"

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	
	printf("Welcome Sensor Application\n");
	
	APP_Start();

	/* Replace with your application code */
	while (1) {
	}
}
