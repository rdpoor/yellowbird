/*
 * Code generated from Atmel Start.
 *
 * This file will be overwritten when reconfiguring your Atmel Start project.
 * Please copy examples or other code you want to keep to a separate file
 * to avoid losing it when reconfiguring.
 */

#include "driver_init.h"
#include <peripheral_clk_config.h>
#include <utils.h>
#include <hal_init.h>

struct spi_m_sync_descriptor SPI_INSTANCE;

struct calendar_descriptor CALENDAR_0;

struct usart_sync_descriptor TARGET_IO;

void EXTIRQ_INSTANCE_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, EIC_GCLK_ID, CONF_GCLK_EIC_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_mclk_set_APBAMASK_EIC_bit(MCLK);

	// Set pin direction to input
	gpio_set_pin_direction(PB07, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(PB07,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PB07, PINMUX_PB07A_EIC_EXTINT7);

	ext_irq_init();
}

void CALENDAR_0_CLOCK_init(void)
{
	hri_mclk_set_APBAMASK_RTC_bit(MCLK);
}

void CALENDAR_0_init(void)
{
	CALENDAR_0_CLOCK_init();
	calendar_init(&CALENDAR_0, RTC);
}

void TARGET_IO_PORT_init(void)
{

	gpio_set_pin_function(PB25, PINMUX_PB25D_SERCOM2_PAD0);

	gpio_set_pin_function(PB24, PINMUX_PB24D_SERCOM2_PAD1);
}

void TARGET_IO_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_CORE, CONF_GCLK_SERCOM2_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM2_GCLK_ID_SLOW, CONF_GCLK_SERCOM2_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBBMASK_SERCOM2_bit(MCLK);
}

void TARGET_IO_init(void)
{
	TARGET_IO_CLOCK_init();
	usart_sync_init(&TARGET_IO, SERCOM2, (void *)NULL);
	TARGET_IO_PORT_init();
}

void SPI_INSTANCE_PORT_init(void)
{

	gpio_set_pin_level(PB27,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(PB27, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(PB27, PINMUX_PB27D_SERCOM4_PAD0);

	gpio_set_pin_level(PB26,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(PB26, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(PB26, PINMUX_PB26D_SERCOM4_PAD1);

	// Set pin direction to input
	gpio_set_pin_direction(PB29, GPIO_DIRECTION_IN);

	gpio_set_pin_pull_mode(PB29,
	                       // <y> Pull configuration
	                       // <id> pad_pull_config
	                       // <GPIO_PULL_OFF"> Off
	                       // <GPIO_PULL_UP"> Pull-up
	                       // <GPIO_PULL_DOWN"> Pull-down
	                       GPIO_PULL_OFF);

	gpio_set_pin_function(PB29, PINMUX_PB29D_SERCOM4_PAD3);
}

void SPI_INSTANCE_CLOCK_init(void)
{
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM4_GCLK_ID_CORE, CONF_GCLK_SERCOM4_CORE_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));
	hri_gclk_write_PCHCTRL_reg(GCLK, SERCOM4_GCLK_ID_SLOW, CONF_GCLK_SERCOM4_SLOW_SRC | (1 << GCLK_PCHCTRL_CHEN_Pos));

	hri_mclk_set_APBDMASK_SERCOM4_bit(MCLK);
}

void SPI_INSTANCE_init(void)
{
	SPI_INSTANCE_CLOCK_init();
	spi_m_sync_init(&SPI_INSTANCE, SERCOM4);
	SPI_INSTANCE_PORT_init();
}

void delay_driver_init(void)
{
	delay_init(SysTick);
}

void system_init(void)
{
	init_mcu();

	// GPIO on PA06

	gpio_set_pin_level(RESET_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(RESET_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(RESET_PIN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PA27

	gpio_set_pin_level(CE_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   false);

	// Set pin direction to output
	gpio_set_pin_direction(CE_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(CE_PIN, GPIO_PIN_FUNCTION_OFF);

	// GPIO on PB28

	gpio_set_pin_level(CS_PIN,
	                   // <y> Initial level
	                   // <id> pad_initial_level
	                   // <false"> Low
	                   // <true"> High
	                   true);

	// Set pin direction to output
	gpio_set_pin_direction(CS_PIN, GPIO_DIRECTION_OUT);

	gpio_set_pin_function(CS_PIN, GPIO_PIN_FUNCTION_OFF);

	EXTIRQ_INSTANCE_init();

	CALENDAR_0_init();

	TARGET_IO_init();

	SPI_INSTANCE_init();

	delay_driver_init();
}
