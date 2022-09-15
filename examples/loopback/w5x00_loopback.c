/**
 * Copyright (c) 2021 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <string.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "w5x00_spi.h"
#include "ILI9340/ILI9340.h"

#include "loopback.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_LOOPBACK 0

/* Port */
#define PORT_LOOPBACK 5000

/* LCD */
#define LCD_PRINT_TEXT_MAX              20
#define LCD_PRINT_DATA_MEM_ARR_SIZE     15
#define LCD_PRINT_DATA_MEM_CNT_MAX      LCD_PRINT_DATA_MEM_ARR_SIZE-1 //0~15
#define LCD_PRINT_DATA_LINE_MAX         LCD_PRINT_DATA_MEM_ARR_SIZE   //1~16


/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 2},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};

/* Loopback */
static uint8_t g_loopback_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};

/* LCD */
int8_t g_lcd_print_data_mem[LCD_PRINT_DATA_MEM_ARR_SIZE][LCD_PRINT_TEXT_MAX];

int8_t g_lcd_newest_mem_index= 0;
int8_t g_lcd_current_printed_line= 1;


/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void);

/* LCD */
void set_lcd_print_info(int8_t index, int8_t* text);
void set_lcd_print_log(void);


/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main()
{
    /* Initialize */
    int retval = 0;
    int loopFlag= 0;
    set_clock_khz();

    stdio_init_all();

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    network_initialize(g_net_info);
    /* Get network information */
    print_network_information(g_net_info);

    ILI9340_init();
    /* Infinite loop */
    while (1)
    {
        /* TCP server loopback test */
        if ((retval = loopback_tcps(SOCKET_LOOPBACK, g_loopback_buf, PORT_LOOPBACK, &loopFlag)) < 0)
        {
            printf(" Loopback error : %d\n", retval);

            while (1)
                ;
        }
        if (loopFlag)
        {
            printf("\r\n ret:%d, data:%s", retval, g_loopback_buf, loopFlag);
            set_lcd_print_info(g_lcd_newest_mem_index, g_loopback_buf);
            set_lcd_print_log();

            memset(g_loopback_buf,0x00,2048);
            loopFlag= 0;
        }

    }
}

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

void set_lcd_print_info(int8_t index, int8_t* text)
{
    memset(g_lcd_print_data_mem[index], 0x00, LCD_PRINT_TEXT_MAX);
    memcpy(g_lcd_print_data_mem[index], text, LCD_PRINT_TEXT_MAX);
}

void set_lcd_print_log(void)
{
    int8_t start_mem_cnt;
    
    if (g_lcd_newest_mem_index == LCD_PRINT_DATA_MEM_CNT_MAX|| g_lcd_current_printed_line < LCD_PRINT_DATA_LINE_MAX )
        start_mem_cnt=0;
    else
        start_mem_cnt= g_lcd_newest_mem_index+1;
    
    for(int8_t i=0; g_lcd_current_printed_line > i; i++)
    { 
        ILI9340_Write_Scroll(i, g_lcd_print_data_mem[i], strlen(g_lcd_print_data_mem[i]));

        if (start_mem_cnt == LCD_PRINT_DATA_MEM_CNT_MAX)
            start_mem_cnt=0;
        else 
            start_mem_cnt++;
    }

    g_lcd_newest_mem_index= start_mem_cnt;
    
    if(g_lcd_current_printed_line < LCD_PRINT_DATA_LINE_MAX )
        g_lcd_current_printed_line++;
}