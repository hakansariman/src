/*	$OpenBSD: wss.c,v 1.23 2008/04/21 00:32:42 jakemsr Exp $	*/
/*	$NetBSD: wss.c,v 1.42 1998/01/19 22:18:23 augustss Exp $	*/

/*
 * Copyright (c) 1994 John Brezak
 * Copyright (c) 1991-1993 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/buf.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <sys/audioio.h>
#include <dev/audio_if.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>

#include <dev/ic/ad1848reg.h>
#include <dev/isa/ad1848var.h>
#include <dev/isa/wssreg.h>
#include <dev/isa/wssvar.h>
#include <dev/isa/madreg.h>

#ifdef AUDIO_DEBUG
#define DPRINTF(x)	if (wssdebug) printf x
int	wssdebug = 0;
#else
#define DPRINTF(x)
#endif

struct audio_device wss_device = {
	"wss,ad1848",
	"",
	"WSS"
};

int	wss_getdev(void *, struct audio_device *);

int	wss_mixer_set_port(void *, mixer_ctrl_t *);
int	wss_mixer_get_port(void *, mixer_ctrl_t *);
int	wss_query_devinfo(void *, mixer_devinfo_t *);

/*
 * Define our interface to the higher level audio driver.
 */

struct audio_hw_if wss_hw_if = {
	ad1848_open,
	ad1848_close,
	NULL,
	ad1848_query_encoding,
	ad1848_set_params,
	ad1848_round_blocksize,
	ad1848_commit_settings,
	ad1848_dma_init_output,
	ad1848_dma_init_input,
	ad1848_dma_output,
	ad1848_dma_input,
	ad1848_halt_out_dma,
	ad1848_halt_in_dma,
	NULL,
	wss_getdev,
	NULL,
	wss_mixer_set_port,
	wss_mixer_get_port,
	wss_query_devinfo,
	ad1848_malloc,
	ad1848_free,
	ad1848_round,
	ad1848_mappage,
	ad1848_get_props,
	NULL,
	NULL,
	NULL
};

/*
 * Attach hardware to driver, attach hardware driver to audio
 * pseudo-device driver .
 */
void
wssattach(sc)
    struct wss_softc *sc;
{
    int version;
    
    madattach(sc);

    sc->sc_ih = isa_intr_establish(sc->sc_ic, sc->wss_irq, IST_EDGE, IPL_AUDIO,
        ad1848_intr, &sc->sc_ad1848, sc->sc_dev.dv_xname);

    ad1848_attach(&sc->sc_ad1848);
    
    version = bus_space_read_1(sc->sc_iot, sc->sc_ioh, WSS_STATUS) & WSS_VERSMASK;
    printf(" (vers %d)", version);
    switch(sc->mad_chip_type) {
    case MAD_82C928:
	printf(", 82C928");
	break;
    case MAD_OTI601D:
	printf(", OTI-601D");
	break;
    case MAD_82C929:
	printf(", 82C929");
	break;
    case MAD_82C931:
	printf(", 82C931");
	break;
    default:
	break;
    }
    printf("\n");

    sc->sc_ad1848.parent = sc;

    audio_attach_mi(&wss_hw_if, &sc->sc_ad1848, &sc->sc_dev);
}

int
wss_getdev(addr, retp)
    void *addr;
    struct audio_device *retp;
{
    *retp = wss_device;
    return 0;
}

static ad1848_devmap_t mappings[] = {
{ WSS_MIC_IN_LVL, AD1848_KIND_LVL, AD1848_AUX2_CHANNEL },
{ WSS_LINE_IN_LVL, AD1848_KIND_LVL, AD1848_AUX1_CHANNEL },
{ WSS_DAC_LVL, AD1848_KIND_LVL, AD1848_DAC_CHANNEL },
{ WSS_MON_LVL, AD1848_KIND_LVL, AD1848_MONO_CHANNEL },
{ WSS_MIC_IN_MUTE, AD1848_KIND_MUTE, AD1848_AUX2_CHANNEL },
{ WSS_LINE_IN_MUTE, AD1848_KIND_MUTE, AD1848_AUX1_CHANNEL },
{ WSS_DAC_MUTE, AD1848_KIND_MUTE, AD1848_DAC_CHANNEL },
{ WSS_REC_LVL, AD1848_KIND_RECORDGAIN, -1 },
{ WSS_RECORD_SOURCE, AD1848_KIND_RECORDSOURCE, -1}
};

static int nummap = sizeof(mappings) / sizeof(mappings[0]);

int
wss_mixer_set_port(addr, cp)
    void *addr;
    mixer_ctrl_t *cp;
{
    struct ad1848_softc *ac = addr;

    return (ad1848_mixer_set_port(ac, mappings, nummap, cp));
}

int
wss_mixer_get_port(addr, cp)
    void *addr;
    mixer_ctrl_t *cp;
{
    struct ad1848_softc *ac = addr;

    return (ad1848_mixer_get_port(ac, mappings, nummap, cp));
}

int
wss_query_devinfo(addr, dip)
    void *addr;
    mixer_devinfo_t *dip;
{
    DPRINTF(("wss_query_devinfo: index=%d\n", dip->index));

    switch(dip->index) {
    case WSS_MIC_IN_LVL:	/* Microphone */
	dip->type = AUDIO_MIXER_VALUE;
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->prev = AUDIO_MIXER_LAST;
	dip->next = WSS_MIC_IN_MUTE;
	strlcpy(dip->label.name, AudioNmicrophone, sizeof dip->label.name);
	dip->un.v.num_channels = 2;
	strlcpy(dip->un.v.units.name, AudioNvolume, sizeof dip->un.v.units.name);
	break;

    case WSS_LINE_IN_LVL:	/* line/CD */
	dip->type = AUDIO_MIXER_VALUE;
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->prev = AUDIO_MIXER_LAST;
	dip->next = WSS_LINE_IN_MUTE;
	strlcpy(dip->label.name, AudioNcd, sizeof dip->label.name);
	dip->un.v.num_channels = 2;
	strlcpy(dip->un.v.units.name, AudioNvolume, sizeof dip->un.v.units.name);
	break;

    case WSS_DAC_LVL:		/*  dacout */
	dip->type = AUDIO_MIXER_VALUE;
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->prev = AUDIO_MIXER_LAST;
	dip->next = WSS_DAC_MUTE;
	strlcpy(dip->label.name, AudioNdac, sizeof dip->label.name);
	dip->un.v.num_channels = 2;
	strlcpy(dip->un.v.units.name, AudioNvolume, sizeof dip->un.v.units.name);
	break;

    case WSS_REC_LVL:	/* record level */
	dip->type = AUDIO_MIXER_VALUE;
	dip->mixer_class = WSS_RECORD_CLASS;
	dip->prev = AUDIO_MIXER_LAST;
	dip->next = WSS_RECORD_SOURCE;
	strlcpy(dip->label.name, AudioNrecord, sizeof dip->label.name);
	dip->un.v.num_channels = 2;
	strlcpy(dip->un.v.units.name, AudioNvolume, sizeof dip->un.v.units.name);
	break;

    case WSS_MON_LVL:	/* monitor level */
	dip->type = AUDIO_MIXER_VALUE;
	dip->mixer_class = WSS_MONITOR_CLASS;
	dip->next = dip->prev = AUDIO_MIXER_LAST;
	strlcpy(dip->label.name, AudioNmonitor, sizeof dip->label.name);
	dip->un.v.num_channels = 1;
	strlcpy(dip->un.v.units.name, AudioNvolume, sizeof dip->un.v.units.name);
	break;

    case WSS_INPUT_CLASS:			/* input class descriptor */
	dip->type = AUDIO_MIXER_CLASS;
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->next = dip->prev = AUDIO_MIXER_LAST;
	strlcpy(dip->label.name, AudioCinputs, sizeof dip->label.name);
	break;

    case WSS_MONITOR_CLASS:			/* monitor class descriptor */
	dip->type = AUDIO_MIXER_CLASS;
	dip->mixer_class = WSS_MONITOR_CLASS;
	dip->next = dip->prev = AUDIO_MIXER_LAST;
	strlcpy(dip->label.name, AudioCmonitor, sizeof dip->label.name);
	break;
	    
    case WSS_RECORD_CLASS:			/* record source class */
	dip->type = AUDIO_MIXER_CLASS;
	dip->mixer_class = WSS_RECORD_CLASS;
	dip->next = dip->prev = AUDIO_MIXER_LAST;
	strlcpy(dip->label.name, AudioCrecord, sizeof dip->label.name);
	break;
	
    case WSS_MIC_IN_MUTE:
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->type = AUDIO_MIXER_ENUM;
	dip->prev = WSS_MIC_IN_LVL;
	dip->next = AUDIO_MIXER_LAST;
	goto mute;
	
    case WSS_LINE_IN_MUTE:
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->type = AUDIO_MIXER_ENUM;
	dip->prev = WSS_LINE_IN_LVL;
	dip->next = AUDIO_MIXER_LAST;
	goto mute;
	
    case WSS_DAC_MUTE:
	dip->mixer_class = WSS_INPUT_CLASS;
	dip->type = AUDIO_MIXER_ENUM;
	dip->prev = WSS_DAC_LVL;
	dip->next = AUDIO_MIXER_LAST;
    mute:
	strlcpy(dip->label.name, AudioNmute, sizeof dip->label.name);
	dip->un.e.num_mem = 2;
	strlcpy(dip->un.e.member[0].label.name, AudioNoff,
	    sizeof dip->un.e.member[0].label.name);
	dip->un.e.member[0].ord = 0;
	strlcpy(dip->un.e.member[1].label.name, AudioNon,
	    sizeof dip->un.e.member[1].label.name);
	dip->un.e.member[1].ord = 1;
	break;

    case WSS_RECORD_SOURCE:
	dip->mixer_class = WSS_RECORD_CLASS;
	dip->type = AUDIO_MIXER_ENUM;
	dip->prev = WSS_REC_LVL;
	dip->next = AUDIO_MIXER_LAST;
	strlcpy(dip->label.name, AudioNsource, sizeof dip->label.name);
	dip->un.e.num_mem = 3;
	strlcpy(dip->un.e.member[0].label.name, AudioNmicrophone,
	    sizeof dip->un.e.member[0].label.name);
	dip->un.e.member[0].ord = WSS_MIC_IN_LVL;
	strlcpy(dip->un.e.member[1].label.name, AudioNcd,
	    sizeof dip->un.e.member[1].label.name);
	dip->un.e.member[1].ord = WSS_LINE_IN_LVL;
	strlcpy(dip->un.e.member[2].label.name, AudioNdac,
	    sizeof dip->un.e.member[2].label.name);
	dip->un.e.member[2].ord = WSS_DAC_LVL;
	break;

    default:
	return ENXIO;
	/*NOTREACHED*/
    }
    DPRINTF(("AUDIO_MIXER_DEVINFO: name=%s\n", dip->label.name));

    return 0;
}


/*
 * Copyright by Hannu Savolainen 1994
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/*
 * Initialization code for OPTi MAD16 compatible audio chips. Including
 *
 *      OPTi 82C928     MAD16           (replaced by C929)
 *      OAK OTI-601D    Mozart
 *      OPTi 82C929     MAD16 Pro
 *
 */

u_int
mad_read(sc, port)
    struct wss_softc *sc;
    int port;
{
    u_int tmp;
    int pwd;
    int s;
    
    switch (sc->mad_chip_type) {	/* Output password */
    case MAD_82C928:
    case MAD_OTI601D:
	pwd = M_PASSWD_928;
	break;
    case MAD_82C929:
	pwd = M_PASSWD_929;
	break;
    case MAD_82C931:
	pwd = M_PASSWD_931;
	break;
    default:
	panic("mad_read: Bad chip type=%d", sc->mad_chip_type);
    }
    s = splaudio();		/* don't want an interrupt between outb&inb */
    bus_space_write_1(sc->sc_iot, sc->mad_ioh, MC_PASSWD_REG, pwd);
    tmp = bus_space_read_1(sc->sc_iot, sc->mad_ioh, port);
    splx(s);
    return tmp;
}

void
mad_write(sc, port, value)
    struct wss_softc *sc;
    int port;
    int value;
{
    int pwd;
    int s;

    switch (sc->mad_chip_type) {	/* Output password */
    case MAD_82C928:
    case MAD_OTI601D:
	pwd = M_PASSWD_928;
	break;
    case MAD_82C929:
	pwd = M_PASSWD_929;
	break;
    case MAD_82C931:
	pwd = M_PASSWD_931;
	break;
    default:
	panic("mad_write: Bad chip type=%d", sc->mad_chip_type);
    }
    s = splaudio();
    bus_space_write_1(sc->sc_iot, sc->mad_ioh, MC_PASSWD_REG, pwd);
    bus_space_write_1(sc->sc_iot, sc->mad_ioh, port, value & 0xff);
    splx(s);
}

void
madattach(sc)
    struct wss_softc *sc;
{
    unsigned char cs4231_mode;
    int joy;

    if (sc->mad_chip_type == MAD_NONE)
        return;

    /* Do we want the joystick disabled? */
    joy = sc->sc_dev.dv_cfdata->cf_flags & 2 ? MC1_JOYDISABLE : 0;

    /* enable WSS emulation at the I/O port */
    mad_write(sc, MC1_PORT, M_WSS_PORT_SELECT(sc->mad_ioindex) | joy);
    mad_write(sc, MC2_PORT, 0x03); /* ? */
    mad_write(sc, MC3_PORT, 0xf0); /* Disable SB */

    cs4231_mode = 
	strncmp(sc->sc_ad1848.chip_name, "CS4248", 6) == 0 ||
	strncmp(sc->sc_ad1848.chip_name, "CS4231", 6) == 0 ? 0x02 : 0;

    if (sc->mad_chip_type == MAD_82C929) {
	mad_write(sc, MC4_PORT, 0x92);
	mad_write(sc, MC5_PORT, 0xA5 | cs4231_mode);
	mad_write(sc, MC6_PORT, 0x03);	/* Disable MPU401 */
    } else {
	mad_write(sc, MC4_PORT, 0x02);
	mad_write(sc, MC5_PORT, 0x30 | cs4231_mode);
    }

#ifdef AUDIO_DEBUG
    if (wssdebug) {
	int i;
	for (i = MC1_PORT; i <= MC7_PORT; i++)
	    DPRINTF(("port %03x after init = %02x\n", i, mad_read(sc, i)));
    }
#endif
}
