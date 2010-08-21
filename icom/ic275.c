/*
 *  Hamlib CI-V backend - description of IC-275 and variations
 *  Copyright (c) 2000-2010 by Stephane Fillod
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details.
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "hamlib/rig.h"
#include "icom.h"


#define IC275_MODES (RIG_MODE_CW|RIG_MODE_SSB|RIG_MODE_FM)

#define IC275_VFO_ALL (RIG_VFO_A|RIG_VFO_B|RIG_VFO_MEM)

#define IC275_VFO_OPS (RIG_OP_FROM_VFO|RIG_OP_TO_VFO)


/*
 * FIXME: this appears to be the IC-275A/E
 * what about the IC-275H ? please give it a fix. --SF
 */
static const struct icom_priv_caps ic275_priv_caps = { 
		0x10,	/* default address */
		0,		/* 731 mode */
		ic737_ts_sc_list
};

const struct rig_caps ic275_caps = {
.rig_model =  RIG_MODEL_IC275,
.model_name = "IC-275", 
.mfg_name =  "Icom", 
.version =  BACKEND_VER ".1",
.copyright =  "LGPL",
.status =  RIG_STATUS_BETA,
.rig_type =  RIG_TYPE_TRANSCEIVER,
.ptt_type =  RIG_PTT_NONE,
.dcd_type =  RIG_DCD_NONE,
.port_type =  RIG_PORT_SERIAL,
.serial_rate_min =  1200,
.serial_rate_max =  9600,
.serial_data_bits =  8,
.serial_stop_bits =  1,
.serial_parity =  RIG_PARITY_NONE,
.serial_handshake =  RIG_HANDSHAKE_NONE, 
.write_delay =  0,
.post_write_delay =  0,
.timeout =  200,
.retry =  3, 
.has_get_func =  RIG_FUNC_NONE,
.has_set_func =  RIG_FUNC_NONE, 
.has_get_level =  RIG_LEVEL_NONE,
.has_set_level =  RIG_LEVEL_NONE,
.has_get_parm =  RIG_PARM_NONE,
.has_set_parm =  RIG_PARM_NONE,
.level_gran =  {},
.parm_gran =  {},
.ctcss_list =  NULL,
.dcs_list =  NULL,
.preamp =   { RIG_DBLST_END, },
.attenuator =   { RIG_DBLST_END, },
.max_rit =  Hz(0),
.max_xit =  Hz(0),
.max_ifshift =  Hz(0),
.targetable_vfo =  0,
.vfo_ops =  IC275_VFO_OPS,
.scan_ops =  RIG_SCAN_NONE,
.transceive =  RIG_TRN_RIG,
.bank_qty =   0,
.chan_desc_sz =  0,

.chan_list =  {
		   {   1,  99, RIG_MTYPE_MEM, IC_MIN_MEM_CAP  },
		   { 100, 101, RIG_MTYPE_EDGE, IC_MIN_MEM_CAP },
		   { 102, 102, RIG_MTYPE_CALL, IC_MIN_MEM_CAP },
		   RIG_CHAN_END,
		},

.rx_range_list1 =   { {MHz(138),MHz(174),IC275_MODES,-1,-1,IC275_VFO_ALL},
	RIG_FRNG_END, },
.tx_range_list1 =  { {MHz(144),MHz(146),IC275_MODES,W(2.5),W(25),IC275_VFO_ALL},
	RIG_FRNG_END, },

.rx_range_list2 =   { {MHz(138),MHz(174),IC275_MODES,-1,-1,IC275_VFO_ALL},
	RIG_FRNG_END, },
.tx_range_list2 =  { {MHz(144),MHz(148),IC275_MODES,W(2.5),W(25),IC275_VFO_ALL},
	RIG_FRNG_END, },

.tuning_steps = 	{
	 {IC275_MODES,10},	/* TBC: does this rig supports settin tuning step? */
	 RIG_TS_END,
	},
	/* mode/filter list, remember: order matters! */
.filters = 	{
		{RIG_MODE_SSB|RIG_MODE_CW, kHz(2.2)},
		{RIG_MODE_FM, kHz(15)},
		RIG_FLT_END,
	},

.cfgparams =  icom_cfg_params,
.set_conf =  icom_set_conf,
.get_conf =  icom_get_conf,

.priv =  (void*)&ic275_priv_caps,
.rig_init =   icom_init,
.rig_cleanup =   icom_cleanup,
.rig_open =  NULL,
.rig_close =  NULL,

.set_freq =  icom_set_freq,
.get_freq =  icom_get_freq,
.set_mode =  icom_set_mode,
.get_mode =  icom_get_mode,
.set_vfo =  icom_set_vfo,

.decode_event =  icom_decode_event,
.set_mem =  icom_set_mem,
.vfo_op =  icom_vfo_op,

};

