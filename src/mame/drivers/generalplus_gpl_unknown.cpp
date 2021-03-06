// license:BSD-3-Clause
// copyright-holders:David Haywood

#include "emu.h"

#include "cpu/unsp/unsp.h"

#include "emupal.h"
#include "screen.h"
#include "speaker.h"

#include "logmacro.h"


class generalplus_gpl_unknown_state : public driver_device
{
public:
	generalplus_gpl_unknown_state(const machine_config &mconfig, device_type type, const char *tag) :
		driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		//m_mainrom(*this, "maincpu"),
		//m_mainram(*this, "mainram"),
		m_palette(*this, "palette"),
		m_screen(*this, "screen"),
		m_spirom(*this, "spi")
	{ }
	
	void generalplus_gpl_unknown(machine_config &config);

private:
	virtual void machine_start() override;
	virtual void machine_reset() override;

	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);

	required_device<unsp_20_device> m_maincpu;
	//required_region_ptr<uint16_t> m_mainrom;
	//required_shared_ptr<uint16_t> m_mainram;

	required_device<palette_device> m_palette;
	required_device<screen_device> m_screen;
	required_region_ptr<uint8_t> m_spirom;

	void map(address_map &map);
};

uint32_t generalplus_gpl_unknown_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	return 0;
}


static INPUT_PORTS_START( generalplus_gpl_unknown )
INPUT_PORTS_END


void generalplus_gpl_unknown_state::map(address_map &map)
{
	map(0x000000, 0x00ffff).ram(); // unknown, RAM/ROM? there are jumps to this region, doesn't seem the usual unSP 8000-ffff ROM mapping

	// there are calls to 0x0f000 (internal ROM?)
	map(0x200000, 0x3fffff).ram(); // almost certainly RAM where the SPI ROM gets copied
}


void generalplus_gpl_unknown_state::machine_start()
{
}

void generalplus_gpl_unknown_state::machine_reset()
{
	address_space& mem = m_maincpu->space(AS_PROGRAM);
	for (int j = 0; j < 0x200000/2; j++)
	{
		uint16_t data = (m_spirom[(j*2)+1] << 8) | (m_spirom[(j*2)+0]);
		mem.write_word(0x200000+j, data);
	}

	m_maincpu->set_state_int(UNSP_PC, 0x1000); // this is not the boot code, everything in the SPI ROM seems like relatively small functions (that never touch internal registers) only the very last one in SPI ROM has no callers from in the SPI ROM code
	m_maincpu->set_state_int(UNSP_SR, 0x0020);
}

void generalplus_gpl_unknown_state::generalplus_gpl_unknown(machine_config &config)
{

	UNSP_20(config, m_maincpu, 96000000); // unknown CPU, no unSP2.0 opcodes?
	m_maincpu->set_addrmap(AS_PROGRAM, &generalplus_gpl_unknown_state::map);

	SCREEN(config, m_screen, SCREEN_TYPE_RASTER);
	m_screen->set_refresh_hz(60);
	m_screen->set_vblank_time(ATTOSECONDS_IN_USEC(10));
	m_screen->set_size(64*8, 32*8);
	m_screen->set_visarea(0*8, 320-1, 0*8, 240-1);
	m_screen->set_screen_update(FUNC(generalplus_gpl_unknown_state::screen_update));
	//m_screen->screen_vblank().set(FUNC(generalplus_gpl_unknown_state::screen_vblank));

	PALETTE(config, m_palette).set_format(palette_device::xBGR_555, 0x8000);
}

ROM_START( unkunsp )
	//ROM_REGION( 0x2000, "maincpu", ROMREGION_ERASEFF )
	//ROM_LOAD16_WORD_SWAP( "internal.rom", 0x000000, 0x2000, NO_DUMP ) // exact size unknown

	ROM_REGION( 0x800000, "spi", ROMREGION_ERASEFF ) // this simply maps at 200000, and contains jumps to lower areas that suggest even the ROM/internal RAM mapping is very different
	ROM_LOAD( "fm25q16a.bin", 0x000000, 0x200000, CRC(aeb472ac) SHA1(500c24b725f6d3308ef8cbdf4259f5be556c7c92) )
ROM_END

// different hardware, first 0x2000 bytes in ROM are blank, maps fully in RAM at 200000
CONS( 2017, unkunsp,      0,       0,      generalplus_gpl_unknown,   generalplus_gpl_unknown, generalplus_gpl_unknown_state, empty_init, "Super Impulse", "Pac-Man (Micro Arcade)", MACHINE_IS_SKELETON )
