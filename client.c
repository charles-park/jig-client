//------------------------------------------------------------------------------
/**
 * @file client.c
 * @author charles-park (charles.park@hardkernel.com)
 * @brief server main control.
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
#include <sys/time.h>
#include <getopt.h>

#define	SYSTEM_LOOP_DELAY_uS	100
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

#include "client.h"
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

//------------------------------------------------------------------------------
bool run_interval_check (struct timeval *t, double interval_ms)
{
	struct timeval base_time;
	double difftime;

	gettimeofday(&base_time, NULL);

	if (interval_ms) {
		/* 현재 시간이 interval시간보다 크면 양수가 나옴 */
		difftime = (base_time.tv_sec - t->tv_sec) +
					((base_time.tv_usec - (t->tv_usec + interval_ms * 1000)) / 1000000);

		if (difftime > 0) {
			t->tv_sec  = base_time.tv_sec;
			t->tv_usec = base_time.tv_usec;
			return true;
		}
		return false;
	}
	/* 현재 시간 저장 */
	t->tv_sec  = base_time.tv_sec;
	t->tv_usec = base_time.tv_usec;
	return true;
}

//------------------------------------------------------------------------------
void time_display (jig_client_t *pclient)
{
	static struct timeval i_time;
	static int i = 0;

	if (run_interval_check(&i_time, 500)) {
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
		ui_set_printf (pclient->pfb, pclient->pui, 0, "%s", pclient->model);
		ui_set_printf (pclient->pfb, pclient->pui, 1, "%s", pclient->bdate);
		ui_set_printf (pclient->pfb, pclient->pui, 2, "%02d:%02d:%02d",
			tm.tm_hour, tm.tm_min, tm.tm_sec);

		if (i++ % 2) {
			ui_set_ritem (pclient->pfb, pclient->pui, 0, COLOR_GREEN, -1);
			ui_set_sitem (pclient->pfb, pclient->pui, 3, COLOR_RED, -1, "OFF");
		}
		else {
			ui_set_ritem (pclient->pfb, pclient->pui, 0, COLOR_RED, -1);
			ui_set_sitem (pclient->pfb, pclient->pui, 3, COLOR_GREEN, -1, "ON");
		}
		info("%s\n", ctime(&t));
	}
}

//------------------------------------------------------------------------------
int protocol_check(ptc_var_t *var)
{
	/* head & tail check with protocol size */
	if(var->buf[(var->p_sp + var->size -1) % var->size] != '#')	return 0;
	if(var->buf[(var->p_sp               ) % var->size] != '@')	return 0;
	return 1;
}

//------------------------------------------------------------------------------
int protocol_catch(ptc_var_t *var)
{
	int i;
	char rdata[PROTOCOL_DATA_SIZE], resp = var->buf[(var->p_sp + 1) % var->size];

	memset (rdata, 0, sizeof(PROTOCOL_DATA_SIZE));
	switch (resp) {
		case 'C':
			for (i = 2; i < var->size -2; i++) {
				rdata[i] = var->buf[(var->p_sp +i) % var->size];
				printf("%c", rdata[i]);
			}
		break;
		case 'R':
		default :
			info ("%s : resp = %c\n", __func__, resp);
		break;
	}
	return 1;
}

//------------------------------------------------------------------------------
void send_msg (jig_client_t *pclient, char cmd, __u8 cmd_id, char *pmsg)
{
	protocol_t s;
	int m_size, pos;
	__u8 *p = (__u8 *)&s;

	memset (&s, 0, sizeof(protocol_t));
	s.head = '@';	s.tail = '#';
	s.cmd  = cmd;

	pos = sprintf(s.data, "%03d,", cmd_id);

	if (pmsg != NULL) {
		m_size = strlen(pmsg);
		m_size = (m_size > (PROTOCOL_DATA_SIZE - pos)) ?
							(PROTOCOL_DATA_SIZE - pos) : m_size;
		strncpy (&s.data[pos], pmsg, m_size);
	}
	for (m_size = 0; m_size < sizeof(protocol_t); m_size++) {
		queue_put(&pclient->puart->tx_q, p + m_size);
	}
}

//------------------------------------------------------------------------------
void run_gpio_cmd (jig_client_t *pclient, char cmd_id)
{
	char adc_pin[10], s_msg[PROTOCOL_DATA_SIZE], *ptr;
	int gpio_no, gpio_level, ui_id;

	memset (adc_pin, 0, sizeof(adc_pin));
	ptr = strtok (NULL, ",");	strncpy(adc_pin, ptr, strlen(ptr));
	ptr = strtok (NULL, ",");	gpio_no    = atoi(ptr);
	ptr = strtok (NULL, ",");	gpio_level = atoi(ptr);
	ptr = strtok (NULL, ",");	ui_id      = atoi(ptr);

	// gpio_control (gpio_no, gpio_level);
	// gpio_get_level = gpio_level_confirm (gpio_no)
	// if gpio_get_level == gpio_level then cmd is 'A'ck or 'O'key else cmd is 'E'rror
	memset (s_msg, 0, sizeof(s_msg));
	sprintf(s_msg, "GPIO,%s,%d,%d,%d", adc_pin, gpio_no, gpio_level, ui_id);

	info ("%s : adc_pin = %s, gpio_no = %d, gpio_level = %d\n",
				__func__, adc_pin, gpio_no, gpio_level);

	send_msg (pclient, 'O', cmd_id, s_msg);
}

//------------------------------------------------------------------------------
void run_usb_cmd (jig_client_t *pclient, char cmd_id)
{

}

//------------------------------------------------------------------------------
void run_uart_cmd (jig_client_t *pclient, char cmd_id)
{

}

//------------------------------------------------------------------------------
void catch_msg (ptc_var_t *var, __u8 *msg)
{
	int i;
	/* header & cmd는 제외 */
	for (i = 0; i < PROTOCOL_DATA_SIZE; i++)
		msg[i] = var->buf[(var->p_sp + 2 + i) % var->size];
}

//------------------------------------------------------------------------------
void recv_msg_check (jig_client_t *pclient, __s8 *msg)
{
	ptc_grp_t *ptc_grp = pclient->puart;
	__u8 idata, p_cnt;

	/* uart data processing */
	if (queue_get (&ptc_grp->rx_q, &idata))
		ptc_event (ptc_grp, idata);

	for (p_cnt = 0; p_cnt < ptc_grp->pcnt; p_cnt++) {
		if (ptc_grp->p[p_cnt].var.pass) {
			__s8 *ptr, cmd_id;

			catch_msg (&ptc_grp->p[p_cnt].var, msg);
			info ("pass message = %s\n", msg);

			ptc_grp->p[p_cnt].var.pass = false;
			ptc_grp->p[p_cnt].var.open = true;
			/*
				cmd & cmd_id check;
			*/
			ptr = strtok (msg, ",");	cmd_id = atoi(ptr);

			ptr = strtok (NULL, ",");
			if (!strncmp(ptr,  "GPIO", strlen("GPIO")))	run_gpio_cmd (pclient, cmd_id);
			if (!strncmp(ptr,   "USB", strlen("USB")))	run_usb_cmd  (pclient, cmd_id);
			if (!strncmp(ptr,  "UART", strlen("UART")))	run_uart_cmd (pclient, cmd_id);
			memset(msg, 0, PROTOCOL_DATA_SIZE);
		}
	}
}

//------------------------------------------------------------------------------
int client_main (jig_client_t *pclient)
{
	__s8 MsgData[PROTOCOL_DATA_SIZE];

	if (ptc_grp_init (pclient->puart, 1)) {
		if (!ptc_func_init (pclient->puart, 0, sizeof(protocol_t), 
    							protocol_check, protocol_catch))
			return 0;
	}

	while (1) {
		time_display(pclient);

		recv_msg_check (pclient, MsgData);

		usleep(SYSTEM_LOOP_DELAY_uS);
	}
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
