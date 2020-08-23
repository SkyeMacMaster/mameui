// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//
// Netlist for Fire One
//
// Derived from the schematics in the manual.
//
// Known problems/issues:
//
//    * Slow!
//

#include "netlist/devices/net_lib.h"

//
// 556 is just two 555s in one package
//

static NETLIST_START(NE556_DIP)
	NE555(A)
	NE555(B)

	NET_C(A.GND, B.GND)
	NET_C(A.VCC, B.VCC)

	DIPPINS(      /*        +--------------+        */
		 A.DISCH, /* 1DISCH |1     ++    14| VCC    */ A.VCC,
		A.THRESH, /* 1THRES |2           13| 2DISCH */ B.DISCH,
		  A.CONT, /*  1CONT |3           12| 2THRES */ B.THRESH,
		 A.RESET, /* 1RESET |4   NE556   11| 2CONT  */ B.CONT,
		   A.OUT, /*   1OUT |5           10| 2RESET */ B.RESET,
		  A.TRIG, /*  1TRIG |6            9| 2OUT   */ B.OUT,
		   A.GND, /*    GND |7            8| 2TRIG  */ B.TRIG
				  /*        +--------------+        */
	)
NETLIST_END()


//
// ICL8038 is broadly similar to a 566 VCO, and can be simulated partially as such.
//

static NETLIST_START(ICL8038_DIP)
	VCVS(VI, 1)
	CCCS(CI1, -1)
	CCCS(CI2, 2)
	SYS_COMPD(COMP)
	SYS_DSW2(SW)
	VCVS(VO, 1)
	RES(R_SHUNT, RES_R(50))

	PARAM(VO.RO, 50)
	PARAM(COMP.MODEL, "FAMILY(TYPE=CUSTOM IVL=0.16 IVH=0.4 OVL=0.01 OVH=0.01 ORL=50 ORH=50)")
	PARAM(SW.GOFF, 0) // This has to be zero to block current sources

	NET_C(VI.OP, CI1.IN, CI2.IN)
	NET_C(CI1.OP, VO.IP)
	NET_C(COMP.Q, SW.I)
	NET_C(CI2.OP, SW.2)
	NET_C(COMP.VCC, R_SHUNT.1)
	NET_C(SW.1, R_SHUNT.2)
	NET_C(SW.3, VO.IP)
	NET_C(VO.OP, COMP.IN)

	// Avoid singular Matrix due to G=0 switch
	RES(RX1, 1e10)
	RES(RX2, 1e10)
	NET_C(RX1.1, SW.1)
	NET_C(RX2.1, SW.3)

	NET_C(COMP.GND, RX1.2, RX2.2)

	RES(R1, 5000)
	RES(R2, 5000)
	RES(R3, 5000)

	// Square output wave
	VCVS(V_SQR, 1)
	NET_C(COMP.Q, V_SQR.IP)

	NET_C(COMP.GND, SW.GND, VI.ON, VI.IN, CI1.ON, CI2.ON, VO.IN, VO.ON, R2.2, V_SQR.IN, V_SQR.ON)
	NET_C(COMP.VCC, SW.VCC, R1.2)
	NET_C(COMP.IP, R1.1, R2.1, R3.1)
	NET_C(COMP.Q, R3.2)

	ALIAS(11, VI.ON) // GND
	ALIAS(9, V_SQR.OP) // Square out
	ALIAS(3, VO.OP) // Triag out
	ALIAS(8, VI.IP) // VC
	ALIAS(4, CI1.IP) // R1
	ALIAS(5, CI2.IP) // R2
	ALIAS(10, VO.IP) // C1
	ALIAS(6, COMP.VCC) // V+
NETLIST_END()

//
// Main netlist
//

NETLIST_START(fireone)
	NET_MODEL("2N3704 NPN(IS=26.03f VAF=90.7 Bf=736.1K IKF=.1983 XTB=1.5 BR=1.024 CJC=11.01p CJE=24.07p RB=10 RC=.5 RE=.5 TR=233.8n TF=1.03n ITF=0 VTF=0 XTF=0 mfg=Motorola)")

	SOLVER(Solver, 48000)

	ANALOG_INPUT(V12, 12)
	ANALOG_INPUT(VM12, -12)
	ANALOG_INPUT(V5, 5)
	ALIAS(VCC, V5)

	NET_MODEL("PIT8253PORT FAMILY(TYPE=NMOS OVL=0.05 OVH=0.05 ORL=100.0 ORH=0.5k)")

	TTL_INPUT(LTORP, 0)         // active high
	TTL_INPUT(LSHPHT, 0)        // active high
	TTL_INPUT(LBOOM, 0)         // active high
	TTL_INPUT(SOUND_OFF_L, 0)   // active high
	TTL_INPUT(SOUND_OFF_R, 0)   // active high
	TTL_INPUT(RTORP, 0)         // active high
	TTL_INPUT(RSHPHT, 0)        // active high
	TTL_INPUT(RBOOM, 0)         // active high
	TTL_INPUT(TORPCOLL, 0)      // active high
	TTL_INPUT(SUBENG, 0)        // active high
	TTL_INPUT(ALERT, 0)         // active high
	//TTL_INPUT(SONAR_ENABLE, 0)    // active high
	TTL_INPUT(SONAR_SYNC, 0)    // active low

	LOGIC_INPUT(MUSIC_A,   0, "PIT8253PORT")
	LOGIC_INPUT(MUSIC_B,   0, "PIT8253PORT")
	LOGIC_INPUT(MUSIC_C,   0, "PIT8253PORT")

	NET_C(VCC, LTORP.VCC, LSHPHT.VCC, LBOOM.VCC, SOUND_OFF_L.VCC, SOUND_OFF_R.VCC, RTORP.VCC, RSHPHT.VCC, RBOOM.VCC, TORPCOLL.VCC, SUBENG.VCC, ALERT.VCC, SONAR_SYNC.VCC)
	NET_C(GND, LTORP.GND, LSHPHT.GND, LBOOM.GND, SOUND_OFF_L.GND, SOUND_OFF_R.GND, RTORP.GND, RSHPHT.GND, RBOOM.GND, TORPCOLL.GND, SUBENG.GND, ALERT.GND, SONAR_SYNC.GND)
	NET_C(VCC, MUSIC_A.VCC, MUSIC_B.VCC, MUSIC_C.VCC)
	NET_C(GND, MUSIC_A.GND, MUSIC_B.GND, MUSIC_C.GND)

	LOCAL_SOURCE(NE556_DIP)
	LOCAL_SOURCE(ICL8038_DIP)

	TTL_7406_GATE(IC27_A)
	TTL_7406_GATE(IC27_B)
	TTL_7406_GATE(IC27_C)
	TTL_7406_GATE(IC27_D)
	NET_C(VCC, IC27_A.VCC, IC27_B.VCC, IC27_C.VCC, IC27_D.VCC)
	NET_C(GND, IC27_A.GND, IC27_B.GND, IC27_C.GND, IC27_D.GND)

	//CD4070_GATE(IC41_A)
	//CD4070_GATE(IC41_B)
	CD4070_GATE(IC41_C)
	CD4070_GATE(IC41_D)
	NET_C(V12, /*IC41_A.VDD, IC41_B.VDD,*/ IC41_C.VDD, IC41_D.VDD)
	NET_C(GND, /*IC41_A.VSS, IC41_B.VSS,*/ IC41_C.VSS, IC41_D.VSS)

	CD4006_DIP(IC40)
	NET_C(V12, IC40.14)
	NET_C(GND, IC40.7)

	CD4017_DIP(IC25)
	NET_C(V12, IC25.16)
	NET_C(GND, IC25.8)

	CD4013(IC3_A)
	CD4013(IC3_B)
	NET_C(V12, IC3_A.VDD, IC3_B.VDD)
	NET_C(GND, IC3_A.VSS, IC3_A.SET, IC3_A.RESET, IC3_B.VSS, IC3_B.SET)

	MC1558_DIP(IC6)
	NET_C(IC6.8, V12)
	NET_C(IC6.4, VM12)

	MC1558_DIP(IC16)
	NET_C(IC16.8, V12)
	NET_C(IC16.4, VM12)

	MC1558_DIP(IC17)
	NET_C(IC17.8, V12)
	NET_C(IC17.4, VM12)

	NE555(IC29)

	SUBMODEL(NE556_DIP, IC31)
	NET_C(IC31.14, V5)
	NET_C(IC31.7, GND)

	MC3340_DIP(IC28)
	NET_C(IC28.8, V12)
	NET_C(IC28.3, GND)

	MC3340_DIP(IC30)
	NET_C(IC30.8, V12)
	NET_C(IC30.3, GND)

	SUBMODEL(ICL8038_DIP, IC15)
	NET_C(V12, IC15.6)
	NET_C(GND, IC15.11)

	LM3900(IC2_A)
	LM3900(IC2_D)
	LM3900(IC4_A)
	LM3900(IC4_B)
	LM3900(IC4_D)
	LM3900(IC14_A)
	LM3900(IC14_B)
	LM3900(IC14_C)
	LM3900(IC14_D)
	NET_C(V12, IC2_A.VCC, IC2_D.VCC, IC4_A.VCC, IC4_B.VCC, IC4_D.VCC, IC14_A.VCC, IC14_B.VCC, IC14_C.VCC, IC14_D.VCC)
	NET_C(GND, IC2_A.GND, IC2_D.GND, IC4_A.GND, IC4_B.GND, IC4_D.GND, IC14_A.GND, IC14_B.GND, IC14_C.GND, IC14_D.GND)

	RES(R1, RES_K(560))
	RES(R2, RES_K(560))
	RES(R3, RES_K(560))
	RES(R4, RES_K(560))
	RES(R5, RES_K(560))
	RES(R6, RES_K(560))
	RES(R8, RES_M(2))
	RES(R9, RES_K(820))
	RES(R10, RES_K(220))
	RES(R11, RES_K(620))
	RES(R12, RES_K(3.9))
	RES(R13, RES_K(3.9))
	RES(R14, RES_K(3.9))
	RES(R15, RES_M(5.6))
	RES(R16, RES_K(680))
	RES(R17, RES_K(10))
	RES(R18, RES_K(1))
	RES(R19, RES_K(150))
	RES(R20, RES_K(2))
	RES(R21, RES_K(180))
	RES(R22, RES_K(180))
	RES(R23, RES_K(130))
	RES(R24, RES_K(10))
	RES(R25, RES_K(2))
	RES(R26, RES_K(68))
	RES(R27, RES_K(270))
	RES(R28, RES_K(10))
	RES(R29, RES_K(130))
	RES(R31, RES_K(560))
	RES(R32, RES_K(220))
	RES(R33, RES_M(1.2))
	RES(R34, RES_K(12))
	RES(R35, RES_K(1))
	RES(R36, RES_K(15))
	RES(R37, RES_K(15))
	RES(R38, RES_K(820))
	RES(R39, RES_M(1))
	RES(R40, RES_K(220))
	RES(R41, RES_K(680))
	RES(R42, RES_K(820))
	RES(R43, RES_K(12))
	RES(R44, RES_K(560))
	RES(R45, RES_M(1.2))
	RES(R46, RES_K(680))
	RES(R47, RES_M(1.2))
	RES(R48, RES_K(12))
	RES(R49, RES_M(1.2))
	RES(R50, RES_K(820))
	RES(R51, RES_M(5.6))
	RES(R52, RES_K(680))
	RES(R53, RES_K(680))
	RES(R54, RES_M(2))
	RES(R55, RES_K(1))
	RES(R56, RES_K(10))
	RES(R57, RES_K(680))
	RES(R58, RES_M(1))
	RES(R59, RES_K(43))
	RES(R60, RES_K(43))
	RES(R61, RES_K(150))
	RES(R62, RES_M(1))
	//RES(R63, RES_K(82))
	POT(R64, RES_K(10))
	POT(R65, RES_K(10))
	RES(R66, RES_K(27))
	RES(R67, RES_K(68))
	RES(R68, RES_K(100))
	RES(R69, RES_M(1))
	RES(R70, RES_K(10))
	RES(R71, RES_K(16))
	RES(R72, RES_K(68))
	RES(R73, RES_K(47))
	RES(R74, RES_K(12))
	RES(R75, RES_K(120))
	RES(R76, RES_K(33))
	RES(R77, RES_K(33))
	RES(R78, RES_K(150))
	RES(R79, RES_K(130))
	RES(R80, RES_K(130))
	RES(R81, RES_K(560))
	RES(R82, RES_K(160))
	RES(R83, RES_M(1))
	RES(R84, RES_K(100))
	RES(R85, RES_K(270))
	RES(R86, RES_K(16))
	RES(R87, RES_K(100))
	RES(R94, RES_K(1))
	RES(R95, RES_K(1))
	RES(R96, RES_K(1))
	RES(R99, RES_K(1))
	RES(R100, RES_K(1))
	//RES(R101, RES_K(10))
	RES(R102, RES_K(3))
	RES(R103, RES_K(3))
	RES(R104, RES_M(1))
	RES(R105, RES_K(100))
	RES(R106, RES_K(100))
	RES(R107, RES_K(47))
	RES(R108, RES_K(20))
	RES(R109, RES_K(560))
	RES(R110, RES_K(1))
	RES(R116, RES_K(4.7))
	RES(R117, RES_K(20))
	RES(R118, RES_K(560))
	RES(R119, RES_K(560))
	RES(R120, RES_K(5.6))
	RES(R122, RES_K(2.7))
	RES(R123, RES_K(100))
	RES(R124, RES_K(3))
	RES(R125, RES_K(3))
	RES(R126, RES_K(6.8))
	RES(R127, RES_K(8.2))
	RES(R132, RES_M(1))
	RES(R133, RES_M(1))
	RES(R134, RES_M(1))
	RES(R135, RES_M(1))
	RES(R137, RES_K(5.6))
	RES(R138, RES_K(47))
	RES(R139, 100)
	RES(R140, RES_K(10))
	RES(R141, RES_K(3))
	//RES(R142, RES_K(56))
	//RES(R143, RES_K(56))
	RES(R144, RES_K(100))
	RES(R150, RES_K(100))

	CAP(C3, CAP_U(1.0))
	CAP(C4, CAP_U(0.1))
	CAP(C6, CAP_U(4.7))
	CAP(C7, CAP_U(0.33))
	CAP(C8, CAP_U(0.33))
	CAP(C10, CAP_U(0.1))
	CAP(C11, CAP_U(2.2))
	CAP(C12, CAP_U(2.2))
	CAP(C13, CAP_U(4.7))
	CAP(C14, CAP_U(2.2))
	CAP(C15, CAP_U(1.0))
	CAP(C16, CAP_U(10))
	CAP(C17, CAP_U(0.1))
	CAP(C18, CAP_U(0.1))
	CAP(C20, CAP_U(10))
	CAP(C22, CAP_U(0.033))
	CAP(C23, CAP_U(0.33))
	CAP(C26, CAP_U(0.022))
	CAP(C27, CAP_U(0.022))
	CAP(C35, CAP_U(0.0022))
	CAP(C37, CAP_U(0.0022))
	CAP(C38, CAP_U(0.1))
	CAP(C39, CAP_U(0.0022))
	CAP(C40, CAP_U(0.033))
	CAP(C41, CAP_U(1.0))
	CAP(C42, CAP_U(1.0))
	CAP(C44, CAP_U(0.0047))
	CAP(C45, CAP_U(0.1))
	CAP(C48, CAP_U(0.1))
	CAP(C49, CAP_U(0.0022))
	CAP(C50, CAP_U(1.0))
	CAP(C51, CAP_U(0.1))
	CAP(C52, CAP_U(0.1))
	CAP(C55, CAP_U(0.1))
	CAP(C70, CAP_U(1.0))
	CAP(C72, CAP_U(2.2))
	CAP(C73, CAP_U(1.0))
	CAP(C74, CAP_U(1.0))
	CAP(C75, CAP_U(0.1))
	CAP(C80, CAP_U(1.0))
	CAP(C81, CAP_U(0.1))
	CAP(C82, CAP_U(0.1))
	CAP(C84, CAP_U(0.1))
	CAP(C85, CAP_U(2.2))
	CAP(C97, CAP_U(0.0022))
	CAP(C98, CAP_U(0.0022))
	CAP(C100, CAP_U(0.22))
	//CAP(C102, CAP_U(0.001))
	CAP(C106, CAP_U(0.1))
	CAP(C109, CAP_U(0.1))
	CAP(C114, CAP_U(0.1))
	CAP(C121, CAP_U(0.001))
	CAP(C122, CAP_U(0.001))

	DIODE(D1, "1N914")
	DIODE(D2, "1N914")
	DIODE(D3, "1N914")
	DIODE(D4, "1N914")
	DIODE(D5, "1N914")
	DIODE(D6, "1N914")
	DIODE(D7, "1N914")
	DIODE(D8, "1N914")
	DIODE(D9, "1N914")
	DIODE(D10, "1N914")
	DIODE(D11, "1N914")
	DIODE(D12, "1N914")
	DIODE(D13, "1N914")
	DIODE(D14, "1N914")
	DIODE(D15, "1N914")

	QBJT_EB(Q1, "2N3704")
	QBJT_EB(Q2, "2N3704")

	// Noise Generator
	CLOCK(HLE_SONAR_CLOCK, 1000)
	NET_C(HLE_SONAR_CLOCK.GND, GND)
	NET_C(HLE_SONAR_CLOCK.VCC, V12)

	SWITCH2(SONAR_ENABLE)
	NET_C(SONAR_ENABLE.1, HLE_SONAR_CLOCK.Q)
	NET_C(SONAR_ENABLE.2, GND)
	NET_C(SONAR_ENABLE.Q, IC40.3)

	NET_C(IC40.1, IC40.12, IC41_C.A)
	NET_C(IC40.4, IC41_C.Q)
	NET_C(IC40.5, IC41_D.Q)
	NET_C(R141.1, IC40.6, IC40.10, C45.1)
	ALIAS(NOISE_A, R141.1)
	NET_C(R141.2, R140.1)
	NET_C(R140.2, V12)
	ALIAS(NOISE, R140.1)
	NET_C(IC40.8, IC41_C.B)
	NET_C(IC40.13, IC41_D.A)

	// Sonar
	NET_C(IC27_A.A, SONAR_SYNC.Q)
	NET_C(IC27_A.Y, R70.1, IC25.15, IC41_D.B)
	NET_C(R70.2, V12, IC29.RESET)
	NET_C(IC29.GND, GND)
	NET_C(IC29.VCC, V12)
	NET_C(IC29.DISCH, R110.1, R107.1)
	NET_C(R110.2, V12)
	NET_C(IC29.THRESH, IC29.TRIG, R107.2, C72.1)
	NET_C(C72.2, GND)
	NET_C(IC29.OUT, D3.K, IC3_A.CLOCK)
	NET_C(IC3_A.QQ, IC3_A.DATA)
	NET_C(IC3_A.Q, D2.K, IC25.14)
	NET_C(IC25.13, GND)
	NET_C(IC25.2, R36.1)
	NET_C(IC25.4, R72.1)
	NET_C(IC25.10, R73.1)
	NET_C(IC25.11, R75.1)
	NET_C(R36.2, R72.2, R73.2, R75.2, D3.A, D2.A, R19.2, Q2.C)
	NET_C(NOISE_A, R138.1)
	NET_C(R138.2, C100.1, R137.1)
	NET_C(C100.2, GND)
	NET_C(R137.2, R139.1, C27.1, C26.1, R19.1)
	NET_C(R139.2, GND)
	NET_C(IC6.3, GND)
	NET_C(IC6.2, R38.1, C27.2)
	NET_C(IC6.1, R37.1, R38.2, R85.1, R27.1, C26.2)
	NET_C(R37.2, D7.A, D8.K, IC6.5)
	NET_C(D7.K, GND)
	NET_C(D8.A, GND)
	NET_C(IC6.6, R35.1, R39.1)
	NET_C(R35.2, C23.1)
	NET_C(C23.2, GND)
	NET_C(IC6.7, R56.1, R39.2)
	NET_C(R56.2, D14.K, Q2.B)
	NET_C(D14.A, GND)
	NET_C(Q2.E, GND)

	ALIAS(MIX_L, R27.2)
	ALIAS(MIX_R, R85.2)

	// Low Filter
	NET_C(C45.2, R68.1)
	NET_C(R68.2, IC16.6, R66.1)
	NET_C(IC16.5, GND)
	NET_C(R66.2, IC16.7, R71.1)
	NET_C(R71.2, R86.1, C51.1)
	NET_C(R86.2, C55.1, IC17.5)
	NET_C(C55.2, GND)
	NET_C(C51.2, IC17.7, R82.1)
	NET_C(R82.2, IC17.6, R123.1)
	NET_C(R123.2, GND)
	ALIAS(RUMBLE, IC17.7)

	// Submarine Engine
	NET_C(IC31.4, V5)
	NET_C(IC31.1, R120.1, R122.1)
	NET_C(R120.2, V5)
	NET_C(IC31.2, IC31.6, IC31.11, R122.2, C85.1)
	NET_C(C85.2, GND)
	NET_C(IC31.5, GND)
	NET_C(IC31.13, R126.1, R127.1)
	NET_C(R126.2, V5)
	NET_C(IC31.8, IC31.12, R127.2, C84.1)
	NET_C(C84.2, GND)
	NET_C(IC31.10, SUBENG.Q)
	NET_C(IC31.9, R124.1)
	NET_C(R124.2, R125.1, C114.1)
	NET_C(C114.2, GND)
	NET_C(R125.2, R79.1, R80.1, C52.1)
	NET_C(C52.2, GND)
	NET_C(R79.2, MIX_L)
	NET_C(R80.2, MIX_R)

	// Ship Explosion (L)
	NET_C(RUMBLE, R87.1)
	NET_C(R87.2, R84.1, C80.1)
	NET_C(R84.2, GND)
	NET_C(IC30.1, C80.2)
	NET_C(IC30.7, R117.1)
	NET_C(IC30.6, C81.1)
	NET_C(C81.2, GND)
	NET_C(LBOOM.Q, IC27_C.A)
	NET_C(IC27_C.Y, R103.1)
	NET_C(R103.2, R104.1, C70.1)
	NET_C(C70.2, GND)
	NET_C(R104.2, IC17.3, R83.1)
	NET_C(R83.2, V5)
	NET_C(IC17.1, IC17.2, IC30.2)
	NET_C(R117.2, MIX_L)

	// Ship Explosion (R)
	NET_C(RUMBLE, R105.1)
	NET_C(R105.2, R106.1, C74.1)
	NET_C(R106.2, GND)
	NET_C(IC28.1, C74.2)
	NET_C(IC28.7, R108.1)
	NET_C(IC28.6, C75.1)
	NET_C(C75.2, GND)
	NET_C(RBOOM.Q, IC27_D.A)
	NET_C(IC27_D.Y, R102.1)
	NET_C(R102.2, R69.1, C73.1)
	NET_C(C73.2, GND)
	NET_C(R69.2, IC16.3, R62.1)
	NET_C(R62.2, V5)
	NET_C(IC16.1, IC16.2, IC28.2)
	NET_C(R108.2, MIX_R)

	// Torpedo (L)
	NET_C(LTORP.Q, R100.1, D10.A, D11.A)
	NET_C(R100.2, V5)
	NET_C(D10.K, C7.1, R46.1)
	NET_C(D11.K, C14.1, R53.1)
	NET_C(C7.2, C14.2, R132.2, R133.2, GND)
	NET_C(R53.2, IC14_C.PLUS)
	NET_C(R51.1, V12)
	NET_C(R51.2, C42.1, R54.1, IC14_C.MINUS)
	NET_C(R54.2, C42.2, IC14_C.OUT, R57.1)
	NET_C(R57.2, R46.2, IC14_B.PLUS)
	NET_C(NOISE, R42.1)
	NET_C(R42.2, R47.1, IC14_B.MINUS)
	NET_C(R47.2, C35.1, IC14_B.OUT)
	NET_C(C35.2, R132.1, C98.1)
	NET_C(C98.2, R133.1, R118.1)
	NET_C(R118.2, MIX_L)

	// Torpedo (R)
	NET_C(RTORP.Q, R55.1, D12.A, D9.A)
	NET_C(R55.2, V5)
	NET_C(D12.K, C8.1, R52.1)
	NET_C(D9.K, C12.1, R16.1)
	NET_C(C8.2, C12.2, R134.2, R135.2, GND)
	NET_C(R16.2, IC2_A.PLUS)
	NET_C(R15.1, V12)
	NET_C(R15.2, C3.1, R8.1, IC2_A.MINUS)
	NET_C(R8.2, C3.2, IC2_A.OUT, R41.1)
	NET_C(R41.2, R52.2, IC14_A.PLUS)
	NET_C(NOISE, R50.1)
	NET_C(R50.2, R49.1, IC14_A.MINUS)
	NET_C(R49.2, C37.1, IC14_A.OUT)
	NET_C(C37.2, R134.1, C97.1)
	NET_C(C97.2, R135.1, R109.1)
	NET_C(R109.2, MIX_R)

	// Ship Partial Hit (L)
	NET_C(LSHPHT.Q, R94.1, D6.A)
	NET_C(R94.2, V5)
	NET_C(D6.K, C13.1, R32.1)
	NET_C(C13.2, C49.2, C22.2, GND)
	NET_C(R32.2, IC4_D.PLUS)
	NET_C(NOISE, R31.1)
	NET_C(R31.2, R33.1, IC4_D.MINUS)
	NET_C(R33.2, R34.1, IC4_D.OUT)
	NET_C(R34.2, C49.1, R74.1)
	NET_C(R74.2, C22.1, R78.1)
	NET_C(R78.2, MIX_L)

	// Ship Partial Hit (R)
	NET_C(RSHPHT.Q, R99.1, D13.A)
	NET_C(R99.2, V5)
	NET_C(D13.K, C6.1, R40.1)
	NET_C(C6.2, C39.2, C40.2, GND)
	NET_C(R40.2, IC14_D.PLUS)
	NET_C(NOISE, R44.1)
	NET_C(R44.2, R45.1, IC14_D.MINUS)
	NET_C(R45.2, R48.1, IC14_D.OUT)
	NET_C(R48.2, C39.1, R43.1)
	NET_C(R43.2, C40.1, R61.1)
	NET_C(R61.2, MIX_L)

	// Torpedo Collision
	NET_C(TORPCOLL.Q, R95.1, D1.A)
	NET_C(R95.2, V5)
	NET_C(D1.K, C11.1, R10.1)
	NET_C(C11.2, C4.2, C38.2, GND)
	NET_C(R10.2, IC2_D.PLUS)
	NET_C(NOISE, R11.1)
	NET_C(R11.2, R9.1, IC2_D.MINUS)
	NET_C(R9.2, R12.1, IC2_D.OUT)
	NET_C(R12.2, C4.1, R13.1)
	NET_C(R13.2, C38.1, R76.1, R77.1)
	NET_C(R77.2, MIX_L)
	NET_C(R76.2, MIX_R)

	// Alert
	NET_C(ALERT.Q, IC27_B.A)
	NET_C(IC27_B.Y, R96.2, D15.A, IC3_B.RESET)
	NET_C(D15.K, C41.2, Q1.C, IC15.8)
	NET_C(R14.2, R17.1, Q1.B)
	NET_C(Q1.E, R58.1)
	NET_C(IC15.10, C44.1)
	NET_C(IC15.4, R59.2)
	NET_C(IC15.5, R60.2)
	NET_C(IC15.9, R116.2, IC3_B.CLOCK)
	NET_C(IC3_B.QQ, IC3_B.DATA)
	NET_C(IC3_B.Q, R18.1)
	NET_C(R18.2, C10.1, R119.1, R81.1)
	NET_C(V12, R96.1, C41.1, R14.1, R116.1, R59.1, R60.1)
	NET_C(GND, R17.2, R58.2, C44.2, C10.2)
	NET_C(R119.2, MIX_L)
	NET_C(R81.2, MIX_R)

	// Mixers (shared)
	NET_C(SOUND_OFF_L.Q, D4.A)
	NET_C(SOUND_OFF_R.Q, D5.A)

	// Music
	NET_C(MUSIC_A.Q, R1.1, R4.1)
	NET_C(MUSIC_B.Q, R5.1, R6.1)
	NET_C(MUSIC_C.Q, R3.1, R2.1)
	NET_C(R1.2, R3.2, R5.2, MIX_L)
	NET_C(R2.2, R4.2, R6.2, MIX_R)

	// Mixer (L)
	NET_C(MIX_L, C15.1)
	NET_C(R25.1, V12)
	NET_C(R25.2, C16.1, C48.1, R23.1)
	NET_C(C16.2, C48.2, C82.2, R64.1, R144.2, GND)
	NET_C(R23.2, IC4_A.PLUS)
	NET_C(D4.K, C82.1, R24.1)
	NET_C(R24.2, C15.2, IC4_A.MINUS, R26.1, C121.1)
	NET_C(C121.2, R26.2, IC4_A.OUT, R21.1)
	NET_C(R21.2, R64.3)
	NET_C(R64.2, C106.1)
	NET_C(C106.2, R144.1)
	ALIAS(OUT_L, C106.2)

	// Mixer (R)
	NET_C(MIX_R, C50.1)
	NET_C(R20.1, V12)
	NET_C(R20.2, C20.1, C18.1, R29.1)
	NET_C(C20.2, C18.2, C17.2, R65.1, R150.2, GND)
	NET_C(R29.2, IC4_B.PLUS)
	NET_C(D5.K, C17.1, R28.1)
	NET_C(R28.2, C50.2, IC4_B.MINUS, R67.1, C122.1)
	NET_C(C122.2, R67.2, IC4_B.OUT, R22.1)
	NET_C(R22.2, R65.3)
	NET_C(R65.2, C109.1)
	NET_C(C109.2, R150.1)
	ALIAS(OUT_R, C109.2)

	// Separate each input into the summing network
	OPTIMIZE_FRONTIER(R27.1, RES_K(270), 50) // SONAR (L)
	OPTIMIZE_FRONTIER(R77.1, RES_K(33), 50) // TORPCOLL (L)
	OPTIMIZE_FRONTIER(R78.1, RES_K(150), 50) // L SHPHT
	OPTIMIZE_FRONTIER(R79.1, RES_K(130), 50) // SUBENG (L)
	OPTIMIZE_FRONTIER(R117.1, RES_K(20), 50) // L BOOM
	OPTIMIZE_FRONTIER(R118.1, RES_K(560), 50) // L TORP

	OPTIMIZE_FRONTIER(R85.1, RES_K(270), 50) // SONAR (R)
	OPTIMIZE_FRONTIER(R76.1, RES_K(33), 50) // TORPCOLL (R)
	OPTIMIZE_FRONTIER(R61.1, RES_K(150), 50) // R SHPHT
	OPTIMIZE_FRONTIER(R80.1, RES_K(130), 50) // SUBENG (R)
	OPTIMIZE_FRONTIER(R108.1, RES_K(20), 50) // R BOOM
	OPTIMIZE_FRONTIER(R109.1, RES_K(560), 50) // R TORP

	OPTIMIZE_FRONTIER(R138.1, RES_K(47), 50) // Isolation for NOISE_A going into SONAR section
	OPTIMIZE_FRONTIER(R42.1, RES_K(820), 50) // Isolation for NOISE going into L TORP section
	OPTIMIZE_FRONTIER(R50.1, RES_K(820), 50) // Isolation for NOISE going into R TORP section
	OPTIMIZE_FRONTIER(R31.1, RES_K(560), 50) // Isolation for NOISE going into L SHPHT section
	OPTIMIZE_FRONTIER(R44.1, RES_K(560), 50) // Isolation for NOISE going into R SHPHT section
	OPTIMIZE_FRONTIER(R11.1, RES_K(620), 50) // Isolation for NOISE going into TORPCOLL section
	OPTIMIZE_FRONTIER(R87.1, RES_K(100), 50) // Isolation for RUMBLE going into L BOOM section
	OPTIMIZE_FRONTIER(R105.1, RES_K(100), 50) // Isolation for RUMBLE going into R BOOM section
NETLIST_END()
