//------------------------------------------------------------------------------
/**
 * @file main.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief ODROID-N2 Lite JIG client application.
 * @version 0.1
 * @date 2022-05-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

//------------------------------------------------------------------------------
// for my lib
//------------------------------------------------------------------------------
/* 많이 사용되는 define 정의 모음 */
#include "typedefs.h"

/* framebuffer를 control하는 함수 */
#include "lib_fb.h"

/* file parser control 함수 */
#include "lib_ui.h"

/* uart control 함수 */
#include "lib_uart.h"

#if 0

/* jig용으로 만들어진 adc board control 함수 */
#include "lib_adc.h"

/* i2c control 함수 */
#include "lib_i2c.h"

/* network label printer control 함수 */
#include "lib_nlp.h"

/* mac server control 함수 */
#include "lib_mac.h"

#endif

#include "client.h"
//------------------------------------------------------------------------------
// Default global value
//------------------------------------------------------------------------------
const char	*OPT_UI_CFG_FILE		= "default_ui_client.cfg";
const char	*OPT_SYSTEM_CFG_FILE 	= "default_client.cfg";

//------------------------------------------------------------------------------
// function prototype define
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void tolowerstr (char *p)
{
	while (*p++)   *p = tolower(*p);
}

//------------------------------------------------------------------------------
static void toupperstr (char *p)
{
	while (*p++)   *p = toupper(*p);
}

//------------------------------------------------------------------------------
static void print_usage(const char *prog)
{
	printf("Usage: %s [-fu]\n", prog);
	puts("  -f --client_cfg_file    default default_client.cfg.\n"
		 "  -u --ui_cfg_file        default file name is default_ui_client.cfg\n"
	);
	exit(1);
}

//------------------------------------------------------------------------------
static void parse_opts (int argc, char *argv[])
{
	while (1) {
		static const struct option lopts[] = {
			{ "client_config_file"		, 1, 0, 'f' },
			{ "client_ui_config_file"	, 1, 0, 'u' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "f:u:", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
		case 'f':
			OPT_SYSTEM_CFG_FILE = optarg;
			break;
		case 'u':
			OPT_UI_CFG_FILE = optarg;
			break;
		default:
			print_usage(argv[0]);
			break;
		}
	}
}

//------------------------------------------------------------------------------
char *_str_remove_space (char *ptr)
{
	/* 문자열이 없거나 앞부분의 공백이 있는 경우 제거 */
	int slen = strlen(ptr);

	while ((*ptr == 0x20) && slen--)
		ptr++;

	return ptr;
}

//------------------------------------------------------------------------------
void _strtok_strcpy (char *dst)
{
	char *ptr;

	if ((ptr = strtok (NULL, ",")) != NULL) {
		ptr = _str_remove_space(ptr);
		strncpy(dst, ptr, strlen(ptr));
	}
}

//------------------------------------------------------------------------------
bool parse_cfg_file (char *cfg_filename, jig_client_t *pclient)
{
	FILE *pfd;
	char buf[256], *ptr, is_cfg_file = 0;

	if ((pfd = fopen(cfg_filename, "r")) == NULL) {
		err ("%s file open fail!\n", cfg_filename);
		return false;
	}

	/* config file에서 1 라인 읽어올 buffer 초기화 */
	memset (buf, 0, sizeof(buf));
	while(fgets(buf, sizeof(buf), pfd) != NULL) {
		/* config file signature 확인 */
		if (!is_cfg_file) {
			is_cfg_file = strncmp ("ODROID-JIG-CLIENT-CONFIG", buf, strlen(buf)-1) == 0 ? 1 : 0;
			memset (buf, 0x00, sizeof(buf));
			continue;
		}

		ptr = strtok (buf, ",");
		if (!strncmp(ptr, "MODEL", strlen("MODEL")))	_strtok_strcpy(pclient->model);
		if (!strncmp(ptr,    "FB", strlen("FB")))		_strtok_strcpy(pclient->fb_dev);
		if (!strncmp(ptr,  "UART", strlen("UART")))		_strtok_strcpy(pclient->uart_dev);
		memset (buf, 0x00, sizeof(buf));
	}

	if (pfd)
		fclose (pfd);

	if (!is_cfg_file) {
		err("This file is not JIG Config File! (filename = %s)\n", cfg_filename);
		return false;
	}
	return true;
}

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	jig_client_t	*pclient;

    parse_opts(argc, argv);

	if ((pclient = (jig_client_t *)malloc(sizeof(jig_client_t))) == NULL) {
		err ("create client fail!\n");
		goto err_out;
	}
	memset  (pclient, 0, sizeof(jig_client_t));

	info("JIG Config file : %s\n", OPT_SYSTEM_CFG_FILE);
	if (!parse_cfg_file ((char *)OPT_SYSTEM_CFG_FILE, pclient)) {
		err ("client init fail!\n");
		goto err_out;
	}
	strncpy (pclient->bdate, __DATE__, strlen(__DATE__));
	strncpy (pclient->btime, __TIME__, strlen(__TIME__));
	info ("Application Build : %s / %s\n", pclient->bdate, pclient->btime);

	info("Framebuffer Device : %s\n", pclient->fb_dev);
	if ((pclient->pfb = fb_init (pclient->fb_dev)) == NULL) {
		err ("create framebuffer fail!\n");
		goto err_out;
	}

	info("UART Device : %s, baud = 115200bps(%d)\n", pclient->uart_dev, B115200);
	if ((pclient->puart = uart_init (pclient->uart_dev, B115200)) == NULL) {
		err ("create uart fail!\n");
		goto err_out;
	}

	info("UI Config file : %s\n", OPT_UI_CFG_FILE);
	if ((pclient->pui = ui_init (pclient->pfb, OPT_UI_CFG_FILE)) == NULL) {
		err ("create ui fail!\n");
		goto err_out;
	}

	// main control function (server.c)
	client_main (pclient);

err_out:
	uart_close (pclient->puart);
	ui_close   (pclient->pui);
	fb_clear   (pclient->pfb);
	fb_close   (pclient->pfb);

	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
