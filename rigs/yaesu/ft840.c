/*
 * hamlib - (C) Frank Singleton 2000 (javabear at users.sourceforge.net)
 *
 * ft840.c - (C) Frank Singleton 2000 (javabear at users.sourceforge.net)
 *           (C) Stephane Fillod 2002-2009 (fillods at users.sourceforge.net)
 *           (C) Nate Bargmann 2002, 2003 (n0nb at arrl.net)
 *
 * This shared library provides an API for communicating
 * via serial interface to an FT-840 using the "CAT" interface
 *
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <stdlib.h>
#include <string.h>  /* String function definitions */

#include "hamlib/rig.h"
#include "bandplan.h"
#include "iofunc.h"
#include "misc.h"
#include "yaesu.h"
#include "ft840.h"

/*
 * Native FT840 functions. More to come :-)
 *
 */

enum ft840_native_cmd_e
{
    FT840_NATIVE_SPLIT_OFF = 0,
    FT840_NATIVE_SPLIT_ON,
    FT840_NATIVE_RECALL_MEM,
    FT840_NATIVE_VFO_TO_MEM,
    FT840_NATIVE_VFO_A,
    FT840_NATIVE_VFO_B,
    FT840_NATIVE_MEM_TO_VFO,
    FT840_NATIVE_CLARIFIER_OPS,
    FT840_NATIVE_FREQ_SET,
    FT840_NATIVE_MODE_SET,
    FT840_NATIVE_PACING,
    FT840_NATIVE_PTT_OFF,
    FT840_NATIVE_PTT_ON,
    FT840_NATIVE_MEM_CHNL,
    FT840_NATIVE_OP_DATA,
    FT840_NATIVE_VFO_DATA,
    FT840_NATIVE_MEM_CHNL_DATA,
    FT840_NATIVE_TUNER_OFF,
    FT840_NATIVE_TUNER_ON,
    FT840_NATIVE_TUNER_START,
    FT840_NATIVE_READ_METER,
    FT840_NATIVE_READ_FLAGS,
    FT840_NATIVE_SIZE             /* end marker, value indicates number of */
    /* native cmd entries */
};

/*
 *
 * The FT-840 backend is cloned after the FT-890 backend.
 *
 * The differences are (as reported by Thomas - DK6KD):
 * -the 840 has 100 Memory channel, the 890 has 20
 *  -the 840 has 5 flag bytes, like the 890 but the third isn't specified in
 *  the manual (the 890 has the ptt_enabled bit there, which is used by the
 *  FT-890's get_ptt function), i need to investigate if there is really no
 *  information in the third byte from the 840.
 *  -the 840 RIT offset can not be set by a CAT command. The command exists
 *  but it can only enable/disable RIT, it seems that the following bytes
 *  given to this command containing the offset are ignored.
 *  -the display brightness of the 840 can not be set by CAT
 *  (This is an important feature, isn't it? HI)
 */

/*
 * Functions considered to be Beta code (2003-04-11):
 * set_freq
 * get_freq
 * set_mode
 * get_mode
 * set_vfo
 * get_vfo
 * set_ptt
 * get_ptt
 * set_split
 * get_split
 * set_rit
 * get_rit
 * set_func
 * get_func
 * get_level
 *
 * Functions considered to be Alpha code (2003-04-11):
 * vfo_op
 *
 * functions not yet implemented (2003-04-11):
 * set_split_freq
 * get_split_freq
 * set_split_mode
 * get_split_mode
 *
 */

/* Enable these as needed: */
#undef USE_FT840_GET_PTT
#undef USE_FT840_SET_RIT

/*
 * API local implementation
 *
 */

static int ft840_init(RIG *rig);
static int ft840_cleanup(RIG *rig);
static int ft840_open(RIG *rig);
static int ft840_close(RIG *rig);

static int ft840_set_freq(RIG *rig, vfo_t vfo, freq_t freq);
static int ft840_get_freq(RIG *rig, vfo_t vfo, freq_t *freq);

static int ft840_set_mode(RIG *rig, vfo_t vfo, rmode_t mode, pbwidth_t width);
static int ft840_get_mode(RIG *rig, vfo_t vfo, rmode_t *mode, pbwidth_t *width);

static int ft840_set_vfo(RIG *rig, vfo_t vfo);
static int ft840_get_vfo(RIG *rig, vfo_t *vfo);

static int ft840_set_ptt(RIG *rig, vfo_t vfo, ptt_t ptt);
#ifdef USE_FT840_GET_PTT
static int ft840_get_ptt(RIG *rig, vfo_t vfo, ptt_t *ptt);
#endif /* USE_FT840_GET_PTT */

static int ft840_set_split_vfo(RIG *rig, vfo_t vfo, split_t split,
                               vfo_t tx_vfo);
static int ft840_get_split_vfo(RIG *rig, vfo_t vfo, split_t *split,
                               vfo_t *tx_vfo);

#ifdef USE_FT840_SET_RIT
static int ft840_set_rit(RIG *rig, vfo_t vfo, shortfreq_t rit);
#endif /* USE_FT840_SET_RIT */
static int ft840_get_rit(RIG *rig, vfo_t vfo, shortfreq_t *rit);

static int ft840_set_func(RIG *rig, vfo_t vfo, setting_t func, int status);

static int ft840_get_level(RIG *rig, vfo_t vfo, setting_t level, value_t *val);

static int ft840_vfo_op(RIG *rig, vfo_t vfo, vfo_op_t op);


/* Private helper function prototypes */

static int ft840_get_update_data(RIG *rig, unsigned char ci, unsigned char rl);

static int ft840_send_static_cmd(RIG *rig, unsigned char ci);

static int ft840_send_dynamic_cmd(RIG *rig, unsigned char ci,
                                  unsigned char p1, unsigned char p2,
                                  unsigned char p3, unsigned char p4);

static int ft840_send_dial_freq(RIG *rig, unsigned char ci, freq_t freq);

#ifdef USE_FT840_SET_RIT
static int ft840_send_rit_freq(RIG *rig, unsigned char ci, shortfreq_t rit);
#endif /* USE_FT840_SET_RIT */


/*
 * Native ft840 cmd set prototypes. These are READ ONLY as each
 * rig instance will copy from these and modify if required.
 * Complete sequences (1) can be read and used directly as a cmd sequence.
 * Incomplete sequences (0) must be completed with extra parameters
 * eg: mem number, or freq etc..
 *
 * TODO: Shorten this static array with parameter substitution -N0NB
 *
 */

static const yaesu_cmd_set_t ncmd[] =
{
    { 1, { 0x00, 0x00, 0x00, 0x00, 0x01 } }, /* split = off */
    { 1, { 0x00, 0x00, 0x00, 0x01, 0x01 } }, /* split = on */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x02 } }, /* recall memory */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x03 } }, /* memory operations */
    { 1, { 0x00, 0x00, 0x00, 0x00, 0x05 } }, /* select vfo A */
    { 1, { 0x00, 0x00, 0x00, 0x01, 0x05 } }, /* select vfo B */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x06 } }, /* copy memory data to vfo A */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x09 } }, /* clarifier operations */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x0a } }, /* set display freq */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x0c } }, /* mode set */
    { 0, { 0x00, 0x00, 0x00, 0x00, 0x0e } }, /* update interval/pacing */
    { 1, { 0x00, 0x00, 0x00, 0x00, 0x0f } }, /* PTT off */
    { 1, { 0x00, 0x00, 0x00, 0x01, 0x0f } }, /* PTT on */
    { 1, { 0x00, 0x00, 0x00, 0x01, 0x10 } }, /* Status Update Data--Memory Channel Number (1 byte) */
    { 1, { 0x00, 0x00, 0x00, 0x02, 0x10 } }, /* Status Update Data--Current operating data for VFO/Memory (19 bytes) */
    { 1, { 0x00, 0x00, 0x00, 0x03, 0x10 } }, /* Status Update DATA--VFO A and B Data (18 bytes) */
    { 0, { 0x00, 0x00, 0x00, 0x04, 0x10 } }, /* Status Update Data--Memory Channel Data (19 bytes) P4 = 0x01-0x20 Memory Channel Number */
    { 1, { 0x00, 0x00, 0x00, 0x00, 0x81 } }, /* tuner off */
    { 1, { 0x00, 0x00, 0x00, 0x01, 0x81 } }, /* tuner on */
    { 1, { 0x00, 0x00, 0x00, 0x00, 0x82 } }, /* tuner start*/
    { 1, { 0x00, 0x00, 0x00, 0x00, 0xf7 } }, /* Read meter, S on RX, ALC|PO|SWR on TX */
    { 1, { 0x00, 0x00, 0x00, 0x00, 0xfa } }, /* Read status flags */

};


/*
 * future - private data
 *
 * FIXME: Does this need to be exposed to the application/frontend through
 * ft840_caps.priv? -N0NB
 */

struct ft840_priv_data
{
    unsigned char pacing;                     /* pacing value */
    vfo_t current_vfo;                        /* active VFO from last cmd */
    unsigned char
    p_cmd[YAESU_CMD_LENGTH];    /* private copy of 1 constructed CAT cmd */
    unsigned char
    update_data[FT840_ALL_DATA_LENGTH]; /* returned data--max value, some are less */
    unsigned char current_mem;                   /* private memory channel number */
};

/*
 * ft840 rigs capabilities.
 * Also this struct is READONLY!
 *
 */

struct rig_caps ft840_caps =
{
    RIG_MODEL(RIG_MODEL_FT840),
    .model_name =         "FT-840",
    .mfg_name =           "Yaesu",
    .version =            "20200323.0",
    .copyright =          "LGPL",
    .status =             RIG_STATUS_STABLE,
    .rig_type =           RIG_TYPE_TRANSCEIVER,
    .ptt_type =           RIG_PTT_RIG,
    .dcd_type =           RIG_DCD_NONE,
    .port_type =          RIG_PORT_SERIAL,
    .serial_rate_min =    4800,
    .serial_rate_max =    4800,
    .serial_data_bits =   8,
    .serial_stop_bits =   2,
    .serial_parity =      RIG_PARITY_NONE,
    .serial_handshake =   RIG_HANDSHAKE_NONE,
    .write_delay =        FT840_WRITE_DELAY,
    .post_write_delay =   FT840_POST_WRITE_DELAY,
    .timeout =            2000,
    .retry =              0,
    .has_get_func =       RIG_FUNC_TUNER,
    .has_set_func =       RIG_FUNC_TUNER,
    .has_get_level =      RIG_LEVEL_STRENGTH,
    .has_set_level =      RIG_LEVEL_BAND_SELECT,
    .has_get_parm =       RIG_PARM_NONE,
    .has_set_parm =       RIG_PARM_NONE,
    .level_gran =
    {
#include "level_gran_yaesu.h"
    },
    .ctcss_list =         NULL,
    .dcs_list =           NULL,
    .preamp =             { RIG_DBLST_END, },
    .attenuator =         { RIG_DBLST_END, },
    .max_rit =            Hz(9999),
    .max_xit =            Hz(0),
    .max_ifshift =        Hz(0),
    .vfo_ops =            RIG_OP_TUNE,
    .targetable_vfo =     RIG_TARGETABLE_ALL,
    .transceive =         RIG_TRN_OFF,        /* Yaesus have to be polled, sigh */
    .bank_qty =           0,
    .chan_desc_sz =       0,
    .chan_list =          { RIG_CHAN_END, },  /* FIXME: memory channel list: 100 */

    .rx_range_list1 =     {
        {kHz(100), MHz(30), FT840_ALL_RX_MODES, -1, -1, FT840_VFO_ALL, FT840_ANTS},   /* General coverage + ham */
        RIG_FRNG_END,
    }, /* FIXME:  Are these the correct Region 1 values? */

    .tx_range_list1 =     {
        FRQ_RNG_HF(1, FT840_OTHER_TX_MODES, W(5), W(100), FT840_VFO_ALL, FT840_ANTS),
        FRQ_RNG_HF(1, FT840_AM_TX_MODES, W(2), W(25), FT840_VFO_ALL, FT840_ANTS),   /* AM class */

        RIG_FRNG_END,
    },

    .rx_range_list2 =     {
        {kHz(100), MHz(30), FT840_ALL_RX_MODES, -1, -1, FT840_VFO_ALL, FT840_ANTS},
        RIG_FRNG_END,
    },

    .tx_range_list2 =     {
        FRQ_RNG_HF(2, FT840_OTHER_TX_MODES, W(5), W(100), FT840_VFO_ALL, FT840_ANTS),
        FRQ_RNG_HF(2, FT840_AM_TX_MODES, W(2), W(25), FT840_VFO_ALL, FT840_ANTS),   /* AM class */

        RIG_FRNG_END,
    },

    .tuning_steps =       {
        {FT840_SSB_CW_RX_MODES, Hz(10)},    /* Normal */
        {FT840_SSB_CW_RX_MODES, Hz(100)},   /* Fast */

        {FT840_AM_RX_MODES,     Hz(100)},   /* Normal */
        {FT840_AM_RX_MODES,     kHz(1)},    /* Fast */

        {FT840_FM_RX_MODES,     Hz(100)},   /* Normal */
        {FT840_FM_RX_MODES,     kHz(1)},    /* Fast */

        RIG_TS_END,

    },

    /* mode/filter list, .remember =  order matters! */
    .filters =            {
        {RIG_MODE_SSB,  kHz(2.2)},  /* standard SSB filter bandwidth */
        {RIG_MODE_CW,   kHz(2.2)},  /* normal CW filter */
        {RIG_MODE_CW,   kHz(0.5)},  /* CW filter with narrow selection (must be installed!) */
        {RIG_MODE_AM,   kHz(6)},    /* normal AM filter */
        {RIG_MODE_AM,   kHz(2.2)},  /* AM filter with narrow selection (SSB filter switched in) */
        {RIG_MODE_FM,   kHz(12)},   /* FM  */

        RIG_FLT_END,
    },

    .rig_init =           ft840_init,
    .rig_cleanup =        ft840_cleanup,
    .rig_open =           ft840_open,     /* port opened */
    .rig_close =          ft840_close,    /* port closed */

    .set_freq =           ft840_set_freq,
    .get_freq =           ft840_get_freq,
    .set_mode =           ft840_set_mode,
    .get_mode =           ft840_get_mode,
    .set_vfo =            ft840_set_vfo,
    .get_vfo =            ft840_get_vfo,
    .set_ptt =            ft840_set_ptt,
#ifdef USE_FT840_GET_PTT
    .get_ptt =            ft840_get_ptt,
#endif /* USE_FT840_GET_PTT */
    .set_split_vfo =          ft840_set_split_vfo,
    .get_split_vfo =          ft840_get_split_vfo,
#ifdef USE_FT840_SET_RIT
    .set_rit =            ft840_set_rit,
#endif /* USE_FT840_SET_RIT */
    .get_rit =            ft840_get_rit,
    .set_func =           ft840_set_func,
    .get_level =          ft840_get_level,
    .vfo_op =             ft840_vfo_op,
    .hamlib_check_rig_caps = HAMLIB_CHECK_RIG_CAPS
};


/*
 * ************************************
 *
 * Hamlib API functions
 *
 * ************************************
 */

/*
 * rig_init
 *
 */

static int ft840_init(RIG *rig)
{
    struct ft840_priv_data *priv;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    STATE(rig)->priv = (struct ft840_priv_data *) calloc(1,
                       sizeof(struct ft840_priv_data));

    if (!STATE(rig)->priv)                       /* whoops! memory shortage! */
    {
        return -RIG_ENOMEM;
    }

    priv = STATE(rig)->priv;

    /* TODO: read pacing from preferences */
    priv->pacing = FT840_PACING_DEFAULT_VALUE; /* set pacing to minimum for now */
    priv->current_vfo =  RIG_VFO_MAIN;  /* default to whatever */

    return RIG_OK;
}


/*
 * rig_cleanup
 *
 * the serial port is closed by the frontend
 *
 */

static int ft840_cleanup(RIG *rig)
{

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    if (STATE(rig)->priv)
    {
        free(STATE(rig)->priv);
    }

    STATE(rig)->priv = NULL;

    return RIG_OK;
}


/*
 * rig_open
 *
 */

static int ft840_open(RIG *rig)
{
    struct ft840_priv_data *priv;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    rig_debug(RIG_DEBUG_TRACE, "%s: write_delay = %i msec\n",
              __func__, RIGPORT(rig)->write_delay);
    rig_debug(RIG_DEBUG_TRACE, "%s: post_write_delay = %i msec\n",
              __func__, RIGPORT(rig)->post_write_delay);
    rig_debug(RIG_DEBUG_TRACE,
              "%s: read pacing = %i\n", __func__, priv->pacing);

    err = ft840_send_dynamic_cmd(rig, FT840_NATIVE_PACING,
                                 priv->pacing, 0, 0, 0);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * rig_close
 *
 */

static int ft840_close(RIG *rig)
{

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    return RIG_OK;
}


/*
 * rig_set_freq
 *
 * Set frequency for a given VFO
 *
 * If vfo is set to RIG_VFO_CUR then vfo from priv_data is used.
 * If vfo differs from stored value then VFO will be set to the
 * passed vfo.
 *
 */

static int ft840_set_freq(RIG *rig, vfo_t vfo, freq_t freq)
{
    struct ft840_priv_data *priv;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed freq = %"PRIfreq" Hz\n", __func__, freq);

    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE, "%s: priv->current_vfo = 0x%02x\n",
                  __func__, vfo);
    }
    else if (vfo != priv->current_vfo)
    {
        /* force a VFO change if requested vfo value differs from stored value */
        err = ft840_set_vfo(rig, vfo);

        if (err != RIG_OK)
        {
            return err;
        }
    }

    err = ft840_send_dial_freq(rig, FT840_NATIVE_FREQ_SET, freq);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * rig_get_freq
 *
 * Return Freq for a given VFO
 *
 */

static int ft840_get_freq(RIG *rig, vfo_t vfo, freq_t *freq)
{
    struct ft840_priv_data *priv;
    unsigned char *p;
    unsigned char offset;
    freq_t f;
    int err, cmd_index, count;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (vfo == RIG_VFO_CURR)
    {
        err = ft840_get_vfo(rig, &priv->current_vfo);

        if (err != RIG_OK)
        {
            return err;
        }

        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    }

    switch (vfo)
    {
    case RIG_VFO_A:
    case RIG_VFO_VFO:
        cmd_index = FT840_NATIVE_VFO_DATA;
        offset = FT840_SUMO_VFO_A_FREQ;
        count = FT840_VFO_DATA_LENGTH;
        break;

    case RIG_VFO_B:
        cmd_index = FT840_NATIVE_VFO_DATA;
        offset = FT840_SUMO_VFO_B_FREQ;
        count = FT840_VFO_DATA_LENGTH;
        break;

    case RIG_VFO_MEM:
    case RIG_VFO_MAIN:
        cmd_index = FT840_NATIVE_OP_DATA;
        offset = FT840_SUMO_DISPLAYED_FREQ;
        count = FT840_OP_DATA_LENGTH;
        break;

    default:
        return -RIG_EINVAL;         /* sorry, wrong VFO */
    }

    err = ft840_get_update_data(rig, cmd_index, count);

    if (err != RIG_OK)
    {
        return err;
    }

    p = &priv->update_data[offset];

    /* big endian integer */
    f = ((((p[0] << 8) + p[1]) << 8) + p[2]) * 10;

    rig_debug(RIG_DEBUG_TRACE,
              "%s: freq = %"PRIfreq" Hz for vfo 0x%02x\n", __func__, f, vfo);

    *freq = f;                    /* return displayed frequency */

    return RIG_OK;
}


/*
 * rig_set_mode
 *
 * set mode and passband: eg AM, CW etc for a given VFO
 *
 * If vfo is set to RIG_VFO_CUR then vfo from priv_data is used.
 *
 */

static int ft840_set_mode(RIG *rig, vfo_t vfo, rmode_t mode,
                          pbwidth_t width)
{
    struct ft840_priv_data *priv;
    unsigned char mode_parm;      /* mode parameter */
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = %s\n", __func__, rig_strvfo(vfo));
    rig_debug(RIG_DEBUG_TRACE, "%s: passed mode = %s\n", __func__,
              rig_strrmode(mode));
    rig_debug(RIG_DEBUG_TRACE, "%s: passed width = %d Hz\n", __func__, (int)width);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo  = 0x%02x\n", __func__, vfo);
    }

    /* translate mode from generic to ft840 specific */
    switch (vfo)
    {
    case RIG_VFO_A:               /* force to VFO */
    case RIG_VFO_VFO:
        err = ft840_set_vfo(rig, RIG_VFO_A);

        if (err != RIG_OK)
        {
            return err;
        }

        break;

    case RIG_VFO_B:
        err = ft840_set_vfo(rig, RIG_VFO_B);

        if (err != RIG_OK)
        {
            return err;
        }

        break;

    case RIG_VFO_MEM:             /* MEM TUNE or user doesn't care */
    case RIG_VFO_MAIN:
        break;

    default:
        return -RIG_EINVAL;       /* sorry, wrong VFO */
    }

    switch (mode)
    {
    case RIG_MODE_AM:
        mode_parm = MODE_SET_AM_W;
        break;

    case RIG_MODE_CW:
        mode_parm = MODE_SET_CW_W;
        break;

    case RIG_MODE_USB:
        mode_parm = MODE_SET_USB;
        break;

    case RIG_MODE_LSB:
        mode_parm = MODE_SET_LSB;
        break;

    case RIG_MODE_FM:
        mode_parm = MODE_SET_FM;
        break;

    default:
        return -RIG_EINVAL;       /* sorry, wrong MODE */
    }

    /*
     * Now set width (shamelessly stolen from ft847.c and then butchered :)
     * The FT-840 only supports narrow width in AM and CW modes
     *
     */
    if (width != RIG_PASSBAND_NOCHANGE)
    {
        if (width == rig_passband_narrow(rig, mode))
        {
            switch (mode)
            {
            case RIG_MODE_CW:
                mode_parm = MODE_SET_CW_N;
                break;

            case RIG_MODE_AM:
                mode_parm = MODE_SET_AM_N;
                break;

            default:
                return -RIG_EINVAL; /* Invalid mode, how can caller know? */
            }
        }
        else
        {
            if (width != RIG_PASSBAND_NORMAL &&
                    width != rig_passband_normal(rig, mode))
            {
                return -RIG_EINVAL; /* Invalid width, how can caller know? */
            }
        }
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: set mode_parm = 0x%02x\n", __func__, mode_parm);

    err = ft840_send_dynamic_cmd(rig, FT840_NATIVE_MODE_SET,
                                 mode_parm, 0, 0, 0);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;                /* good */
}


/*
 * rig_get_mode
 *
 * get mode eg AM, CW etc for a given VFO
 *
 */

static int ft840_get_mode(RIG *rig, vfo_t vfo, rmode_t *mode, pbwidth_t *width)
{
    struct ft840_priv_data *priv;
    unsigned char my_mode, m_offset;    /* ft840 mode, mode offset */
    unsigned char flag, f_offset;       /* CW/AM narrow flag */
    int err, cmd_index, norm, count;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    }

    switch (vfo)
    {
    case RIG_VFO_A:
    case RIG_VFO_VFO:
        cmd_index = FT840_NATIVE_VFO_DATA;
        m_offset = FT840_SUMO_VFO_A_MODE;
        f_offset = FT840_SUMO_VFO_A_FLAG;
        count = FT840_VFO_DATA_LENGTH;
        break;

    case RIG_VFO_B:
        cmd_index = FT840_NATIVE_VFO_DATA;
        m_offset = FT840_SUMO_VFO_B_MODE;
        f_offset = FT840_SUMO_VFO_B_FLAG;
        count = FT840_VFO_DATA_LENGTH;
        break;

    case RIG_VFO_MEM:
    case RIG_VFO_MAIN:
        cmd_index = FT840_NATIVE_OP_DATA;
        m_offset = FT840_SUMO_DISPLAYED_MODE;
        f_offset = FT840_SUMO_DISPLAYED_FLAG;
        count = FT840_OP_DATA_LENGTH;
        break;

    default:
        return -RIG_EINVAL;
    }

    err = ft840_get_update_data(rig, cmd_index, count);

    if (err != RIG_OK)
    {
        return err;
    }

    my_mode = MODE_MASK & priv->update_data[m_offset];
    flag = FLAG_MASK & priv->update_data[f_offset];


    rig_debug(RIG_DEBUG_TRACE, "%s: mode = %s\n", __func__, rig_strrmode(*mode));
    rig_debug(RIG_DEBUG_TRACE, "%s: flag = 0x%02x\n", __func__, flag);

    /*
     * translate mode from ft840 to generic.
     */
    switch (my_mode)
    {
    case MODE_LSB:
        *mode = RIG_MODE_LSB;
        norm = TRUE;
        break;

    case MODE_USB:
        *mode = RIG_MODE_USB;
        norm = TRUE;
        break;

    case MODE_CW:
        *mode = RIG_MODE_CW;

        if (flag & FLAG_CW_N)
        {
            norm = FALSE;
        }
        else
        {
            norm = TRUE;
        }

        break;

    case MODE_AM:
        *mode = RIG_MODE_AM;

        if (flag & FLAG_AM_N)
        {
            norm = FALSE;
        }
        else
        {
            norm = TRUE;
        }

        break;

    case MODE_FM:
        *mode = RIG_MODE_FM;
        norm = TRUE;
        break;

    default:
        return -RIG_EINVAL;         /* Oops! file bug report */
    }

    if (norm)
    {
        *width = rig_passband_normal(rig, *mode);
    }
    else
    {
        *width = rig_passband_narrow(rig, *mode);
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: set mode = %s\n", __func__,
              rig_strrmode(*mode));
    rig_debug(RIG_DEBUG_TRACE, "%s: set width = %d Hz\n", __func__, (int)*width);

    return RIG_OK;
}


/*
 * rig_set_vfo
 *
 * set vfo and store requested vfo for later RIG_VFO_CURR
 * requests.
 *
 */

static int ft840_set_vfo(RIG *rig, vfo_t vfo)
{
    struct ft840_priv_data *priv;
    unsigned char cmd_index;      /* index of sequence to send */
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    }

    /* FIXME: Include support for RIG_VFO_MAIN */
    switch (vfo)
    {
    case RIG_VFO_A:
        cmd_index = FT840_NATIVE_VFO_A;
        priv->current_vfo = vfo;    /* update active VFO */
        break;

    case RIG_VFO_B:
        cmd_index = FT840_NATIVE_VFO_B;
        priv->current_vfo = vfo;
        break;

    case RIG_VFO_MEM:
        /* reset to memory channel stored by previous get_vfo
         * The recall mem channel command uses 0x01 though 0x20
         */
        err = ft840_send_dynamic_cmd(rig, FT840_NATIVE_RECALL_MEM,
                                     (priv->current_mem + 1), 0, 0, 0);

        if (err != RIG_OK)
        {
            return err;
        }

        priv->current_vfo = vfo;

        rig_debug(RIG_DEBUG_TRACE, "%s: set mem channel = 0x%02x\n",
                  __func__, priv->current_mem);
        return RIG_OK;

    default:
        return -RIG_EINVAL;         /* sorry, wrong VFO */
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: set cmd_index = %i\n", __func__, cmd_index);

    err = ft840_send_static_cmd(rig, cmd_index);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * rig_get_vfo
 *
 * get current RX vfo/mem and store requested vfo for
 * later RIG_VFO_CURR requests plus pass the tested vfo/mem
 * back to the frontend.
 *
 */

static int ft840_get_vfo(RIG *rig, vfo_t *vfo)
{
    struct ft840_priv_data *priv;
    unsigned char status_0;           /* ft840 status flag 0 */
    unsigned char stat_vfo, stat_mem; /* status tests */
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    /* Get flags for VFO status */
    err = ft840_get_update_data(rig, FT840_NATIVE_READ_FLAGS,
                                FT840_STATUS_FLAGS_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    status_0 = priv->update_data[FT840_SUMO_DISPLAYED_STATUS_0];
    stat_vfo = status_0 & SF_VFO_MASK;    /* get VFO active bits */
    stat_mem = status_0 & SF_MEM_MASK;    /* get MEM active bits */

    rig_debug(RIG_DEBUG_TRACE,
              "%s: vfo status_0 = 0x%02x\n", __func__, status_0);
    rig_debug(RIG_DEBUG_TRACE,
              "%s: stat_vfo = 0x%02x\n", __func__, stat_vfo);
    rig_debug(RIG_DEBUG_TRACE,
              "%s: stat_mem = 0x%02x\n", __func__, stat_mem);

    /*
     * translate vfo and mem status from ft840 to generic.
     *
     * First a test is made on bits 6 and 7 of status_0.  Bit 7 is set
     * when FT-840 is in VFO mode on display.  Bit 6 is set when VFO B
     * is active and cleared when VFO A is active.
     *
     * Conversely, bit 7 is cleared when MEM or MEM TUNE mode is selected
     * Bit 6 still follows last selected VFO (A or B), but this is not
     * tested right now.
     */
    switch (stat_vfo)
    {
    case SF_VFOA:
        *vfo = RIG_VFO_A;
        priv->current_vfo = RIG_VFO_A;
        break;

    case SF_VFOB:
        *vfo = RIG_VFO_B;
        priv->current_vfo = RIG_VFO_B;
        break;

    default:
        switch (stat_mem)
        {
        case SF_MT:
        case SF_MR:
            *vfo = RIG_VFO_MEM;
            priv->current_vfo = RIG_VFO_MEM;

            /*
             * Per Hamlib policy capture and store memory channel number
             * for future set_vfo command.
             */
            err = ft840_get_update_data(rig, FT840_NATIVE_MEM_CHNL,
                                        FT840_MEM_CHNL_LENGTH);

            if (err != RIG_OK)
            {
                return err;
            }

            priv->current_mem = priv->update_data[FT840_SUMO_MEM_CHANNEL];

            rig_debug(RIG_DEBUG_TRACE, "%s: stored mem channel = 0x%02x\n",
                      __func__, priv->current_mem);
            break;

        default:                      /* Oops! */
            return -RIG_EINVAL;         /* sorry, wrong current VFO */
        }
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: set vfo = 0x%02x\n", __func__, *vfo);

    return RIG_OK;

}


/*
 * rig_set_ptt
 *
 * set the '840 into TX mode
 *
 * vfo is respected by calling ft840_set_vfo if
 * passed vfo != priv->current_vfo
 *
 */

static int ft840_set_ptt(RIG *rig, vfo_t vfo, ptt_t ptt)
{
    struct ft840_priv_data *priv;
    unsigned char cmd_index;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed ptt = 0x%02x\n", __func__, ptt);

    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    }
    else if (vfo != priv->current_vfo)
    {
        ft840_set_vfo(rig, vfo);
    }

    switch (ptt)
    {
    case RIG_PTT_OFF:
        cmd_index = FT840_NATIVE_PTT_OFF;
        break;

    case RIG_PTT_ON:
        cmd_index = FT840_NATIVE_PTT_ON;
        break;

    default:
        return -RIG_EINVAL;         /* wrong PTT state! */
    }

    err = ft840_send_static_cmd(rig, cmd_index);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * rig_get_ptt
 *
 * get current PTT status
 *
 */

#ifdef USE_FT840_GET_PTT
static int ft840_get_ptt(RIG *rig, vfo_t vfo, ptt_t *ptt)
{
    struct ft840_priv_data *priv;
    unsigned char status_2;           /* ft840 status flag 2 */
    unsigned char stat_ptt;           /* status tests */
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    /* Get flags for VFO status */
    err = ft840_get_update_data(rig, FT840_NATIVE_READ_FLAGS,
                                FT840_STATUS_FLAGS_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    status_2 = priv->update_data[FT840_SUMO_DISPLAYED_STATUS_2];
    stat_ptt = status_2 & SF_PTT_MASK;    /* get PTT active bit */

    rig_debug(RIG_DEBUG_TRACE,
              "%s: ptt status_2 = 0x%02x\n", __func__, status_2);

    switch (stat_ptt)
    {
    case SF_PTT_OFF:
        *ptt = RIG_PTT_OFF;
        break;

    case SF_PTT_ON:
        *ptt = RIG_PTT_ON;
        break;

    default:                      /* Oops! */
        return -RIG_EINVAL;         /* Invalid PTT bit?! */
    }

    return RIG_OK;
}
#endif /* USE_FT840_GET_PTT */


/*
 * rig_set_split_vfo
 *
 * set the '840 into split TX/RX mode
 *
 * VFO cannot be set as the set split on command only changes the
 * TX to the other VFO.  Setting split off returns the TX to the
 * main display.
 *
 */

static int ft840_set_split_vfo(RIG *rig, vfo_t vfo, split_t split,
                               vfo_t tx_vfo)
{
    unsigned char cmd_index;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed split = 0x%02x\n", __func__, split);

    switch (split)
    {
    case RIG_SPLIT_OFF:
        cmd_index = FT840_NATIVE_SPLIT_OFF;
        break;

    case RIG_SPLIT_ON:
        cmd_index = FT840_NATIVE_SPLIT_ON;
        break;

    default:
        return -RIG_EINVAL;
    }

    err = ft840_send_static_cmd(rig, cmd_index);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * rig_get_split_vfo
 *
 * Get whether the '840 is in split mode
 *
 * vfo value is not used
 *
 */

static int ft840_get_split_vfo(RIG *rig, vfo_t vfo, split_t *split,
                               vfo_t *tx_vfo)
{
    struct ft840_priv_data *priv;
    unsigned char status_0;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    /* Get flags for VFO split status */
    err = ft840_get_update_data(rig, FT840_NATIVE_READ_FLAGS,
                                FT840_STATUS_FLAGS_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    /* get Split active bit */
    status_0 = SF_SPLIT & priv->update_data[FT840_SUMO_DISPLAYED_STATUS_0];

    rig_debug(RIG_DEBUG_TRACE,
              "%s: split status_0 = 0x%02x\n", __func__, status_0);

    switch (status_0)
    {
    case SF_SPLIT:
        *split = RIG_SPLIT_ON;
        break;

    default:
        *split = RIG_SPLIT_OFF;
        break;
    }

    return RIG_OK;
}


/*
 * rig_set_rit
 *
 * VFO and MEM rit values are independent.
 *
 * passed vfo value is respected.
 *
 * Clarifier offset is retained in the rig for either VFO when the
 * VFO is changed.  Offset is not retained when in memory tune mode
 * and VFO mode is selected or another memory channel is selected.
 *
 */

#ifdef USE_FT840_SET_RIT
static int ft840_set_rit(RIG *rig, vfo_t vfo, shortfreq_t rit)
{
    struct ft840_priv_data *priv;
//  unsigned char offset;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    if (rit < -9990 || rit > 9990)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed rit = %li\n", __func__, rit);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    /*
     * The assumption here is that the user hasn't changed
     * the VFO manually.  Does it really need to be checked
     * every time?  My goal is to reduce the traffic on the
     * serial line to a minimum, but respect the application's
     * request to change the VFO with this call.
     *
     */
    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous rig_get_vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    }
    else if (vfo != priv->current_vfo)
    {
        ft840_set_vfo(rig, vfo);
    }

    /*
     * Shuts clarifier off but does not set frequency to 0 Hz
     */
    if (rit == 0)
    {
        err = ft840_send_dynamic_cmd(rig, FT840_NATIVE_CLARIFIER_OPS,
                                     CLAR_RX_OFF, 0, 0, 0);
        return RIG_OK;
    }

    /*
     * Clarifier must first be turned on then the frequency can
     * be set, +9990 Hz to -9990 Hz
     */
    err = ft840_send_dynamic_cmd(rig, FT840_NATIVE_CLARIFIER_OPS,
                                 CLAR_RX_ON, 0, 0, 0);

    if (err != RIG_OK)
    {
        return err;
    }

    err = ft840_send_rit_freq(rig, FT840_NATIVE_CLARIFIER_OPS, rit);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}
#endif /* USE_FT840_SET_RIT */


/*
 * rig_get_rit
 *
 * Rig returns offset as hex from 0x0000 to 0x03e7 for 0 to +9.990 kHz
 * and 0xffff to 0xfc19 for -1 to -9.990 kHz
 *
 */

static int ft840_get_rit(RIG *rig, vfo_t vfo, shortfreq_t *rit)
{
    struct ft840_priv_data *priv;
    unsigned char *p;
    unsigned char offset;
    shortfreq_t f;
    int err, cmd_index, length;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed vfo = 0x%02x\n", __func__, vfo);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (vfo == RIG_VFO_CURR)
    {
        vfo = priv->current_vfo;    /* from previous vfo cmd */
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: priv->current_vfo = 0x%02x\n", __func__, vfo);
    }

    switch (vfo)
    {
    case RIG_VFO_MEM:
        cmd_index = FT840_NATIVE_OP_DATA;
        offset = FT840_SUMO_DISPLAYED_CLAR;
        length = FT840_OP_DATA_LENGTH;
        break;

    case RIG_VFO_A:
    case RIG_VFO_VFO:
        cmd_index = FT840_NATIVE_VFO_DATA;
        offset = FT840_SUMO_VFO_A_CLAR;
        length = FT840_VFO_DATA_LENGTH;
        break;

    case RIG_VFO_B:
        cmd_index = FT840_NATIVE_VFO_DATA;
        offset = FT840_SUMO_VFO_B_CLAR;
        length = FT840_VFO_DATA_LENGTH;
        break;

    default:
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: set cmd_index = %i\n", __func__, cmd_index);
    rig_debug(RIG_DEBUG_TRACE, "%s: set offset = 0x%02x\n", __func__, offset);

    err = ft840_get_update_data(rig, cmd_index, length);

    if (err != RIG_OK)
    {
        return err;
    }

    p = &priv->update_data[offset];

    /* big endian integer */
    f = (p[0] << 8) + p[1];       /* returned value is hex to nearest hundred Hz */

    if (f > 0xfc18)               /* 0xfc19 to 0xffff is negative offset */
    {
        f = ~(0xffff - f);
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read freq = %li Hz\n", __func__, f * 10);

    *rit = f * 10;                /* store clarifier frequency */

    return RIG_OK;
}


/*
 * rig_set_func
 *
 * set the '840 supported functions
 *
 * vfo is ignored for tuner as it is an independent function
 *
 */

static int ft840_set_func(RIG *rig, vfo_t vfo, setting_t func, int status)
{
    int err, cmd_index;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed func = %s\n", __func__,
              rig_strfunc(func));
    rig_debug(RIG_DEBUG_TRACE, "%s: passed status = %d\n", __func__, status);

    switch (func)
    {
    case RIG_FUNC_TUNER:
        switch (status)
        {
        case OFF:
            cmd_index = FT840_NATIVE_TUNER_OFF;
            break;

        case ON:
            cmd_index = FT840_NATIVE_TUNER_ON;
            break;

        default:
            return -RIG_EINVAL;
        }

        break;

    default:
        return -RIG_EINVAL;
    }

    err = ft840_send_static_cmd(rig, cmd_index);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * rig_get_level
 *
 * get the '840 meter level
 *
 * vfo is ignored for now
 *
 * Meter level returned from FT-840 is S meter when rig is in RX
 * Meter level returned is one of ALC or PO or SWR when rig is in TX
 * depending on front panel meter selection.  Meter selection is NOT
 * available via CAT.
 *
 * TODO: Add support for TX values
 *
 */

static int ft840_get_level(RIG *rig, vfo_t vfo, setting_t level, value_t *val)
{
    struct ft840_priv_data *priv;
    unsigned char *p;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed level = %s\n", __func__,
              rig_strlevel(level));

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    switch (level)
    {
    case RIG_LEVEL_STRENGTH:
        err = ft840_get_update_data(rig, FT840_NATIVE_READ_METER,
                                    FT840_STATUS_FLAGS_LENGTH);

        if (err != RIG_OK)
        {
            return err;
        }

        p = &priv->update_data[FT840_SUMO_METER];

        /*
         * My FT-840 returns a range of 0x00 to 0x44 for S0 to S9 and 0x44 to
         * 0x9d for S9 to S9 +60
         *
         * For ease of calculation I rounded S9 up to 0x48 (72 decimal) and
         * S9 +60 up to 0xa0 (160 decimal).  I calculated a divisor for readings
         * less than S9 by dividing 72 by 54 and the divisor for readings greater
         * than S9 by dividing 88 (160 - 72) by 60.  The result tracks rather well.
         *
         * The greatest error is around S1 and S2 and then from S9 to S9 +35.  Such
         * is life when mapping non-linear S-meters to a linear scale.
         *
         */
        if (*p > 160)
        {
            val->i = 60;
        }
        else  if (*p <= 72)
        {
            val->i = ((72 - *p) / 1.3333) * -1;
        }
        else
        {
            val->i = ((*p - 72) / 1.4667);
        }

        rig_debug(RIG_DEBUG_TRACE, "%s: calculated level = %i\n", __func__, val->i);

        break;

    default:
        return -RIG_EINVAL;
    }

    return RIG_OK;
}


/*
 * rig_vfo_op
 *
 * VFO operations--tuner start, etc
 *
 * vfo is ignored for now
 *
 */

static int ft840_vfo_op(RIG *rig, vfo_t vfo, vfo_op_t op)
{
    int err, cmd_index;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed op = 0x%02x\n", __func__, op);

    switch (op)
    {
    case RIG_OP_TUNE:
        cmd_index = FT840_NATIVE_TUNER_START;
        break;

    default:
        return -RIG_EINVAL;
    }

    err = ft840_send_static_cmd(rig, cmd_index);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * ************************************
 *
 * Private functions to ft840 backend
 *
 * ************************************
 */


/*
 * Private helper function. Retrieves update data from rig.
 * using pacing value and buffer indicated in *priv struct.
 * Extended to be command agnostic as 840 has several ways to
 * get data and several ways to return it.
 *
 * Need to use this when doing ft840_get_* stuff
 *
 * Arguments:   *rig    Valid RIG instance
 *              ci      command index
 *              rl      expected length of returned data in octets
 *
 * Returns:     RIG_OK if all called functions are successful,
 *              otherwise returns error from called function
 */

static int ft840_get_update_data(RIG *rig, unsigned char ci, unsigned char rl)
{
    struct ft840_priv_data *priv;
    int n, err;                       /* for read_  */

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    err = ft840_send_static_cmd(rig, ci);

    if (err != RIG_OK)
    {
        return err;
    }

    n = read_block(RIGPORT(rig), priv->update_data, rl);

    if (n < 0)
    {
        return n;    /* die returning read_block error */
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: read %i bytes\n", __func__, n);

    return RIG_OK;
}


/*
 * Private helper function to send a complete command sequence.
 *
 * TODO: place variant of this in yaesu.c
 *
 * Arguments:   *rig    Valid RIG instance
 *              ci      Command index of the ncmd table
 *
 * Returns:     RIG_OK if all called functions are successful,
 *              otherwise returns error from called function
 */

static int ft840_send_static_cmd(RIG *rig, unsigned char ci)
{
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    if (!ncmd[ci].ncomp)
    {
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: Attempt to send incomplete sequence\n", __func__);
        return -RIG_EINVAL;
    }

    err = write_block(RIGPORT(rig), ncmd[ci].nseq,
                      YAESU_CMD_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * Private helper function to build and then send a complete command
 * sequence.
 *
 * TODO: place variant of this in yaesu.c
 *
 * Arguments:   *rig    Valid RIG instance
 *              ci      Command index of the ncmd table
 *              p1-p4   Command parameters
 *
 * Returns:     RIG_OK if all called functions are successful,
 *              otherwise returns error from called function
 */

static int ft840_send_dynamic_cmd(RIG *rig, unsigned char ci,
                                  unsigned char p1, unsigned char p2,
                                  unsigned char p3, unsigned char p4)
{
    struct ft840_priv_data *priv;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed ci = %i\n", __func__, ci);
    rig_debug(RIG_DEBUG_TRACE,
              "%s: passed p1 = 0x%02x, p2 = 0x%02x, p3 = 0x%02x, p4 = 0x%02x,\n",
              __func__, p1, p2, p3, p4);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (ncmd[ci].ncomp)
    {
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: Attempt to modify complete sequence\n", __func__);
        return -RIG_EINVAL;
    }

    memcpy(&priv->p_cmd, &ncmd[ci].nseq, YAESU_CMD_LENGTH);

    priv->p_cmd[P1] = p1;         /* ick */
    priv->p_cmd[P2] = p2;
    priv->p_cmd[P3] = p3;
    priv->p_cmd[P4] = p4;

    err = write_block(RIGPORT(rig), (unsigned char *) &priv->p_cmd,
                      YAESU_CMD_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * Private helper function to build and send a complete command to
 * change the display frequency.
 *
 * TODO: place variant of this in yaesu.c
 *
 * Arguments:   *rig    Valid RIG instance
 *              ci      Command index of the ncmd table
 *              freq    freq_t frequency value
 *
 * Returns:     RIG_OK if all called functions are successful,
 *              otherwise returns error from called function
 */

static int ft840_send_dial_freq(RIG *rig, unsigned char ci, freq_t freq)
{
    struct ft840_priv_data *priv;
    int err;
    // cppcheck-suppress *
    char *fmt = "%s: requested freq after conversion = %"PRIll" Hz\n";

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed ci = %i\n", __func__, ci);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed freq = %"PRIfreq" Hz\n", __func__, freq);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (ncmd[ci].ncomp)
    {
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: Attempt to modify complete sequence\n", __func__);
        return -RIG_EINVAL;
    }

    /* Copy native cmd freq_set to private cmd storage area */
    memcpy(&priv->p_cmd, &ncmd[ci].nseq, YAESU_CMD_LENGTH);

    /* store bcd format in in p_cmd */
    to_bcd(priv->p_cmd, freq / 10, FT840_BCD_DIAL);

    rig_debug(RIG_DEBUG_TRACE, fmt, __func__, (int64_t)from_bcd(priv->p_cmd,
              FT840_BCD_DIAL) * 10);

    err = write_block(RIGPORT(rig), (unsigned char *) &priv->p_cmd,
                      YAESU_CMD_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}


/*
 * Private helper function to build and send a complete command to
 * change the RIT frequency.
 *
 * TODO: place variant of this in yaesu.c
 *
 * Arguments:   *rig    Valid RIG instance
 *              ci      Command index of the ncmd table
 *              rit     shortfreq_t frequency value
 *              p1      P1 value -- CLAR_SET_FREQ
 *              p2      P2 value -- CLAR_OFFSET_PLUS || CLAR_OFFSET_MINUS
 *
 * Returns:     RIG_OK if all called functions are successful,
 *              otherwise returns error from called function
 *
 * Assumes:     rit doesn't exceed tuning limits of rig
 */

#ifdef USE_FT840_SET_RIT
static int ft840_send_rit_freq(RIG *rig, unsigned char ci, shortfreq_t rit)
{
    struct ft840_priv_data *priv;
    unsigned char p1;
    unsigned char p2;
    int err;

    rig_debug(RIG_DEBUG_VERBOSE, "%s called\n", __func__);

    if (!rig)
    {
        return -RIG_EINVAL;
    }

    rig_debug(RIG_DEBUG_TRACE, "%s: passed ci = %i\n", __func__, ci);
    rig_debug(RIG_DEBUG_TRACE, "%s: passed rit = %li Hz\n", __func__, rit);

    priv = (struct ft840_priv_data *)STATE(rig)->priv;

    if (ncmd[ci].ncomp)
    {
        rig_debug(RIG_DEBUG_TRACE,
                  "%s: Attempt to modify complete sequence\n", __func__);
        return -RIG_EINVAL;
    }

    p1 = CLAR_SET_FREQ;

    if (rit < 0)
    {
        rit = labs(rit);            /* get absolute value of rit */
        p2 = CLAR_OFFSET_MINUS;
    }
    else
    {
        p2 = CLAR_OFFSET_PLUS;
    }

    /* Copy native cmd clarifier ops to private cmd storage area */
    memcpy(&priv->p_cmd, &ncmd[ci].nseq, YAESU_CMD_LENGTH);

    /* store bcd format in in p_cmd */
    to_bcd(priv->p_cmd, rit / 10, FT840_BCD_RIT);

    rig_debug(RIG_DEBUG_TRACE,
              "%s: requested rit after conversion = %li Hz\n",
              __func__, from_bcd(priv->p_cmd, FT840_BCD_RIT) * 10);

    priv->p_cmd[P1] = p1;         /* ick */
    priv->p_cmd[P2] = p2;

    err = write_block(RIGPORT(rig), (char *) &priv->p_cmd,
                      YAESU_CMD_LENGTH);

    if (err != RIG_OK)
    {
        return err;
    }

    return RIG_OK;
}
#endif /* USE_FT840_SET_RIT */


