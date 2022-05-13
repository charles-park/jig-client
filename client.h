//-----------------------------------------------------------------------------
/**
 * @file client.h
 * @author charles-park (charles-park@hardkernel.com)
 * @brief client control header file.
 * @version 0.1
 * @date 2022-05-13
 * 
 * @copyright Copyright (c) 2022
 * 
 */
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef __CLIENT_H__
#define __CLIENT_H__

//-----------------------------------------------------------------------------
#include "typedefs.h"
#include "lib_fb.h"
#include "lib_ui.h"
#include "lib_uart.h"

//-----------------------------------------------------------------------------
#define	PROTOCOL_DATA_SIZE	32

#pragma packet(1)
typedef struct protocol__t {
	/* @ : start protocol signal */
	__s8	head;

	/*
		command description:
			server to client : 'C'ommand, 'R'eady(boot)
			client to server : 'O'kay, 'A'ck, 'R'eady(boot), 'E'rror, 'B'usy
	*/
	__s8	cmd;

	/* msg no, msg group, msg data1, msg data2, ... */
	__s8	data[PROTOCOL_DATA_SIZE];

	/* # : end protocol signal */
	__s8	tail;
}	protocol_t;

//------------------------------------------------------------------------------
#define	CMD_COUNT_MAX	128

typedef struct jig_client__t {
	/* build info */
	char		bdate[32], btime[32];
	/* JIG model name */
	char		model[32];
	/* UART dev node */
	char		uart_dev[32];
	/* FB dev node */
	char		fb_dev[32];

	fb_info_t	*pfb;
	ui_grp_t	*pui;
	ptc_grp_t	*puart;

	bool		cmd_run;
	char		cmd_id;
}	jig_client_t;

//------------------------------------------------------------------------------
extern  int client_main (jig_client_t *pclient);

//------------------------------------------------------------------------------
#endif  // #define __CLIENT_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
