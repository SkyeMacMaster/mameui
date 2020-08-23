-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   netlist.lua
--
--   Rules for building netlist cores
--
---------------------------------------------------------------------------

project "netlist"
	uuid "665ef8ac-2a4c-4c3e-a05f-fd1e5db11de9"
	kind (LIBTYPE)

	if _OPTIONS["targetos"]=="windows" then
		configuration { "mingw* or vs*" }
		defines {
			"UNICODE",
			"_UNICODE",
			"_WIN32_WINNT=0x0501",
			"WIN32_LEAN_AND_MEAN",
			"NOMINMAX",
		}
	end

	addprojectflags()

	defines {
		"__STDC_CONSTANT_MACROS",
		"NL_USE_ACADEMIC_SOLVERS=0",
	}

	includedirs {
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/netlist",
	}

	files {
		MAME_DIR .. "src/lib/netlist/nl_base.cpp",
		MAME_DIR .. "src/lib/netlist/nl_base.h",
		MAME_DIR .. "src/lib/netlist/nl_config.h",
		MAME_DIR .. "src/lib/netlist/nl_errstr.h",
		MAME_DIR .. "src/lib/netlist/nl_dice_compat.h",
		MAME_DIR .. "src/lib/netlist/nl_factory.cpp",
		MAME_DIR .. "src/lib/netlist/nl_factory.h",
		MAME_DIR .. "src/lib/netlist/nl_interface.h",
		MAME_DIR .. "src/lib/netlist/nl_parser.cpp",
		MAME_DIR .. "src/lib/netlist/nl_parser.h",
		MAME_DIR .. "src/lib/netlist/nl_setup.cpp",
		MAME_DIR .. "src/lib/netlist/nl_setup.h",
		MAME_DIR .. "src/lib/netlist/nl_types.h",
		MAME_DIR .. "src/lib/netlist/core/analog.h",
		MAME_DIR .. "src/lib/netlist/core/base_objects.h",
		MAME_DIR .. "src/lib/netlist/core/core_device.h",
		MAME_DIR .. "src/lib/netlist/core/device.h",
		MAME_DIR .. "src/lib/netlist/core/device_macros.h",
		MAME_DIR .. "src/lib/netlist/core/devices.h",
		MAME_DIR .. "src/lib/netlist/core/exec.h",
		MAME_DIR .. "src/lib/netlist/core/logic_family.h",
		MAME_DIR .. "src/lib/netlist/core/logic.h",
		MAME_DIR .. "src/lib/netlist/core/nets.h",
		MAME_DIR .. "src/lib/netlist/core/object_array.h",
		MAME_DIR .. "src/lib/netlist/core/param.h",
		MAME_DIR .. "src/lib/netlist/core/setup.h",
		MAME_DIR .. "src/lib/netlist/core/state_var.h",
		MAME_DIR .. "src/lib/netlist/plib/pconfig.h",
		MAME_DIR .. "src/lib/netlist/plib/palloc.h",
		MAME_DIR .. "src/lib/netlist/plib/pchrono.h",
		MAME_DIR .. "src/lib/netlist/plib/pgsl.h",
		MAME_DIR .. "src/lib/netlist/plib/penum.h",
		MAME_DIR .. "src/lib/netlist/plib/pexception.cpp",
		MAME_DIR .. "src/lib/netlist/plib/pexception.h",
		MAME_DIR .. "src/lib/netlist/plib/pfunction.cpp",
		MAME_DIR .. "src/lib/netlist/plib/pfunction.h",
		MAME_DIR .. "src/lib/netlist/plib/pfmtlog.cpp",
		MAME_DIR .. "src/lib/netlist/plib/pfmtlog.h",
		MAME_DIR .. "src/lib/netlist/plib/plists.h",
		MAME_DIR .. "src/lib/netlist/plib/pdynlib.cpp",
		MAME_DIR .. "src/lib/netlist/plib/pdynlib.h",
		MAME_DIR .. "src/lib/netlist/plib/pmain.cpp",
		MAME_DIR .. "src/lib/netlist/plib/pmain.h",
		MAME_DIR .. "src/lib/netlist/plib/pmath.h",
		MAME_DIR .. "src/lib/netlist/plib/pmempool.h",
		MAME_DIR .. "src/lib/netlist/plib/pmulti_threading.h",
		MAME_DIR .. "src/lib/netlist/plib/pomp.h",
		MAME_DIR .. "src/lib/netlist/plib/poptions.cpp",
		MAME_DIR .. "src/lib/netlist/plib/poptions.h",
		MAME_DIR .. "src/lib/netlist/plib/ppmf.h",
		MAME_DIR .. "src/lib/netlist/plib/ppreprocessor.cpp",
		MAME_DIR .. "src/lib/netlist/plib/ppreprocessor.h",
		MAME_DIR .. "src/lib/netlist/plib/prandom.h",
		MAME_DIR .. "src/lib/netlist/plib/pstate.h",
		MAME_DIR .. "src/lib/netlist/plib/pstonum.h",
		MAME_DIR .. "src/lib/netlist/plib/pstring.cpp",
		MAME_DIR .. "src/lib/netlist/plib/pstring.h",
		MAME_DIR .. "src/lib/netlist/plib/pstrutil.h",
		MAME_DIR .. "src/lib/netlist/plib/pstream.h",
		MAME_DIR .. "src/lib/netlist/plib/ptime.h",
		MAME_DIR .. "src/lib/netlist/plib/ptimed_queue.h",
		MAME_DIR .. "src/lib/netlist/plib/ptokenizer.cpp",
		MAME_DIR .. "src/lib/netlist/plib/ptokenizer.h",
		MAME_DIR .. "src/lib/netlist/plib/ptypes.h",
		MAME_DIR .. "src/lib/netlist/plib/putil.cpp",
		MAME_DIR .. "src/lib/netlist/plib/putil.h",
		MAME_DIR .. "src/lib/netlist/tools/nl_convert.cpp",
		MAME_DIR .. "src/lib/netlist/tools/nl_convert.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_bjt.cpp",
		MAME_DIR .. "src/lib/netlist/analog/nld_bjt.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_generic_models.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_mosfet.cpp",
		MAME_DIR .. "src/lib/netlist/analog/nld_mosfet.h",
		MAME_DIR .. "src/lib/netlist/analog/nlid_fourterm.cpp",
		MAME_DIR .. "src/lib/netlist/analog/nlid_fourterm.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_fourterm.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_switches.cpp",
		MAME_DIR .. "src/lib/netlist/analog/nld_switches.h",
		MAME_DIR .. "src/lib/netlist/analog/nlid_twoterm.cpp",
		MAME_DIR .. "src/lib/netlist/analog/nlid_twoterm.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_twoterm.h",
		MAME_DIR .. "src/lib/netlist/analog/nld_opamps.cpp",
		MAME_DIR .. "src/lib/netlist/analog/nld_opamps.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_solver.cpp",
		MAME_DIR .. "src/lib/netlist/solver/nld_solver.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_matrix_solver.cpp",
		MAME_DIR .. "src/lib/netlist/solver/nld_matrix_solver.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_direct.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_direct1.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_direct2.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_sor.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_sor_mat.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_gmres.h",
		MAME_DIR .. "src/lib/netlist/solver/mat_cr.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_sm.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_w.h",
		MAME_DIR .. "src/lib/netlist/solver/nld_ms_direct_lu.h",
		MAME_DIR .. "src/lib/netlist/solver/vector_base.h",
		MAME_DIR .. "src/lib/netlist/devices/net_lib.cpp",
		MAME_DIR .. "src/lib/netlist/devices/net_lib.h",
		MAME_DIR .. "src/lib/netlist/devices/net_devinc.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_9316_base.hxx",
		MAME_DIR .. "src/lib/netlist/devices/nld_2102A.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_2102A.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_tms4800.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_tms4800.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4006.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4006.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4013.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4013.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4017.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4017.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4020.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4020.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4053.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4053.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4066.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4066.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_4316.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_4316.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7448.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7448.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7450.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7450.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7473.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7473.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7474.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7474.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7475.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7475.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7483.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7483.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7485.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7485.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7490.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7490.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7492.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7492.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7493.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7493.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_7497.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_7497.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74107.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74107.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74113.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74113.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74123.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74123.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74125.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74125.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74153.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74153.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74161.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74163.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74164.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74164.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74165.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74165.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74166.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74166.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74174.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74174.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74175.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74175.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74192.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74192.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74193.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74193.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74194.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74194.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74377.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74377.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74393.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74393.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74365.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74365.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_74ls629.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_74ls629.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_82S16.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_82S16.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_82S115.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_82S115.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_8277.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_8277.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_9310.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_9316.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_9316.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_9321.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_9321.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_9322.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_9322.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_am2847.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_am2847.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_dm9314.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_dm9314.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_dm9334.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_dm9334.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_9321.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_9321.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_ne555.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_ne555.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_mm5837.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_mm5837.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_r2r_dac.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_r2r_dac.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_tristate.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_tristate.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_schmitt.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_schmitt.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_legacy.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_legacy.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_log.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_log.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_roms.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_roms.h",
		MAME_DIR .. "src/lib/netlist/devices/nld_system.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nld_system.h",
		MAME_DIR .. "src/lib/netlist/devices/nlid_truthtable.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nlid_truthtable.h",
		MAME_DIR .. "src/lib/netlist/devices/nlid_system.h",
		MAME_DIR .. "src/lib/netlist/devices/nlid_proxy.cpp",
		MAME_DIR .. "src/lib/netlist/devices/nlid_proxy.h",
		MAME_DIR .. "src/lib/netlist/macro/nlm_base_lib.cpp",
		MAME_DIR .. "src/lib/netlist/macro/nlm_base_lib.h",
		MAME_DIR .. "src/lib/netlist/macro/nlm_ttl74xx_lib.cpp",
		MAME_DIR .. "src/lib/netlist/macro/nlm_ttl74xx_lib.h",
		MAME_DIR .. "src/lib/netlist/macro/nlm_cd4xxx_lib.cpp",
		MAME_DIR .. "src/lib/netlist/macro/nlm_cd4xxx_lib.h",
		MAME_DIR .. "src/lib/netlist/macro/nlm_opamp_lib.cpp",
		MAME_DIR .. "src/lib/netlist/macro/nlm_opamp_lib.h",
		MAME_DIR .. "src/lib/netlist/macro/nlm_otheric_lib.cpp",
		MAME_DIR .. "src/lib/netlist/macro/nlm_otheric_lib.h",
		MAME_DIR .. "src/lib/netlist/macro/nlm_roms_lib.cpp",
		MAME_DIR .. "src/lib/netlist/macro/nlm_roms_lib.h",

		MAME_DIR .. "src/lib/netlist/generated/static_solvers.cpp",
	}
