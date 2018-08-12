// license:BSD-3-Clause
// copyright-holders:Dan Boris, Aaron Giles
/***************************************************************************

    Atari Return of the Jedi hardware

    driver by Dan Boris

    Games supported:
        * Return of the Jedi

    Notes:
        * The schematics show the smoothing PROMs as being twice as large,
          but the current sizes are confirmed via a PCB.  The PROMs
          are 82S137 devices.

****************************************************************************

    Memory map

****************************************************************************

    ========================================================================
    CPU #1
    ========================================================================
    0000-07FF   R/W   xxxxxxxx    Z-page Working RAM
    0800-08FF   R/W   xxxxxxxx    NVRAM
    0C00        R     xxxx-xxx    Switch inputs #1
                R     x-------       (right coin)
                R     -x------       (left coin)
                R     --x-----       (aux coin)
                R     ---x----       (self test)
                R     -----x--       (left thumb switch)
                R     ------x-       (fire switches)
                R     -------x       (right thumb switch)
    0C01        R     xxx--x--    Communications
                R     x-------       (VBLANK)
                R     -x------       (sound CPU communications latch full flag)
                R     --x-----       (sound CPU acknowledge latch flag)
                R     -----x--       (slam switch)
    1400        R     xxxxxxxx    Sound acknowledge latch
    1800        R     xxxxxxxx    Read A/D conversion
    1C00          W   --------    Enable NVRAM
    1C01          W   --------    Disable NVRAM
    1C80          W   --------    Start A/D conversion (horizontal)
    1C82          W   --------    Start A/D conversion (vertical)
    1D00          W   --------    NVRAM store
    1D80          W   --------    Watchdog clear
    1E00          W   --------    Interrupt acknowledge
    1E80          W   x-------    Left coin counter
    1E81          W   x-------    Right coin counter
    1E82          W   x-------    LED 1 (not used)
    1E83          W   x-------    LED 2 (not used)
    1E84          W   x-------    Alphanumerics bank select
    1E86          W   x-------    Sound CPU reset
    1E87          W   x-------    Video off
    1F00          W   xxxxxxxx    Sound communications latch
    1F80          W   -----xxx    Program ROM bank select
    2000-23FF   R/W   xxxxxxxx    Scrolling playfield (low 8 bits)
    2400-27FF   R/W   ----xxxx    Scrolling playfield (upper 4 bits)
    2800-2BFF   R/W   xxxxxxxx    Color RAM low
                R/W   -----xxx       (blue)
                R/W   --xxx---       (green)
                R/W   xx------       (red LSBs)
    2C00-2FFF   R/W   ----xxxx    Color RAM high
                R/W   -------x       (red MSB)
                R/W   ----xxx-       (intensity)
    3000-37BF   R/W   xxxxxxxx    Alphanumerics RAM
    37C0-37EF   R/W   xxxxxxxx    Motion object picture
    3800-382F   R/W   -xxxxxxx    Motion object flags
                R/W   -x---xx-       (picture bank)
                R/W   --x-----       (vertical flip)
                R/W   ---x----       (horizontal flip)
                R/W   ----x---       (32 pixels tall)
                R/W   -------x       (X position MSB)
    3840-386F   R/W   xxxxxxxx       (Y position)
    38C0-38EF   R/W   xxxxxxxx       (X position LSBs)
    3C00-3C01     W   xxxxxxxx    Scrolling playfield vertical position
    3D00-3D01     W   xxxxxxxx    Scrolling playfield horizontal position
    3E00-3FFF     W   xxxxxxxx    PIXI graphics expander RAM
    4000-7FFF   R     xxxxxxxx    Banked program ROM
    8000-FFFF   R     xxxxxxxx    Fixed program ROM
    ========================================================================
    Interrupts:
        NMI not connected
        IRQ generated by 32V
    ========================================================================


    ========================================================================
    CPU #2
    ========================================================================
    0000-07FF   R/W   xxxxxxxx    Z-page working RAM
    0800-083F   R/W   xxxxxxxx    Custom I/O
    1000          W   --------    Interrupt acknowledge
    1100          W   xxxxxxxx    Speech data
    1200          W   --------    Speech write strobe on
    1300          W   --------    Speech write strobe off
    1400          W   xxxxxxxx    Main CPU acknowledge latch
    1500          W   -------x    Speech chip reset
    1800        R     xxxxxxxx    Main CPU communication latch
    1C00        R     x-------    Speech chip ready
    1C01        R     xx------    Communications
                R     x-------       (sound CPU communication latch full flag)
                R     -x------       (sound CPU acknowledge latch full flag)
    8000-FFFF   R     xxxxxxxx    Program ROM
    ========================================================================
    Interrupts:
        NMI not connected
        IRQ generated by 32V
    ========================================================================

***************************************************************************/

#include "emu.h"
#include "cpu/m6502/m6502.h"
#include "machine/74259.h"
#include "machine/adc0808.h"
#include "machine/nvram.h"
#include "machine/watchdog.h"
#include "includes/jedi.h"



/*************************************
 *
 *  Interrupt handling
 *
 *************************************/

TIMER_CALLBACK_MEMBER(jedi_state::generate_interrupt)
{
	int scanline = param;

	/* IRQ is set by /32V */
	m_maincpu->set_input_line(M6502_IRQ_LINE, (scanline & 32) ? CLEAR_LINE : ASSERT_LINE);
	m_audiocpu->set_input_line(M6502_IRQ_LINE, (scanline & 32) ? CLEAR_LINE : ASSERT_LINE);

	/* set up for the next */
	scanline += 32;
	if (scanline > 256)
		scanline = 32;
	m_interrupt_timer->adjust(m_screen->time_until_pos(scanline), scanline);
}


WRITE8_MEMBER(jedi_state::main_irq_ack_w)
{
	m_maincpu->set_input_line(M6502_IRQ_LINE, CLEAR_LINE);
}



/*************************************
 *
 *  Start
 *
 *************************************/

void jedi_state::machine_start()
{
	/* set a timer to run the interrupts */
	m_interrupt_timer = machine().scheduler().timer_alloc(timer_expired_delegate(FUNC(jedi_state::generate_interrupt),this));
	m_interrupt_timer->adjust(m_screen->time_until_pos(32), 32);

	/* configure the banks */
	membank("bank1")->configure_entries(0, 3, memregion("maincpu")->base() + 0x10000, 0x4000);
}



/*************************************
 *
 *  Reset
 *
 *************************************/

void jedi_state::machine_reset()
{
	m_novram[0]->recall(1);
	m_novram[1]->recall(1);
}



/*************************************
 *
 *  Main program ROM banking
 *
 *************************************/

WRITE8_MEMBER(jedi_state::rom_banksel_w)
{
	if (data & 0x01) membank("bank1")->set_entry(0);
	if (data & 0x02) membank("bank1")->set_entry(1);
	if (data & 0x04) membank("bank1")->set_entry(2);
}



/*************************************
 *
 *  I/O ports
 *
 *************************************/

WRITE_LINE_MEMBER(jedi_state::coin_counter_left_w)
{
	machine().bookkeeping().coin_counter_w(0, state);
}


WRITE_LINE_MEMBER(jedi_state::coin_counter_right_w)
{
	machine().bookkeeping().coin_counter_w(1, state);
}



/*************************************
 *
 *  X2212-30 NOVRAM
 *
 *************************************/

READ8_MEMBER(jedi_state::novram_data_r)
{
	return (m_novram[0]->read(space, offset) & 0x0f) | (m_novram[1]->read(space, offset) << 4);
}


WRITE8_MEMBER(jedi_state::novram_data_w)
{
	m_novram[0]->write(space, offset, data & 0x0f);
	m_novram[1]->write(space, offset, data >> 4);
}


WRITE8_MEMBER(jedi_state::novram_recall_w)
{
	m_novram[0]->recall(BIT(offset, 0));
	m_novram[1]->recall(BIT(offset, 0));
}


WRITE8_MEMBER(jedi_state::novram_store_w)
{
	m_novram[0]->store(1);
	m_novram[1]->store(1);
	m_novram[0]->store(0);
	m_novram[1]->store(0);
}



/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

void jedi_state::main_map(address_map &map)
{
	map(0x0000, 0x07ff).ram();
	map(0x0800, 0x08ff).mirror(0x0300).rw(FUNC(jedi_state::novram_data_r), FUNC(jedi_state::novram_data_w));
	map(0x0c00, 0x0c00).mirror(0x03fe).portr("0c00").nopw();
	map(0x0c01, 0x0c01).mirror(0x03fe).portr("0c01").nopw();
	map(0x1000, 0x13ff).noprw();
	map(0x1400, 0x1400).mirror(0x03ff).r("sacklatch", FUNC(generic_latch_8_device::read)).nopw();
	map(0x1800, 0x1800).mirror(0x03ff).r("adc", FUNC(adc0808_device::data_r)).nopw();
	map(0x1c00, 0x1c01).mirror(0x007e).nopr().w(FUNC(jedi_state::novram_recall_w));
	map(0x1c80, 0x1c87).mirror(0x0078).nopr().w("adc", FUNC(adc0808_device::address_offset_start_w));
	map(0x1d00, 0x1d00).mirror(0x007f).nopr().w(FUNC(jedi_state::novram_store_w));
	map(0x1d80, 0x1d80).mirror(0x007f).nopr().w("watchdog", FUNC(watchdog_timer_device::reset_w));
	map(0x1e00, 0x1e00).mirror(0x007f).nopr().w(FUNC(jedi_state::main_irq_ack_w));
	map(0x1e80, 0x1e87).mirror(0x0078).nopr().w("outlatch", FUNC(ls259_device::write_d7));
	map(0x1f00, 0x1f00).mirror(0x007f).nopr().w("soundlatch", FUNC(generic_latch_8_device::write));
	map(0x1f80, 0x1f80).mirror(0x007f).nopr().w(FUNC(jedi_state::rom_banksel_w));
	map(0x2000, 0x27ff).ram().share("backgroundram");
	map(0x2800, 0x2fff).ram().share("paletteram");
	map(0x3000, 0x37bf).ram().share("foregroundram");
	map(0x37c0, 0x3bff).ram().share("spriteram");
	map(0x3c00, 0x3c01).mirror(0x00fe).nopr().w(FUNC(jedi_state::jedi_vscroll_w));
	map(0x3d00, 0x3d01).mirror(0x00fe).nopr().w(FUNC(jedi_state::jedi_hscroll_w));
	map(0x3e00, 0x3e00).mirror(0x01ff).writeonly().share("smoothing_table");
	map(0x4000, 0x7fff).bankr("bank1");
	map(0x8000, 0xffff).rom();
}



/*************************************
 *
 *  Port definitions
 *
 *************************************/

static INPUT_PORTS_START( jedi )
	PORT_START("0c00")
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_SERVICE( 0x10, IP_ACTIVE_LOW )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_SERVICE1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_COIN2 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_COIN1 )

	PORT_START("0c01")
	PORT_BIT( 0x03, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_TILT )
	PORT_BIT( 0x18, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x60, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_CUSTOM_MEMBER(DEVICE_SELF,jedi_state,jedi_audio_comm_stat_r, nullptr)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_CUSTOM ) PORT_VBLANK("screen")

	PORT_START("STICKY")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START("STICKX")
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)
INPUT_PORTS_END



/*************************************
 *
 *  Machine driver
 *
 *************************************/

MACHINE_CONFIG_START(jedi_state::jedi)

	/* basic machine hardware */
	MCFG_DEVICE_ADD("maincpu", M6502, JEDI_MAIN_CPU_CLOCK)
	MCFG_DEVICE_PROGRAM_MAP(main_map)

	MCFG_QUANTUM_TIME(attotime::from_hz(240))

	X2212(config, "novram12b");
	X2212(config, "novram12c");

	adc0809_device &adc(ADC0809(config, "adc", JEDI_AUDIO_CPU_OSC / 2 / 9)); // nominally 666 kHz
	adc.in_callback<0>().set_ioport("STICKY");
	adc.in_callback<1>().set_constant(0); // SPARE
	adc.in_callback<2>().set_ioport("STICKX");
	adc.in_callback<3>().set_constant(0); // SPARE

	ls259_device &outlatch(LS259(config, "outlatch")); // 14J
	outlatch.q_out_cb<0>().set(FUNC(jedi_state::coin_counter_left_w));
	outlatch.q_out_cb<1>().set(FUNC(jedi_state::coin_counter_right_w));
	outlatch.q_out_cb<2>().set_nop(); // LED control - not used
	outlatch.q_out_cb<3>().set_nop(); // LED control - not used
	outlatch.q_out_cb<4>().set(FUNC(jedi_state::foreground_bank_w));
	outlatch.q_out_cb<6>().set(FUNC(jedi_state::audio_reset_w));
	outlatch.q_out_cb<7>().set(FUNC(jedi_state::video_off_w));

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	jedi_video(config);

	/* audio hardware */
	jedi_audio(config);
MACHINE_CONFIG_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( jedi )
	ROM_REGION( 0x1C000, "maincpu", 0 ) /* 64k for code + 48k for banked ROMs */
	ROM_LOAD( "136030-221.14f",  0x08000, 0x4000, CRC(414d05e3) SHA1(e5f5f8d85433467a13d6ca9e3889e07a62b00e52) )
	ROM_LOAD( "136030-222.13f",  0x0c000, 0x4000, CRC(7b3f21be) SHA1(8fe62401f9b78c7a3e62b544c4b705b1bfa9b8f3) )
	ROM_LOAD( "136030-123.13d",  0x10000, 0x4000, CRC(877f554a) SHA1(8b51109cabd84741b024052f892b3172fbe83223) ) /* Page 0 */
	ROM_LOAD( "136030-124.13b",  0x14000, 0x4000, CRC(e72d41db) SHA1(1b3fcdc435f1e470e8d5b7241856e398a4c3910e) ) /* Page 1 */
	ROM_LOAD( "136030-122.13a",  0x18000, 0x4000, CRC(cce7ced5) SHA1(bff031a637aefca713355dbf251dcb5c2cea0885) ) /* Page 2 */

	ROM_REGION( 0x10000, "audiocpu", 0 )    /* space for the sound ROMs */
	ROM_LOAD( "136030-133.01c",  0x8000, 0x4000, CRC(6c601c69) SHA1(618b77800bbbb4db34a53ca974a71bdaf89b5930) )
	ROM_LOAD( "136030-134.01a",  0xC000, 0x4000, CRC(5e36c564) SHA1(4b0afceb9a1d912f1d5c1f26928d244d5b14ea4a) )

	ROM_REGION( 0x02000, "gfx1",0 )
	ROM_LOAD( "136030-215.11t",  0x00000, 0x2000, CRC(3e49491f) SHA1(ade5e846069c2fa6edf667469d13ce5a6a45c06d) ) /* Alphanumeric */

	ROM_REGION( 0x10000, "gfx2",0 )
	ROM_LOAD( "136030-126.06r",  0x00000, 0x8000, CRC(9c55ece8) SHA1(b8faa23314bb0d199ef46199bfabd9cb17510dd3) ) /* Playfield */
	ROM_LOAD( "136030-127.06n",  0x08000, 0x8000, CRC(4b09dcc5) SHA1(d46b5f4fb69c4b8d823dd9c4d92f8713badfa44a) )

	ROM_REGION( 0x20000, "gfx3", 0 )
	ROM_LOAD( "136030-130.01h",  0x00000, 0x8000, CRC(2646a793) SHA1(dcb5fd50eafbb27565bce099a884be83a9d82285) ) /* Sprites */
	ROM_LOAD( "136030-131.01f",  0x08000, 0x8000, CRC(60107350) SHA1(ded03a46996d3f2349df7f59fd435a7ad6ed465e) )
	ROM_LOAD( "136030-128.01m",  0x10000, 0x8000, CRC(24663184) SHA1(5eba142ed926671ee131430944e59f21a55a5c57) )
	ROM_LOAD( "136030-129.01k",  0x18000, 0x8000, CRC(ac86b98c) SHA1(9f86c8801a7293fa46e9432f1651dd85bf00f4b9) )

	ROM_REGION( 0x1000, "proms", 0 )    /* background smoothing */
	ROM_LOAD( "136030-117.bin",   0x0000, 0x0400, CRC(9831bd55) SHA1(12945ef2d1582914125b9ee591567034d71d6573) )
	ROM_LOAD( "136030-118.bin",   0x0800, 0x0400, CRC(261fbfe7) SHA1(efc65a74a3718563a07b718e34d8a7aa23339a69) )

	// NOVRAM default fills are specified to guarantee an invalid checksum
	ROM_REGION( 0x100, "novram12b", 0 )
	ROM_FILL( 0x00, 0x100, 0x0f )
	ROM_FILL( 0x50, 0x010, 0x00 )
	ROM_REGION( 0x100, "novram12c", 0 )
	ROM_FILL( 0x00, 0x100, 0x0f )
	ROM_FILL( 0x50, 0x010, 0x00 )
ROM_END



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1984, jedi, 0, jedi, jedi, jedi_state, empty_init, ROT0, "Atari", "Return of the Jedi", MACHINE_SUPPORTS_SAVE )
