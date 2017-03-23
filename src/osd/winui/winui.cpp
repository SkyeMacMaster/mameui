// For licensing and usage information, read docs/winui_license.txt
//****************************************************************************

 /***************************************************************************

  winui.c

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/

// standard windows headers
#define _WIN32_IE 0x0501
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>

// standard C headers
#include <dlgs.h>
#include <sys/stat.h>
#include <tchar.h>


// MAME/MAMEUI headers
#include "emu.h"
#include "mame.h"
#include "language.h"
#include "unzip.h"
#include "winutf8.h"
#include "strconv.h"
#include "window.h"
#include "zippath.h"

#include "resource.h"
#include "resource.hm"
#include "mui_util.h"
#include "mui_audit.h"
#include "directories.h"
#include "mui_opts.h"
#include "properties.h"
#include "columnedit.h"
#include "picker.h"
#include "tabview.h"
#include "bitmask.h"
#include "treeview.h"
#include "splitters.h"
#include "dirwatch.h"
#include "help.h"
#include "history.h"
#include "dialogs.h"
#include "directdraw.h"
#include "directinput.h"
#include "dijoystick.h"     /* For DIJoystick availability. */
#include "softwarelist.h"
#include "messui.h"
#include "winui.h"
#include "drivenum.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif // LVS_EX_LABELTIP

// fix warning: cast does not match function type
#if defined(__GNUC__) && defined(ListView_CreateDragImage)
#undef ListView_CreateDragImage
#endif

#ifndef ListView_CreateDragImage
#define ListView_CreateDragImage(hwnd, i, lpptUpLeft) \
    (HIMAGELIST)(LRESULT)(int)SendMessage((hwnd), LVM_CREATEDRAGIMAGE, (WPARAM)(int)(i), (LPARAM)(LPPOINT)(lpptUpLeft))
#endif // ListView_CreateDragImage

#ifndef TreeView_EditLabel
#define TreeView_EditLabel(w, i) \
    SNDMSG(w,TVM_EDITLABEL,0,(LPARAM)(i))
#endif // TreeView_EditLabel

#ifndef HDF_SORTUP
#define HDF_SORTUP 0x400
#endif // HDF_SORTUP

#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN 0x200
#endif // HDF_SORTDOWN

#ifndef LVM_SETBKIMAGEA
#define LVM_SETBKIMAGEA         (LVM_FIRST + 68)
#endif // LVM_SETBKIMAGEA

#ifndef LVM_SETBKIMAGEW
#define LVM_SETBKIMAGEW         (LVM_FIRST + 138)
#endif // LVM_SETBKIMAGEW

#ifndef LVM_GETBKIMAGEA
#define LVM_GETBKIMAGEA         (LVM_FIRST + 69)
#endif // LVM_GETBKIMAGEA

#ifndef LVM_GETBKIMAGEW
#define LVM_GETBKIMAGEW         (LVM_FIRST + 139)
#endif // LVM_GETBKIMAGEW

#ifndef LVBKIMAGE

typedef struct tagLVBKIMAGEA
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
} LVBKIMAGEA, *LPLVBKIMAGEA;

typedef struct tagLVBKIMAGEW
{
	ULONG ulFlags;
	HBITMAP hbm;
	LPWSTR pszImage;
	UINT cchImageMax;
	int xOffsetPercent;
	int yOffsetPercent;
} LVBKIMAGEW, *LPLVBKIMAGEW;

#ifdef UNICODE
#define LVBKIMAGE               LVBKIMAGEW
#define LPLVBKIMAGE             LPLVBKIMAGEW
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEW
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEW
#else
#define LVBKIMAGE               LVBKIMAGEA
#define LPLVBKIMAGE             LPLVBKIMAGEA
#define LVM_SETBKIMAGE          LVM_SETBKIMAGEA
#define LVM_GETBKIMAGE          LVM_GETBKIMAGEA
#endif
#endif

#ifndef LVBKIF_SOURCE_NONE
#define LVBKIF_SOURCE_NONE      0x00000000
#endif // LVBKIF_SOURCE_NONE

#ifndef LVBKIF_SOURCE_HBITMAP
#define LVBKIF_SOURCE_HBITMAP   0x00000001
#endif

#ifndef LVBKIF_SOURCE_URL
#define LVBKIF_SOURCE_URL       0x00000002
#endif // LVBKIF_SOURCE_URL

#ifndef LVBKIF_SOURCE_MASK
#define LVBKIF_SOURCE_MASK      0x00000003
#endif // LVBKIF_SOURCE_MASK

#ifndef LVBKIF_STYLE_NORMAL
#define LVBKIF_STYLE_NORMAL     0x00000000
#endif // LVBKIF_STYLE_NORMAL

#ifndef LVBKIF_STYLE_TILE
#define LVBKIF_STYLE_TILE       0x00000010
#endif // LVBKIF_STYLE_TILE

#ifndef LVBKIF_STYLE_MASK
#define LVBKIF_STYLE_MASK       0x00000010
#endif // LVBKIF_STYLE_MASK

#ifndef ListView_SetBkImage
#define ListView_SetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_SETBKIMAGE, 0, (LPARAM)(plvbki))
#endif // ListView_SetBkImage

#ifndef ListView_GetBkImage
#define ListView_GetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_GETBKIMAGE, 0, (LPARAM)(plvbki))
#endif // ListView_GetBkImage

#define MM_PLAY_GAME (WM_APP + 15000)

#define JOYGUI_MS 100

#define JOYGUI_TIMER 1
#define SCREENSHOT_TIMER 2
#define GAMEWND_TIMER 3

/* Max size of a sub-menu */
#define DBU_MIN_WIDTH  292
#define DBU_MIN_HEIGHT 190

static int MIN_WIDTH  = DBU_MIN_WIDTH;
static int MIN_HEIGHT = DBU_MIN_HEIGHT;

#ifndef LVS_EX_LABELTIP
#define LVS_EX_LABELTIP         0x00004000 // listview unfolds partly hidden labels if it does not have infotip text
#endif

#define NO_FOLDER -1
#define STATESAVE_VERSION 1
//I could not find a predefined value for this event and docs just say it has 1 for the parameter
#define TOOLBAR_EDIT_ACCELERATOR_PRESSED 1


/***************************************************************************
 externally defined global variables
 ***************************************************************************/
extern const ICONDATA g_iconData[];
extern const TCHAR g_szPlayGameString[];
extern const char g_szGameCountString[];
UINT8 playopts_apply = 0;

typedef struct _play_options play_options;
struct _play_options
{
	const char *record;			// OPTION_RECORD
	const char *playback;		// OPTION_PLAYBACK
	const char *state;			// OPTION_STATE
	const char *wavwrite;		// OPTION_WAVWRITE
	const char *mngwrite;		// OPTION_MNGWRITE
	const char *aviwrite;		// OPTION_AVIWRITE
};

/***************************************************************************
    function prototypes
 ***************************************************************************/

static BOOL             Win32UI_init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow);
static void             Win32UI_exit(void);

static BOOL             PumpMessage(void);
static BOOL             OnIdle(HWND hWnd);
static void             OnSize(HWND hwnd, UINT state, int width, int height);
static LRESULT CALLBACK MameWindowProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

static void             SetView(int menu_id);
static void             ResetListView(void);
static void             UpdateGameList(BOOL bUpdateRomAudit, BOOL bUpdateSampleAudit);
static void             DestroyIcons(void);
static void             ReloadIcons(void);
static void             PollGUIJoystick(void);
//static void             PressKey(HWND hwnd,UINT vk);
static BOOL             MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
static void             KeyboardKeyDown(int syskey, int vk_code, int special);
static void             KeyboardKeyUp(int syskey, int vk_code, int special);
static void             KeyboardStateClear(void);

static void             UpdateStatusBar(void);
//static BOOL             PickerHitTest(HWND hWnd);
static BOOL             TreeViewNotify(NMHDR *nm);

//static void             ResetBackground(char *szFile);
static void             LoadBackgroundBitmap(void);
static void             PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y);

static int              GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem);

static void             DisableSelection(void);
static void             EnableSelection(int nGame);

static HICON            GetSelectedPickItemIcon(void);
static void             SetRandomPickItem(void);
static void             PickColor(COLORREF *cDefault);

static LPTREEFOLDER     GetSelectedFolder(void);
static HICON            GetSelectedFolderIcon(void);

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void             MamePlayRecordGame(void);
static void             MamePlayBackGame(void);
static void             MamePlayRecordWave(void);
static void             MamePlayRecordMNG(void);
static void             MamePlayRecordAVI(void);
static void             MameLoadState(void);
static void             MamePlayGameWithOptions(int nGame, const play_options *playopts);
static BOOL             GameCheck(void);
static BOOL             FolderCheck(void);

static void             ToggleScreenShot(void);
static void             ToggleSoftware(void);
static void             AdjustMetrics(void);
//static void             EnablePlayOptions(int nIndex, windows_options *o);

/* Icon routines */
static DWORD            GetShellLargeIconSize(void);
static DWORD            GetShellSmallIconSize(void);
static void             CreateIcons(void);
static int              GetIconForDriver(int nItem);
static void             AddDriverIcon(int nItem,int default_icon_index);

// Context Menu handlers
static void             UpdateMenu(HMENU hMenu);
static void             InitTreeContextMenu(HMENU hTreeMenu);
static void             InitBodyContextMenu(HMENU hBodyContextMenu);
static void             ToggleShowFolder(int folder);
static BOOL             HandleTreeContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleScreenShotContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static void             GamePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static void             GamePicker_OnBodyContextMenu(POINT pt);

static void             InitListView(void);
/* Re/initialize the ListView header columns */
static void             ResetColumnDisplay(BOOL first_time);

static void             CopyToolTipText (LPTOOLTIPTEXT lpttt);

static void             ProgressBarShow(void);
static void             ProgressBarHide(void);
static void             ResizeProgressBar(void);
static void             ProgressBarStep(void);
static void             ProgressBarStepParam(int iGameIndex, int nGameCount);

static HWND             InitProgressBar(HWND hParent);
static HWND             InitToolbar(HWND hParent);
static HWND             InitStatusBar(HWND hParent);

static LRESULT          Statusbar_MenuSelect (HWND hwnd, WPARAM wParam, LPARAM lParam);

static void             UpdateHistory(void);


static void RemoveCurrentGameCustomFolder(void);
static void RemoveGameCustomFolder(int driver_index);

static void BeginListViewDrag(NM_LISTVIEW *pnmv);
static void MouseMoveListViewDrag(POINTS pt);
static void ButtonUpListViewDrag(POINTS p);

static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height);

BOOL MouseHasBeenMoved(void);
static void SwitchFullScreenMode(void);

/***************************************************************************
    External variables
 ***************************************************************************/

/***************************************************************************
    Internal structures
 ***************************************************************************/

/*
 * These next two structs represent how the icon information
 * is stored in an ICO file.
 */
typedef struct
{
	BYTE    bWidth;               /* Width of the image */
	BYTE    bHeight;              /* Height of the image (times 2) */
	BYTE    bColorCount;          /* Number of colors in image (0 if >=8bpp) */
	BYTE    bReserved;            /* Reserved */
	WORD    wPlanes;              /* Color Planes */
	WORD    wBitCount;            /* Bits per pixel */
	DWORD   dwBytesInRes;         /* how many bytes in this resource? */
	DWORD   dwImageOffset;        /* where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT            Width, Height, Colors; /* Width, Height and bpp */
	LPBYTE          lpBits;                /* ptr to DIB bits */
	DWORD           dwNumBytes;            /* how many bytes? */
	LPBITMAPINFO    lpbi;                  /* ptr to header */
	LPBYTE          lpXOR;                 /* ptr to XOR image bits */
	LPBYTE          lpAND;                 /* ptr to AND image bits */
} ICONIMAGE, *LPICONIMAGE;

/* Which edges of a control are anchored to the corresponding side of the parent window */
#define RA_LEFT     0x01
#define RA_RIGHT    0x02
#define RA_TOP      0x04
#define RA_BOTTOM   0x08
#define RA_ALL      0x0F

#define RA_END  0
#define RA_ID   1
#define RA_HWND 2

typedef struct
{
	int         type;       /* Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END
                               to signify last entry */
	union                   /* Can identify a child window by control id or by handle */
	{
		int     id;         /* Window control id */
		HWND    hwnd;       /* Window handle */
	} u;
	BOOL        setfont;    /* Do we set this item's font? */
	int         action;     /* What to do when control is resized */
	void        *subwindow; /* Points to a Resize structure for this subwindow; NULL if none */
} ResizeItem;

typedef struct
{
	RECT        rect;       /* Client rect of window; must be initialized before first resize */
	const ResizeItem* items;      /* Array of subitems to be resized */
} Resize;

static void ResizeWindow(HWND hParent, Resize *r);
static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hFont, BOOL bRedraw);

/* List view Icon defines */
#define LG_ICONMAP_WIDTH    GetSystemMetrics(SM_CXICON)
#define LG_ICONMAP_HEIGHT   GetSystemMetrics(SM_CYICON)
#define ICONMAP_WIDTH       GetSystemMetrics(SM_CXSMICON)
#define ICONMAP_HEIGHT      GetSystemMetrics(SM_CYSMICON)

typedef struct tagPOPUPSTRING
{
	HMENU hMenu;
	UINT uiString;
} POPUPSTRING;

#define MAX_MENUS 3

// Struct needed for Game Window Communication

typedef struct
{
	LPPROCESS_INFORMATION ProcessInfo;
	HWND hwndFound;
} FINDWINDOWHANDLE;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND   hMain  = NULL;
static HACCEL hAccel = NULL;

static HWND hwndList  = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd  = NULL;
static HWND hTabCtrl  = NULL;

static HINSTANCE hInst = NULL;

static HFONT hFont = NULL;     /* Font for list view */

static int optionfolder_count = 0;

/* global data--know where to send messages */
static BOOL in_emulation = 0;

/* idle work at startup */
static BOOL idle_work = 0;

/* object pool in use */
static object_pool *mameui_pool;

static int  game_index = 0;
static int  progBarStep = 0;

static BOOL bDoGameCheck = FALSE;

/* Tree control variables */
static BOOL bShowTree      = 1;
static BOOL bShowToolBar   = 1;
static BOOL bShowStatusBar = 1;
static BOOL bShowTabCtrl   = 1;
static BOOL bProgressShown = FALSE;
static BOOL bListReady     = FALSE;

#define	WM_MAME32_FILECHANGED (WM_USER + 0)
#define	WM_MAME32_AUDITGAME   (WM_USER + 1)

static PDIRWATCHER s_pWatcher;

/* use a joystick subsystem in the gui? */
static const struct OSDJoystick* g_pJoyGUI = NULL;

/* store current keyboard state (in bools) here */
static bool keyboard_state[4096]; /* __code_max #defines the number of internal key_codes */

/* search */
static char g_SearchText[256];
/* table copied from windows/inputs.c */
// table entry indices
#define MAME_KEY		0
#define DI_KEY			1
#define VIRTUAL_KEY		2
#define ASCII_KEY		3

// master keyboard translation table
static const int win_key_trans_table[][4] =
{
	// MAME key             dinput key          virtual key     ascii
	{ ITEM_ID_ESC,			DIK_ESCAPE,			VK_ESCAPE,		27 },
	{ ITEM_ID_1,			DIK_1,				'1',			'1' },
	{ ITEM_ID_2,			DIK_2,				'2',			'2' },
	{ ITEM_ID_3,			DIK_3,				'3',			'3' },
	{ ITEM_ID_4,			DIK_4,				'4',			'4' },
	{ ITEM_ID_5,			DIK_5,				'5',			'5' },
	{ ITEM_ID_6,			DIK_6,				'6',			'6' },
	{ ITEM_ID_7,			DIK_7,				'7',			'7' },
	{ ITEM_ID_8,			DIK_8,				'8',			'8' },
	{ ITEM_ID_9,			DIK_9,				'9',			'9' },
	{ ITEM_ID_0,			DIK_0,				'0',			'0' },
	{ ITEM_ID_BACKSPACE,	DIK_BACK,			VK_BACK,		8 },
	{ ITEM_ID_TAB,			DIK_TAB,			VK_TAB, 		9 },
	{ ITEM_ID_Q,			DIK_Q,				'Q',			'Q' },
	{ ITEM_ID_W,			DIK_W,				'W',			'W' },
	{ ITEM_ID_E,			DIK_E,				'E',			'E' },
	{ ITEM_ID_R,			DIK_R,				'R',			'R' },
	{ ITEM_ID_T,			DIK_T,				'T',			'T' },
	{ ITEM_ID_Y,			DIK_Y,				'Y',			'Y' },
	{ ITEM_ID_U,			DIK_U,				'U',			'U' },
	{ ITEM_ID_I,			DIK_I,				'I',			'I' },
	{ ITEM_ID_O,			DIK_O,				'O',			'O' },
	{ ITEM_ID_P,			DIK_P,				'P',			'P' },
	{ ITEM_ID_OPENBRACE,	DIK_LBRACKET,		VK_OEM_4,		'[' },
	{ ITEM_ID_CLOSEBRACE,	DIK_RBRACKET,		VK_OEM_6,		']' },
	{ ITEM_ID_ENTER,		DIK_RETURN, 		VK_RETURN,		13 },
	{ ITEM_ID_LCONTROL, 	DIK_LCONTROL,		VK_LCONTROL,	0 },
	{ ITEM_ID_A,			DIK_A,				'A',			'A' },
	{ ITEM_ID_S,			DIK_S,				'S',			'S' },
	{ ITEM_ID_D,			DIK_D,				'D',			'D' },
	{ ITEM_ID_F,			DIK_F,				'F',			'F' },
	{ ITEM_ID_G,			DIK_G,				'G',			'G' },
	{ ITEM_ID_H,			DIK_H,				'H',			'H' },
	{ ITEM_ID_J,			DIK_J,				'J',			'J' },
	{ ITEM_ID_K,			DIK_K,				'K',			'K' },
	{ ITEM_ID_L,			DIK_L,				'L',			'L' },
	{ ITEM_ID_COLON,		DIK_SEMICOLON,		VK_OEM_1,		';' },
	{ ITEM_ID_QUOTE,		DIK_APOSTROPHE,		VK_OEM_7,		'\'' },
	{ ITEM_ID_TILDE,		DIK_GRAVE,			VK_OEM_3,		'`' },
	{ ITEM_ID_LSHIFT,		DIK_LSHIFT, 		VK_LSHIFT,		0 },
	{ ITEM_ID_BACKSLASH,	DIK_BACKSLASH,		VK_OEM_5,		'\\' },
	{ ITEM_ID_Z,			DIK_Z,				'Z',			'Z' },
	{ ITEM_ID_X,			DIK_X,				'X',			'X' },
	{ ITEM_ID_C,			DIK_C,				'C',			'C' },
	{ ITEM_ID_V,			DIK_V,				'V',			'V' },
	{ ITEM_ID_B,			DIK_B,				'B',			'B' },
	{ ITEM_ID_N,			DIK_N,				'N',			'N' },
	{ ITEM_ID_M,			DIK_M,				'M',			'M' },
	{ ITEM_ID_SLASH,		DIK_SLASH,			VK_OEM_2,		'/' },
	{ ITEM_ID_RSHIFT,		DIK_RSHIFT, 		VK_RSHIFT,		0 },
	{ ITEM_ID_ASTERISK, 	DIK_MULTIPLY,		VK_MULTIPLY,	'*' },
	{ ITEM_ID_LALT, 		DIK_LMENU,			VK_LMENU,		0 },
	{ ITEM_ID_SPACE,		DIK_SPACE,			VK_SPACE,		' ' },
	{ ITEM_ID_CAPSLOCK, 	DIK_CAPITAL,		VK_CAPITAL, 	0 },
	{ ITEM_ID_F1,			DIK_F1,				VK_F1,			0 },
	{ ITEM_ID_F2,			DIK_F2,				VK_F2,			0 },
	{ ITEM_ID_F3,			DIK_F3,				VK_F3,			0 },
	{ ITEM_ID_F4,			DIK_F4,				VK_F4,			0 },
	{ ITEM_ID_F5,			DIK_F5,				VK_F5,			0 },
	{ ITEM_ID_F6,			DIK_F6,				VK_F6,			0 },
	{ ITEM_ID_F7,			DIK_F7,				VK_F7,			0 },
	{ ITEM_ID_F8,			DIK_F8,				VK_F8,			0 },
	{ ITEM_ID_F9,			DIK_F9,				VK_F9,			0 },
	{ ITEM_ID_F10,			DIK_F10,			VK_F10, 		0 },
	{ ITEM_ID_NUMLOCK,		DIK_NUMLOCK,		VK_NUMLOCK, 	0 },
	{ ITEM_ID_SCRLOCK,		DIK_SCROLL,			VK_SCROLL,		0 },
	{ ITEM_ID_7_PAD,		DIK_NUMPAD7,		VK_NUMPAD7, 	0 },
	{ ITEM_ID_8_PAD,		DIK_NUMPAD8,		VK_NUMPAD8, 	0 },
	{ ITEM_ID_9_PAD,		DIK_NUMPAD9,		VK_NUMPAD9, 	0 },
	{ ITEM_ID_MINUS_PAD,	DIK_SUBTRACT,		VK_SUBTRACT,	0 },
	{ ITEM_ID_4_PAD,		DIK_NUMPAD4,		VK_NUMPAD4, 	0 },
	{ ITEM_ID_5_PAD,		DIK_NUMPAD5,		VK_NUMPAD5, 	0 },
	{ ITEM_ID_6_PAD,		DIK_NUMPAD6,		VK_NUMPAD6, 	0 },
	{ ITEM_ID_PLUS_PAD, 	DIK_ADD,			VK_ADD, 		0 },
	{ ITEM_ID_1_PAD,		DIK_NUMPAD1,		VK_NUMPAD1, 	0 },
	{ ITEM_ID_2_PAD,		DIK_NUMPAD2,		VK_NUMPAD2, 	0 },
	{ ITEM_ID_3_PAD,		DIK_NUMPAD3,		VK_NUMPAD3, 	0 },
	{ ITEM_ID_0_PAD,		DIK_NUMPAD0,		VK_NUMPAD0, 	0 },
	{ ITEM_ID_DEL_PAD,		DIK_DECIMAL,		VK_DECIMAL, 	0 },
	{ ITEM_ID_F11,			DIK_F11,			VK_F11, 		0 },
	{ ITEM_ID_F12,			DIK_F12,			VK_F12, 		0 },
	{ ITEM_ID_F13,			DIK_F13,			VK_F13, 		0 },
	{ ITEM_ID_F14,			DIK_F14,			VK_F14, 		0 },
	{ ITEM_ID_F15,			DIK_F15,			VK_F15, 		0 },
	{ ITEM_ID_ENTER_PAD,	DIK_NUMPADENTER,	VK_RETURN,		0 },
	{ ITEM_ID_RCONTROL, 	DIK_RCONTROL,		VK_RCONTROL,	0 },
	{ ITEM_ID_SLASH_PAD,	DIK_DIVIDE,			VK_DIVIDE,		0 },
	{ ITEM_ID_PRTSCR,		DIK_SYSRQ,			0,				0 },
	{ ITEM_ID_RALT, 		DIK_RMENU,			VK_RMENU,		0 },
	{ ITEM_ID_HOME, 		DIK_HOME,			VK_HOME,		0 },
	{ ITEM_ID_UP,			DIK_UP,				VK_UP,			0 },
	{ ITEM_ID_PGUP, 		DIK_PRIOR,			VK_PRIOR,		0 },
	{ ITEM_ID_LEFT, 		DIK_LEFT,			VK_LEFT,		0 },
	{ ITEM_ID_RIGHT,		DIK_RIGHT,			VK_RIGHT,		0 },
	{ ITEM_ID_END,			DIK_END,			VK_END, 		0 },
	{ ITEM_ID_DOWN, 		DIK_DOWN,			VK_DOWN,		0 },
	{ ITEM_ID_PGDN, 		DIK_NEXT,			VK_NEXT,		0 },
	{ ITEM_ID_INSERT,		DIK_INSERT,			VK_INSERT,		0 },
	{ ITEM_ID_DEL,			DIK_DELETE,			VK_DELETE,		0 },
	{ ITEM_ID_LWIN, 		DIK_LWIN,			VK_LWIN,		0 },
	{ ITEM_ID_RWIN, 		DIK_RWIN,			VK_RWIN,		0 },
	{ ITEM_ID_MENU, 		DIK_APPS,			VK_APPS,		0 },
	{ ITEM_ID_PAUSE,		DIK_PAUSE,			VK_PAUSE,		0 },
	{ ITEM_ID_CANCEL,		0,					VK_CANCEL,		0 },
};



typedef struct
{
	char		name[40];	    // functionality name (optional)
	input_seq	is;				// the input sequence (the keys pressed)
	UINT		func_id;        // the identifier
	input_seq* (*const getiniptr)(void);// pointer to function to get the value from .ini file
} GUISequence;

static const GUISequence GUISequenceControl[]=
{
	{"gui_key_up",                input_seq(),    ID_UI_UP,           Get_ui_key_up },
	{"gui_key_down",              input_seq(),    ID_UI_DOWN,         Get_ui_key_down },
	{"gui_key_left",              input_seq(),    ID_UI_LEFT,         Get_ui_key_left },
	{"gui_key_right",             input_seq(),    ID_UI_RIGHT,        Get_ui_key_right },
	{"gui_key_start",             input_seq(),    ID_UI_START,        Get_ui_key_start },
	{"gui_key_pgup",              input_seq(),    ID_UI_PGUP,         Get_ui_key_pgup },
	{"gui_key_pgdwn",             input_seq(),    ID_UI_PGDOWN,       Get_ui_key_pgdwn },
	{"gui_key_home",              input_seq(),    ID_UI_HOME,         Get_ui_key_home },
	{"gui_key_end",               input_seq(),    ID_UI_END,          Get_ui_key_end },
	{"gui_key_ss_change",         input_seq(),    IDC_SSFRAME,        Get_ui_key_ss_change },
	{"gui_key_history_up",        input_seq(),    ID_UI_HISTORY_UP,   Get_ui_key_history_up },
	{"gui_key_history_down",      input_seq(),    ID_UI_HISTORY_DOWN, Get_ui_key_history_down },

	{"gui_key_context_filters",    input_seq(),    ID_CONTEXT_FILTERS,       Get_ui_key_context_filters },
	{"gui_key_select_random",      input_seq(),    ID_CONTEXT_SELECT_RANDOM, Get_ui_key_select_random },
	{"gui_key_game_audit",         input_seq(),    ID_GAME_AUDIT,            Get_ui_key_game_audit },
	{"gui_key_game_properties",    input_seq(),    ID_GAME_PROPERTIES,       Get_ui_key_game_properties },
	{"gui_key_help_contents",      input_seq(),    ID_HELP_CONTENTS,         Get_ui_key_help_contents },
	{"gui_key_update_gamelist",    input_seq(),    ID_UPDATE_GAMELIST,       Get_ui_key_update_gamelist },
	{"gui_key_view_folders",       input_seq(),    ID_VIEW_FOLDERS,          Get_ui_key_view_folders },
	{"gui_key_view_fullscreen",    input_seq(),    ID_VIEW_FULLSCREEN,       Get_ui_key_view_fullscreen },
	{"gui_key_view_pagetab",       input_seq(),    ID_VIEW_PAGETAB,          Get_ui_key_view_pagetab },
	{"gui_key_view_picture_area",  input_seq(),    ID_VIEW_PICTURE_AREA,     Get_ui_key_view_picture_area },
	{"gui_key_view_software_area", input_seq(),    ID_VIEW_SOFTWARE_AREA,    Get_ui_key_view_software_area },
	{"gui_key_view_status",        input_seq(),    ID_VIEW_STATUS,           Get_ui_key_view_status },
	{"gui_key_view_toolbars",      input_seq(),    ID_VIEW_TOOLBARS,         Get_ui_key_view_toolbars },

	{"gui_key_view_tab_cabinet",     input_seq(),  ID_VIEW_TAB_CABINET,       Get_ui_key_view_tab_cabinet },
	{"gui_key_view_tab_cpanel",      input_seq(),  ID_VIEW_TAB_CONTROL_PANEL, Get_ui_key_view_tab_cpanel },
	{"gui_key_view_tab_flyer",       input_seq(),  ID_VIEW_TAB_FLYER,         Get_ui_key_view_tab_flyer },
	{"gui_key_view_tab_history",     input_seq(),  ID_VIEW_TAB_HISTORY,       Get_ui_key_view_tab_history },
	{"gui_key_view_tab_marquee",     input_seq(),  ID_VIEW_TAB_MARQUEE,       Get_ui_key_view_tab_marquee },
	{"gui_key_view_tab_screenshot",  input_seq(),  ID_VIEW_TAB_SCREENSHOT,    Get_ui_key_view_tab_screenshot },
	{"gui_key_view_tab_title",       input_seq(),  ID_VIEW_TAB_TITLE,         Get_ui_key_view_tab_title },
	{"gui_key_view_tab_pcb",         input_seq(),  ID_VIEW_TAB_PCB, 	    Get_ui_key_view_tab_pcb },
	{"gui_key_quit",                 input_seq(),  ID_FILE_EXIT,              Get_ui_key_quit },
};


#define NUM_GUI_SEQUENCES (sizeof(GUISequenceControl) / sizeof(GUISequenceControl[0]))


static UINT    lastColumnClick   = 0;
static WNDPROC g_lpHistoryWndProc = NULL;
static WNDPROC g_lpPictureFrameWndProc = NULL;
static WNDPROC g_lpPictureWndProc = NULL;

static POPUPSTRING popstr[MAX_MENUS + 1];

/* Tool and Status bar variables */
static HWND hStatusBar = 0;
static HWND s_hToolBar   = 0;

/* Column Order as Displayed */
static BOOL oldControl = FALSE;
static BOOL xpControl = FALSE;

/* Used to recalculate the main window layout */
static int  bottomMargin = 0;
static int  topMargin = 0;
static int  have_history = FALSE;

static BOOL have_selection = FALSE;

static HBITMAP hMissing_bitmap = NULL;

/* Icon variables */
static HIMAGELIST   hLarge = NULL;
static HIMAGELIST   hSmall = NULL;
static HIMAGELIST   hHeaderImages = NULL;
static int          *icon_index = NULL; /* for custom per-game icons */

static const TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,    TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_SOFTWARE_AREA,TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{1, ID_VIEW_PICTURE_AREA,TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_LARGE_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 2},
	{3, ID_VIEW_SMALL_ICON, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 3},
	{4, ID_VIEW_LIST_MENU,  TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 4},
	{5, ID_VIEW_DETAIL,     TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 5},
	{6, ID_VIEW_GROUPED, TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 6},
	{0, 0,                  TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{7, ID_HELP_ABOUT,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 7},
//	{8, ID_HELP_CONTENTS,   TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 8}
};

#define NUM_TOOLBUTTONS (sizeof(tbb) / sizeof(tbb[0]))

#define NUM_TOOLTIPS 9

static const TCHAR szTbStrings[NUM_TOOLTIPS + 1][30] =
{
	TEXT("Toggle Folder List"),
	TEXT("Toggle Software List"),
	TEXT("Toggle Screen Shot"),
	TEXT("Large Icons"),
	TEXT("Small Icons"),
	TEXT("List"),
	TEXT("Details"),
	TEXT("Grouped"),
	TEXT("About"),
	TEXT("Help")
};

static const int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_SOFTWARE_AREA,
	ID_VIEW_PICTURE_AREA,
	ID_VIEW_LARGE_ICON,
	ID_VIEW_SMALL_ICON,
	ID_VIEW_LIST_MENU,
	ID_VIEW_DETAIL,
	ID_VIEW_GROUPED,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	-1
};

static const int s_nPickers[] =
{
	IDC_LIST,
	IDC_SWLIST,
	IDC_SOFTLIST
};


/* How to resize toolbar sub window */
static ResizeItem toolbar_resize_items[] =
{
	{ RA_ID,   { ID_TOOLBAR_EDIT },  TRUE, RA_RIGHT | RA_TOP,     NULL },
	{ RA_END,  { 0 },            FALSE, 0,                                 NULL }
};

static Resize toolbar_resize = { {0, 0, 0, 0}, toolbar_resize_items };

/* How to resize main window */
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            FALSE, RA_LEFT  | RA_RIGHT  | RA_TOP,     &toolbar_resize },
	{ RA_HWND, { 0 },            FALSE, RA_LEFT  | RA_RIGHT  | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  FALSE, RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     TRUE,	RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     TRUE,	RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER }, FALSE,	RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER2 },FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSPICTURE },FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  TRUE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSTAB },    FALSE,	RA_RIGHT | RA_TOP,                 NULL },
	{ RA_ID,   { IDC_SWLIST },    TRUE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SOFTLIST },  TRUE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER3 },FALSE,	RA_RIGHT | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_END,  { 0 },            FALSE, 0,                                 NULL }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

/* last directory for common file dialogs */
TCHAR last_directory[MAX_PATH];

static BOOL g_listview_dragging = FALSE;
static HIMAGELIST himl_drag;
static int game_dragged; /* which game started the drag */
static HTREEITEM prev_drag_drop_target; /* which tree view item we're currently highlighting */

static BOOL g_in_treeview_edit = FALSE;

/***************************************************************************
    Global variables
 ***************************************************************************/

/* Background Image handles also accessed from TreeView.c */
static HPALETTE         hPALbg   = 0;
static HBITMAP          hBackground  = 0;
static MYBITMAPINFO     bmDesc;

/* List view Column text */
extern const LPCTSTR column_names[COLUMN_MAX] =
{
	TEXT("Machine"),
	TEXT("Source"),
	TEXT("Directory"),
	TEXT("Type"),
	TEXT("Screen"),
	TEXT("Manufacturer"),
	TEXT("Year"),
	TEXT("Played"),
	TEXT("Play Time"),
	TEXT("Clone Of"),
	TEXT("Trackball"),
#ifdef SHOW_COLUMN_SAMPLES
	TEXT("Samples"),
#endif
#ifdef SHOW_COLUMN_ROMS
	TEXT("ROMs"),
#endif
};

/***************************************************************************
    Message Macros
 ***************************************************************************/

#ifndef StatusBar_GetItemRect
#define StatusBar_GetItemRect(hWnd, iPart, lpRect) \
    SendMessage(hWnd, SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lpRect)
#endif

#ifndef ToolBar_CheckButton
#define ToolBar_CheckButton(hWnd, idButton, fCheck) \
    SendMessage(hWnd, TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0))
#endif

//============================================================
//  winui_output_error
//============================================================


class mameui_output_error : public osd_output
{
public:
	virtual void output_callback(osd_output_channel channel, const char *msg, va_list args)
	{
		if (channel == OSD_OUTPUT_CHANNEL_ERROR)
		{
			char buffer[1024];

			// if we are in fullscreen mode, go to windowed mode
			if ((video_config.windowed == 0) && !osd_common_t::s_window_list.empty())
				winwindow_toggle_full_screen();

			vsnprintf(buffer, ARRAY_LENGTH(buffer), msg, args);printf("%s\n",buffer);
			win_message_box_utf8(!osd_common_t::s_window_list.empty() ? std::static_pointer_cast<win_window_info>(osd_common_t::s_window_list.front())->platform_window() : hMain, buffer, MAMEUINAME, MB_ICONERROR | MB_OK);
		}
		else
			chain_output(channel, msg, args);
	}
};

static std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}


/***************************************************************************
    External functions
 ***************************************************************************/
static DWORD RunMAME(int nGameIndex, const play_options *playopts)
{
	time_t start = 0, end = 0;
	double elapsedtime = 0;
	int i = 0;
	windows_options global_opts;
	std::string error_string;

	// Tell mame where to get the INIs
	SetDirectories(global_opts);

	CreateGameOptions(global_opts, OPTIONS_GLOBAL, nGameIndex);

	// set some startup options
	global_opts.set_value(OPTION_LANGUAGE, GetLanguageUI(), OPTION_PRIORITY_CMDLINE, error_string);
	global_opts.set_value(OPTION_PLUGINS, GetEnablePlugins(), OPTION_PRIORITY_CMDLINE, error_string);
	global_opts.set_value(OPTION_PLUGIN, GetPlugins(), OPTION_PRIORITY_CMDLINE, error_string);
	global_opts.set_value(OPTION_SYSTEMNAME, driver_list::driver(nGameIndex).name, OPTION_PRIORITY_CMDLINE, error_string);

	// set any specified play options
	if (playopts_apply == 0x57)
	{
		if (playopts->record)
			global_opts.set_value(OPTION_RECORD, playopts->record, OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->playback)
			global_opts.set_value(OPTION_PLAYBACK, playopts->playback, OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->state)
			global_opts.set_value(OPTION_STATE, playopts->state, OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->wavwrite)
			global_opts.set_value(OPTION_WAVWRITE, playopts->wavwrite, OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->mngwrite)
			global_opts.set_value(OPTION_MNGWRITE, playopts->mngwrite, OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->aviwrite)
			global_opts.set_value(OPTION_AVIWRITE, playopts->aviwrite, OPTION_PRIORITY_CMDLINE,error_string);
	}

	if (g_szSelectedSoftware[0] && g_szSelectedDevice[0])
	{
		global_opts.set_value(g_szSelectedDevice, g_szSelectedSoftware, OPTION_PRIORITY_CMDLINE,error_string);
		// Add params and clear so next start of driver is without parameters
		g_szSelectedSoftware[0] = 0;
		g_szSelectedDevice[0] = 0;
	}
	// Mame will parse all the needed .ini files.

	// prepare to run the game
	ShowWindow(hMain, SW_HIDE);

	for (i = 0; i < ARRAY_LENGTH(s_nPickers); i++)
		Picker_ClearIdle(GetDlgItem(hMain, s_nPickers[i]));

	// run the emulation
	time(&start);
	windows_osd_interface osd(global_opts);
	// output errors to message boxes
	mameui_output_error winerror;
	osd_output::push(&winerror);
	osd.register_options();
	mame_machine_manager *manager = mame_machine_manager::instance(global_opts, osd);
	load_translation(global_opts);
	manager->start_http_server();
	manager->start_luaengine();
	manager->execute();
	osd_output::pop(&winerror);
	global_free(manager);
	time(&end);
	elapsedtime = end - start;
	IncrementPlayTime(nGameIndex, elapsedtime);

	// clear any specified play options
	// do it this way to preserve slots and software entries
	if (playopts_apply == 0x57)
	{
		windows_options o;
		load_options(o, OPTIONS_GAME, nGameIndex);
		if (playopts->record)
			o.set_value(OPTION_RECORD, "", OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->playback)
			o.set_value(OPTION_PLAYBACK, "", OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->state)
			o.set_value(OPTION_STATE, "", OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->wavwrite)
			o.set_value(OPTION_WAVWRITE, "", OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->mngwrite)
			o.set_value(OPTION_MNGWRITE, "", OPTION_PRIORITY_CMDLINE,error_string);
		if (playopts->aviwrite)
			o.set_value(OPTION_AVIWRITE, "", OPTION_PRIORITY_CMDLINE,error_string);
		// apply the above to the ini file
		save_options(o, OPTIONS_GAME, nGameIndex);
	}
	playopts_apply = 0;

	// the emulation is complete; continue
	for (i = 0; i < ARRAY_LENGTH(s_nPickers); i++)
		Picker_ResetIdle(GetDlgItem(hMain, s_nPickers[i]));
	ShowWindow(hMain, SW_SHOW);
	SetForegroundWindow(hMain);

	return (DWORD)0;
}

int MameUIMain(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	dprintf("MAMEUI starting\n");

	if (__argc != 1)
	{
		/* Rename main because gcc will use it instead of WinMain even with -mwindows */
		extern int utf8_main(std::vector<std::string> &args);
		std::vector<std::string> utf8_argv(__argc);

		/* convert arguments to UTF-8 */
		for (int i = 0; i < __argc; i++)
			utf8_argv[i] = ui_utf8_from_wstring(__targv[i]);

		/* run utf8_main */
		exit(utf8_main(utf8_argv));
	}
	if (!Win32UI_init(hInstance, lpCmdLine, nCmdShow))
		return 1;

	// pump message, but quit on WM_QUIT
	while(PumpMessage())
		;

	Win32UI_exit();
	return 0;
}


HWND GetMainWindow(void)
{
	return hMain;
}

HWND GetTreeView(void)
{
	return hTreeView;
}

// used by messui.c
HIMAGELIST GetLargeImageList(void)
{
	return hLarge;
}

// used by messui.c
HIMAGELIST GetSmallImageList(void)
{
	return hSmall;
}

object_pool *GetMameUIMemoryPool(void)
{
	return mameui_pool;
}

void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_MAX] = {0};
	int nColumnMax = 0;
	int i = 0;
	BOOL res = 0;

	nColumnMax = Picker_GetNumColumns(hwndList);

	/* Get the Column Order and save it */
	if (!oldControl)
	{
		res = ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

		for (i = 0; i < nColumnMax; i++)
		{
			order[i] = Picker_GetRealColumnFromViewColumn(hwndList, tmpOrder[i]);
		}
	}
	res++;
}

/*
 * PURPOSE: Format raw data read from an ICO file to an HICON
 * PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
 *          UINT nBufferSize - Size of buffer ptrBuffer
 * RETURNS: HICON - handle to the icon, NULL for failure
 * History: July '95 - Created
 *          March '00- Seriously butchered from MSDN for mine own
 *          purposes, sayeth H0ek.
 */
static HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	ICONIMAGE           IconImage;
	LPICONDIRENTRY      lpIDE = NULL;
	UINT                nNumImages = 0;
	UINT                nBufferIndex = 0;
	HICON               hIcon = NULL;

	/* Is there a WORD? */
	if (nBufferSize < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it 'reserved' ?   (ie 0) */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Was it type 1? */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
	{
		return NULL;
	}

	/* Then that's the number of images in the ICO file */
	nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	/* Is there at least one icon in the file? */
	if ( nNumImages < 1 )
	{
		return NULL;
	}

	nBufferIndex += sizeof(WORD);

	/* Is there enough space for the icon directory entries? */
	if ((nBufferIndex + nNumImages * sizeof(ICONDIRENTRY)) > nBufferSize)
	{
		return NULL;
	}

	/* Assign icon directory entries from buffer */
	lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nNumImages * sizeof (ICONDIRENTRY);

	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	/* Seek to beginning of this image */
	if ( lpIDE->dwImageOffset > nBufferSize )
	{
		return NULL;
	}

	nBufferIndex = lpIDE->dwImageOffset;

	/* Read it in */
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
	{
		return NULL;
	}

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;
	/* It failed, odds are good we're on NT so try the non-Ex way */
	if (hIcon == NULL)
	{
		/* We would break on NT if we try with a 16bpp image */
		if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		{
			hIcon = CreateIconFromResourceEx(IconImage.lpBits, IconImage.dwNumBytes, TRUE, 0x00030000,0,0,LR_DEFAULTSIZE);
		}
	}
	return hIcon;
}

HICON LoadIconFromFile(const char *iconname)
{
	HICON hIcon = 0;
	struct stat file_stat;
	char tmpStr[MAX_PATH];
	char tmpIcoName[MAX_PATH];
	const char* sDirName = GetImgDir();
	PBYTE bufferPtr = 0;
	util::archive_file::error ziperr;
	util::archive_file::ptr zip;
	int res = 0;

	sprintf(tmpStr, "%s/%s.ico", GetIconsDir(), iconname);
	if (stat(tmpStr, &file_stat) != 0 || (hIcon = win_extract_icon_utf8(hInst, tmpStr, 0)) == 0)
	{
		sprintf(tmpStr, "%s/%s.ico", sDirName, iconname);
		if (stat(tmpStr, &file_stat) != 0
		|| (hIcon = win_extract_icon_utf8(hInst, tmpStr, 0)) == 0)
		{
			sprintf(tmpStr, "%s/icons.zip", GetIconsDir());
			sprintf(tmpIcoName, "%s.ico", iconname);

			ziperr = util::archive_file::open_zip(tmpStr, zip);
			if (ziperr == util::archive_file::error::NONE)
			{
				res = zip->search(tmpIcoName, false);
				if (res >= 0)
				{
					bufferPtr = (PBYTE)malloc(zip->current_uncompressed_length());
					if (bufferPtr)
					{
						ziperr = zip->decompress(bufferPtr, zip->current_uncompressed_length());
						if (ziperr == util::archive_file::error::NONE)
						{
							hIcon = FormatICOInMemoryToHICON(bufferPtr, zip->current_uncompressed_length());
						}
						free(bufferPtr);
					}
				}
				zip.reset();
			}
			else
			{
				sprintf(tmpStr, "%s/icons.7z", GetIconsDir());
				sprintf(tmpIcoName, "%s.ico", iconname);

				ziperr = util::archive_file::open_7z(tmpStr, zip);
				if (ziperr == util::archive_file::error::NONE)
				{
					res = zip->search(tmpIcoName, false);
					if (res >= 0)
					{
						bufferPtr = (PBYTE)malloc(zip->current_uncompressed_length());
						if (bufferPtr)
						{
							ziperr = zip->decompress(bufferPtr, zip->current_uncompressed_length());
							if (ziperr == util::archive_file::error::NONE)
							{
								hIcon = FormatICOInMemoryToHICON(bufferPtr, zip->current_uncompressed_length());
							}
							free(bufferPtr);
						}
					}
					zip.reset();
				}
			}
		}
	}
	return hIcon;
}

/* Return the number of folders with options */
void SetNumOptionFolders(int count)
{
	optionfolder_count = count;
}

/* search */
const char * GetSearchText(void)
{
	return g_SearchText;
}


/* Sets the treeview and listviews sizes in accordance with their visibility and the splitters */
static void ResizeTreeAndListViews(BOOL bResizeHidden)
{
	AREA area;
	GetWindowArea(&area);
	int fullwidth = area.width;
	bool bShowPicture = GetShowScreenShot();
	bool bShowSoftware = GetShowSoftware();
	int i = 0;
	int nLastWidth = 0;
	//int nLastWidth2 = 0;
	int nLeftWindowWidth = 0;
	RECT rect;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	/* Tree control */
	ShowWindow(GetDlgItem(hMain, IDC_TREE), bShowTree ? SW_SHOW : SW_HIDE);

	for (i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		bool bVisible = GetWindowLong(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), GWL_STYLE) & WS_VISIBLE ? TRUE : FALSE;
		if (bResizeHidden || bVisible)
		{
			nLeftWindowWidth = nSplitterOffset[i] - SPLITTER_WIDTH/2 - nLastWidth;

			/* special case for the rightmost pane when the screenshot is gone */
			if (!bShowPicture && !bShowSoftware && !g_splitterInfo[i+1].nSplitterWindow)
				//nLeftWindowWidth = rect.right - nLastWidth;
				nLeftWindowWidth = fullwidth - nLastWidth;

			/* woah?  are we overlapping ourselves? */
//			while ((nLeftWindowWidth + nLastWidth) > fullwidth)
//				nLeftWindowWidth--;
//			if (nLeftWindowWidth < MIN_VIEW_WIDTH)
//			{
//				nLastWidth = nLastWidth2;
//				nLeftWindowWidth = nSplitterOffset[i] - MIN_VIEW_WIDTH - SPLITTER_WIDTH/2 - nLastWidth;
//				//i--;
//			}
			if (nLastWidth > fullwidth)
				nLastWidth = fullwidth - MIN_VIEW_WIDTH;
			if ((nLastWidth + nLeftWindowWidth) > fullwidth)
				nLeftWindowWidth = MIN_VIEW_WIDTH;

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), nLastWidth, rect.top + 2,
				nLeftWindowWidth, rect.bottom - rect.top - 4, TRUE);

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow), nSplitterOffset[i], rect.top + 2,
				SPLITTER_WIDTH, rect.bottom - rect.top - 4, TRUE);
		}

		if (bVisible)
		{
			//nLastWidth2 = nLastWidth;
			nLastWidth += nLeftWindowWidth + SPLITTER_WIDTH;
		}
	}
}

void UpdateSoftware(void)
{
	RECT rect;
	//int  nWidth;

	/* first time through can't do this stuff */
	if (hwndList == NULL)
		return;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	if (GetShowSoftware())
	{
		//nWidth = nSplitterOffset[GetSplitterCount() - 1];
		CheckMenuItem(GetMenu(hMain),ID_VIEW_SOFTWARE_AREA, MF_CHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_SOFTWARE_AREA, MF_CHECKED);
	}
	else
	{
		//nWidth = rect.right;
		CheckMenuItem(GetMenu(hMain),ID_VIEW_SOFTWARE_AREA, MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_SOFTWARE_AREA, MF_UNCHECKED);
	}

	ResizeTreeAndListViews(FALSE);

	if (GetShowSoftware())
	{
		int nTab;
		HWND hwndSoftwarePicker;
		HWND hwndSoftwareDevView;
		HWND hwndSoftwareList;

		ShowWindow(GetDlgItem(hMain,IDC_SWTAB),SW_SHOW);

		hwndSoftwarePicker = GetDlgItem(GetMainWindow(), IDC_SWLIST);
		hwndSoftwareDevView = GetDlgItem(GetMainWindow(), IDC_SWDEVVIEW);
		hwndSoftwareList = GetDlgItem(GetMainWindow(), IDC_SOFTLIST);

		nTab = TabView_GetCurrentTab(GetDlgItem(GetMainWindow(), IDC_SWTAB));

		switch(nTab)
		{
			case 0:
				ShowWindow(hwndSoftwarePicker, SW_SHOW);
				ShowWindow(hwndSoftwareDevView, SW_HIDE);
				ShowWindow(hwndSoftwareList, SW_HIDE);
				break;

			case 1:
				ShowWindow(hwndSoftwarePicker, SW_HIDE);
				ShowWindow(hwndSoftwareDevView, SW_SHOW);
				ShowWindow(hwndSoftwareList, SW_HIDE);
				break;
			case 2:
				ShowWindow(hwndSoftwarePicker, SW_HIDE);
				ShowWindow(hwndSoftwareDevView, SW_HIDE);
				ShowWindow(hwndSoftwareList, SW_SHOW);
				break;
		}
	}
	else
	{
		ShowWindow(GetDlgItem(hMain,IDC_SWLIST),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SWDEVVIEW),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SOFTLIST),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SWTAB),SW_HIDE);
	}
}

/* Adjust the list view and screenshot button based on GetShowScreenShot() */
void UpdateScreenShot(void)
{
	RECT rect;
	//int  nWidth;
	RECT fRect;
	POINT p = {0, 0};

	/* first time through can't do this stuff */
	if (hwndList == NULL)
		return;

	/* Size the List Control in the Picker */
	GetClientRect(hMain, &rect);

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	if (GetShowScreenShot())
	{
		//nWidth = nSplitterOffset[GetSplitterCount() - 1];
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_CHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_PICTURE_AREA, MF_CHECKED);
	}
	else
	{
		//nWidth = rect.right;
		CheckMenuItem(GetMenu(hMain),ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_PICTURE_AREA, MF_UNCHECKED);
	}

	ResizeTreeAndListViews(FALSE);

	FreeScreenShot();

	if (have_selection)
	{
		if (!g_szSelectedItem[0] || 
			!LoadScreenShotEx(Picker_GetSelectedItem(hwndList), g_szSelectedItem, TabView_GetCurrentTab(hTabCtrl)))
				// load and set image, or empty it if we don't have one
				LoadScreenShot(Picker_GetSelectedItem(hwndList), TabView_GetCurrentTab(hTabCtrl));
	}

	// figure out if we have a history or not, to place our other windows properly
	UpdateHistory();

	// setup the picture area

	if (GetShowScreenShot())
	{
		DWORD dwStyle;
		DWORD dwStyleEx;
		BOOL showing_history;

		ClientToScreen(hMain, &p);
		GetWindowRect(GetDlgItem(hMain, IDC_SSFRAME), &fRect);
		OffsetRect(&fRect, -p.x, -p.y);

		// show history on this tab IFF
		// - we have history for the game
		// - we're on the first tab
		// - we DON'T have a separate history tab
		showing_history = (have_history && (TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab()
			|| GetHistoryTab() == TAB_ALL ) && GetShowTab(TAB_HISTORY) == FALSE);
		CalculateBestScreenShotRect(GetDlgItem(hMain, IDC_SSFRAME), &rect,showing_history);

		dwStyle   = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_STYLE);
		dwStyleEx = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_EXSTYLE);

		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwStyleEx);
		MoveWindow(GetDlgItem(hMain, IDC_SSPICTURE),
				fRect.left  + rect.left,
				fRect.top   + rect.top,
				rect.right  - rect.left,
				rect.bottom - rect.top,
				TRUE);

		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),
				   (TabView_GetCurrentTab(hTabCtrl) != TAB_HISTORY) ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_SHOW);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),bShowTabCtrl ? SW_SHOW : SW_HIDE);

		InvalidateRect(GetDlgItem(hMain,IDC_SSPICTURE),NULL,FALSE);
	}
	else
	{
		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),SW_HIDE);
	}

}

void ResizePickerControls(HWND hWnd)
{
	RECT frameRect;
	RECT rect, sRect;
	int  nListWidth = 0, nScreenShotWidth = 0;
	static BOOL firstTime = TRUE;
	int doSSControls = TRUE;
	int nSplitterCount = 0;

	nSplitterCount = GetSplitterCount();

	/* Size the List Control in the Picker */
	GetClientRect(hWnd, &rect);

	/* Calc the display sizes based on g_splitterInfo */
	if (firstTime)
	{
		RECT rWindow;

		for (int i = 0; i < nSplitterCount; i++)
//			nSplitterOffset[i] = rect.right * g_splitterInfo[i].dPosition;
			nSplitterOffset[i] = GetSplitterPos(i);

		GetWindowRect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		GetWindowRect(s_hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		/*buttonMargin = (sRect.bottom + 4); */

		firstTime = FALSE;
	}
	else
	{
		doSSControls = GetShowScreenShot();
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	MoveWindow(GetDlgItem(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, TRUE);

	ResizeTreeAndListViews(TRUE);
	if (GetShowSoftware())
		nListWidth = nSplitterOffset[2];
	else
		nListWidth = nSplitterOffset[1];
	nScreenShotWidth = rect.right - nListWidth - SPLITTER_WIDTH;

	/* Screen shot Page tab control */
	if (bShowTabCtrl)
	{
		MoveWindow(GetDlgItem(hWnd, IDC_SSTAB), nListWidth + 4, rect.top + 2,
			nScreenShotWidth - 2, rect.top + 20, doSSControls);
		rect.top += 20;
	}

	/* resize the Screen shot frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSFRAME), nListWidth + 4, rect.top + 2,
		nScreenShotWidth - 2, rect.bottom - rect.top - 4, doSSControls);

	/* The screen shot controls */
	GetClientRect(GetDlgItem(hWnd, IDC_SSFRAME), &frameRect);

	/* Text control - game history */
	sRect.left = nListWidth + 14;
	sRect.right = sRect.left + nScreenShotWidth - 22;

	if (GetShowTab(TAB_HISTORY))
	{
		// We're using the new mode, with the history filling the entire tab (almost)
		sRect.top = rect.top + 14;
		sRect.bottom = rect.bottom - rect.top - 30;
	}
	else
	{
		// We're using the original mode, with the history beneath the SS picture
		sRect.top = rect.top + 264;
		sRect.bottom = rect.bottom - rect.top - 278;
	}

	MoveWindow(GetDlgItem(hWnd, IDC_HISTORY), sRect.left, sRect.top, sRect.right - sRect.left, sRect.bottom, doSSControls);

	/* the other screen shot controls will be properly placed in UpdateScreenshot() */
}


char *ModifyThe(const char *str)
{
	static int  bufno = 0;
	static char buffer[4][255];

	if (strncmp(str, "The ", 4) == 0)
	{
		char *s, *p;
		char temp[255];

		strcpy(temp, &str[4]);

		bufno = (bufno + 1) % 4;

		s = buffer[bufno];

		/* Check for version notes in parens */
		p = strchr(temp, '(');
		if (p)
		{
			p[-1] = '\0';
		}

		strcpy(s, temp);
		strcat(s, ", The");

		if (p)
		{
			strcat(s, " ");
			strcat(s, p);
		}

		return s;
	}
	return (char *)str;
}

HBITMAP GetBackgroundBitmap(void)
{
	return hBackground;
}

HPALETTE GetBackgroundPalette(void)
{
	return hPALbg;
}

MYBITMAPINFO * GetBackgroundInfo(void)
{
	return &bmDesc;
}

BOOL GetUseOldControl(void)
{
	return oldControl;
}

BOOL GetUseXPControl(void)
{
	return xpControl;
}

int GetMinimumScreenShotWindowWidth(void)
{
	BITMAP bmp;
	GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

	return bmp.bmWidth + 6; // 6 is for a little breathing room
}


int GetParentIndex(const game_driver *driver)
{
	return GetGameNameIndex(driver->parent);
}

int GetCompatIndex(const game_driver *driver)
{
	if (driver->compatible_with)
		return GetGameNameIndex(driver->compatible_with);
	else
		return -1;
}

int GetParentRomSetIndex(const game_driver *driver)
{
	int nParentIndex = GetGameNameIndex(driver->parent);

	if( nParentIndex >= 0)
	{
		if ((driver_list::driver(nParentIndex).flags & MACHINE_IS_BIOS_ROOT) == 0)
			return nParentIndex;
	}

	return -1;
}

int GetGameNameIndex(const char *name)
{
	return driver_list::find(name);
}

/***************************************************************************
    Internal functions
 ***************************************************************************/

static void SetMainTitle(void)
{
	char buffer[100];
	snprintf(buffer, ARRAY_LENGTH(buffer), "%s %s", MAMEUINAME, GetVersionString());
	win_set_window_text_utf8(hMain,buffer);
}

static void memory_error(const char *message)
{
	win_message_box_utf8(hMain, message, emulator_info::get_appname(), MB_OK);
	exit(-1);
}

static BOOL Win32UI_init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	extern int mame_validitychecks(int game);
	WNDCLASS wndclass;
	RECT rect;
	int i = 0, nSplitterCount = 0;
	extern const FOLDERDATA g_folderData[];
	extern const FILTER_ITEM g_filterList[];
	LONG common_control_version = GetCommonControlVersion();
	TCHAR* t_inpdir = NULL;
	LONG_PTR l=0;

	dprintf("about to init options\n");
	if (!OptionsInit())
		return FALSE;
	dprintf("options loaded\n");
	//win_message_box_utf8(hMain, "test", emulator_info::get_appname(), MB_OK);

	// create the memory pool
	mameui_pool = pool_alloc_lib(memory_error);

	// custom per-game icons
	icon_index = (int*)pool_malloc_lib(mameui_pool, sizeof(int) * driver_list::total());
	memset(icon_index, '\0', sizeof(int) * driver_list::total());

	// set up window class
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = MameWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = DLGWINDOWEXTRA;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAMEUI_ICON));
	wndclass.hCursor       = NULL;
	wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_UI_MENU);
	wndclass.lpszClassName = TEXT("MainClass");

	RegisterClass(&wndclass);

	DevView_RegisterClass();

	InitCommonControls();

	// Are we using an Old comctl32.dll?
	dprintf("common controlversion %ld %ld\n", common_control_version >> 16, common_control_version & 0xffff);

	oldControl = (common_control_version < PACKVERSION(4,71));
	xpControl = (common_control_version >= PACKVERSION(6,0));
	if (oldControl)
	{
		char buf[] = MAMEUINAME " has detected an old version of comctl32.dll.\n\n"
					"Various features are not available without an updated DLL.\n\n"
					"Would you like to continue without using the new features?\n";

		win_message_box_utf8(0, buf, MAMEUINAME " Outdated comctl32.dll Error", MB_OK | MB_ICONWARNING);
		return false;
	}

	HelpInit();

	// should be able to get rid of this directory stuff soon
	t_inpdir = ui_wstring_from_utf8(GetInpDir());
	if( ! t_inpdir )
		return false;

	_tcscpy(last_directory,t_inpdir);
	free(t_inpdir);
	hMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, NULL);
	if (hMain == NULL)
	{
		dprintf("error creating main dialog, aborting\n");
		return false;
	}

	s_pWatcher = DirWatcher_Init(hMain, WM_MAME32_FILECHANGED);
	if (s_pWatcher)
	{
		DirWatcher_Watch(s_pWatcher, 0, GetRomDirs(), TRUE);
		DirWatcher_Watch(s_pWatcher, 1, GetSampleDirs(), TRUE);
	}

	SetMainTitle();
	hTabCtrl = GetDlgItem(hMain, IDC_SSTAB);

	{
		struct TabViewOptions opts;

		static const struct TabViewCallbacks s_tabviewCallbacks =
		{
			GetShowTabCtrl,			// pfnGetShowTabCtrl
			SetCurrentTab,			// pfnSetCurrentTab
			GetCurrentTab,			// pfnGetCurrentTab
			SetShowTab,				// pfnSetShowTab
			GetShowTab,				// pfnGetShowTab

			GetImageTabShortName,	// pfnGetTabShortName
			GetImageTabLongName,	// pfnGetTabLongName
			UpdateScreenShot		// pfnOnSelectionChanged
		};

		memset(&opts, 0, sizeof(opts));
		opts.pCallbacks = &s_tabviewCallbacks;
		opts.nTabCount = MAX_TAB_TYPES;

		if (!SetupTabView(hTabCtrl, &opts))
			return false;
	}

	/* subclass history window */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_HISTORY), GWLP_WNDPROC);
	g_lpHistoryWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_HISTORY), GWLP_WNDPROC, (LONG_PTR)HistoryWndProc);

	/* subclass picture frame area */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_SSFRAME), GWLP_WNDPROC);
	g_lpPictureFrameWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_SSFRAME), GWLP_WNDPROC, (LONG_PTR)PictureFrameWndProc);

	/* subclass picture area */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_SSPICTURE), GWLP_WNDPROC);
	g_lpPictureWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_SSPICTURE), GWLP_WNDPROC, (LONG_PTR)PictureWndProc);

	/* Load the pic for the default screenshot. */
	hMissing_bitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT));

	/* Stash hInstance for later use */
	hInst = hInstance;

	s_hToolBar   = InitToolbar(hMain);
	hStatusBar = InitStatusBar(hMain);
	hProgWnd   = InitProgressBar(hStatusBar);

	main_resize_items[0].u.hwnd = s_hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;

	/* In order to handle 'Large Fonts' as the Windows
     * default setting, we need to make the dialogs small
     * enough to fit in our smallest window size with
     * large fonts, then resize the picker, tab and button
     * controls to fill the window, no matter which font
     * is currently set.  This will still look like bad
     * if the user uses a bigger default font than 125%
     * (Large Fonts) on the Windows display setting tab.
     *
     * NOTE: This has to do with Windows default font size
     * settings, NOT our picker font size.
     */

	GetClientRect(hMain, &rect);

	hTreeView = GetDlgItem(hMain, IDC_TREE);
	hwndList  = GetDlgItem(hMain, IDC_LIST);

	if (!InitSplitters())
		return FALSE;

	nSplitterCount = GetSplitterCount();
	for (i = 0; i < nSplitterCount; i++)
	{
		HWND hWnd = GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow);
		HWND hWndLeft = GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow);
		HWND hWndRight = GetDlgItem(hMain, g_splitterInfo[i].nRightWindow);

		AddSplitter(hWnd, hWndLeft, hWndRight, g_splitterInfo[i].pfnAdjust);
	}

	/* Initial adjustment of controls on the Picker window */
	ResizePickerControls(hMain);

	TabView_UpdateSelection(hTabCtrl);

	bDoGameCheck = GetGameCheck();
	idle_work    = TRUE;
	game_index   = 0;

	bShowTree      = GetShowFolderList();
	bShowToolBar   = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();
	bShowTabCtrl   = GetShowTabCtrl();

	CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(s_hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(s_hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, (bShowTabCtrl) ? MF_CHECKED : MF_UNCHECKED);

	if (oldControl)
	{
		EnableMenuItem(GetMenu(hMain), ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_GAME_PROPERTIES, MF_GRAYED);
		EnableMenuItem(GetMenu(hMain), ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	/* Init DirectDraw */
	if (!DirectDraw_Initialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	LoadBackgroundBitmap();

	dprintf("about to init tree\n");
	InitTree(g_folderData, g_filterList);
	dprintf("did init tree\n");

	/* Initialize listview columns */
	InitMessPicker();
	InitListView();
	SetFocus(hwndList);

	/* Reset the font */
	{
		LOGFONT logfont;

		GetListFont(&logfont);
		if (hFont != NULL)
		{
			//Cleanup old Font, otherwise we have a GDI handle leak
			DeleteFont(hFont);
		}
		hFont = CreateFontIndirect(&logfont);
		if (hFont != NULL)
			SetAllWindowsFont(hMain, &main_resize, hFont, FALSE);
	}

	/* Init DirectInput */
	if (!DirectInputInitialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return FALSE;
	}

	AdjustMetrics();
	UpdateSoftware();
	UpdateScreenShot();

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TAB_KEYS));

	/* clear keyboard state */
	KeyboardStateClear();

	if (GetJoyGUI() == TRUE)
	{
		g_pJoyGUI = &DIJoystick;
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI = NULL;
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI = NULL;

	if (GetHideMouseOnStartup())
	{
		/*  For some reason the mouse is centered when a game is exited, which of
            course causes a WM_MOUSEMOVE event that shows the mouse. So we center
            it now, before the startup coords are initialised, and that way the mouse
            will still be hidden when exiting from a game (i hope) :) */
		SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2);

		// Then hide it
		ShowCursor(FALSE);
	}

	dprintf("about to show window\n");

	nCmdShow = GetWindowState();
	if (nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
	{
		nCmdShow = SW_RESTORE;
	}

	if (GetRunFullScreen())
	{
		LONG lMainStyle;

		// Remove menu
		SetMenu(hMain,NULL);

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);

		nCmdShow = SW_MAXIMIZE;
	}

	ShowWindow(hMain, nCmdShow);


	switch (GetViewMode())
	{
	case VIEW_LARGE_ICONS :
		SetView(ID_VIEW_LARGE_ICON);
		break;
	case VIEW_SMALL_ICONS :
		SetView(ID_VIEW_SMALL_ICON);
		break;
	case VIEW_INLIST :
		SetView(ID_VIEW_LIST_MENU);
		break;
	case VIEW_REPORT :
		SetView(ID_VIEW_DETAIL);
		break;
	case VIEW_GROUPED :
	default :
		SetView(ID_VIEW_GROUPED);
		break;
	}

	if (GetCycleScreenshot() > 0)
	{
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to Seconds
	}

	return true;
}


static void Win32UI_exit()
{
	MySoftwareListClose();

	if (g_pJoyGUI != NULL)
		g_pJoyGUI->exit();

	/* Free GDI resources */
	if (hMain) {
		DeleteObject(hMain);
		hMain = NULL;
	}

	if (hMissing_bitmap)
	{
		DeleteBitmap(hMissing_bitmap);
		hMissing_bitmap = NULL;
	}

	if (hBackground)
	{
		DeleteBitmap(hBackground);
		hBackground = NULL;
	}

	if (hPALbg)
	{
		DeletePalette(hPALbg);
		hPALbg = NULL;
	}

	if (hFont)
	{
		DeleteFont(hFont);
		hFont = NULL;
	}

	DestroyIcons();

	DestroyAcceleratorTable(hAccel);

	DirectInputClose();
	DirectDraw_Close();

	SetSavedFolderID(GetCurrentFolderID());
	SaveGameListOptions();
	//SaveDefaultOptions();   causes all changes to ini\mess.ini to be reverted
	SaveOptions();

	FreeFolders();

    /* DestroyTree(hTreeView); */

	FreeScreenShot();

	OptionsExit();

	HelpExit();

	pool_free_lib(mameui_pool);
	mameui_pool = NULL;
}

static LRESULT CALLBACK MameWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MINMAXINFO	*mminfo;
	int 		i = 0;
	TCHAR szClass[128];
	BOOL res = 0;

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
		if (hBackground && (HWND)lParam == GetDlgItem(hMain, IDC_HISTORY))
		{
			static HBRUSH hBrush=0;
			HDC hDC=(HDC)wParam;
			LOGBRUSH lb;

			if (hBrush)
				DeleteBrush(hBrush);
			lb.lbStyle  = BS_HOLLOW;
			hBrush = CreateBrushIndirect(&lb);
			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, GetListFontColor());
			return (LRESULT) hBrush;
		}
		break;

	case WM_INITDIALOG:
		/* Initialize info for resizing subitems */
		GetClientRect(hWnd, &main_resize.rect);
		return TRUE;

	case WM_SETFOCUS:
		SetFocus(hwndList);
		break;

	case WM_SETTINGCHANGE:
		AdjustMetrics();
		return 0;

	case WM_SIZE:
		OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(wParam));
		return TRUE;

	case WM_MENUSELECT:
		return Statusbar_MenuSelect(hWnd, wParam, lParam);

	case MM_PLAY_GAME:
		MamePlayGame();
		return TRUE;

	case WM_INITMENUPOPUP:
		UpdateMenu(GetMenu(hWnd));
		break;

	case WM_CONTEXTMENU:
		if (HandleTreeContextMenu(hWnd, wParam, lParam)
		||	HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;

	case WM_COMMAND:
		return MameCommand(hWnd,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

	case WM_GETMINMAXINFO:
		/* Don't let the window get too small; it can break resizing */
		mminfo = (MINMAXINFO *) lParam;
		mminfo->ptMinTrackSize.x = MIN_WIDTH;
		mminfo->ptMinTrackSize.y = MIN_HEIGHT;
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case JOYGUI_TIMER:
			PollGUIJoystick();
			break;
		case SCREENSHOT_TIMER:
			TabView_CalculateNextTab(hTabCtrl);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;
		default:
			break;
		}
		return TRUE;

	case WM_CLOSE:
		{
			/* save current item */
			RECT rect;
			AREA area;
			int nItem;
			WINDOWPLACEMENT wndpl;
			UINT state;

			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hMain, &wndpl);
			state = wndpl.showCmd;

			/* Restore the window before we attempt to save parameters,
             * This fixed the lost window on startup problem, among other problems
             */
			if (state == SW_MINIMIZE || state == SW_SHOWMINIMIZED || state == SW_MAXIMIZE)
			{
				if( wndpl.flags & WPF_RESTORETOMAXIMIZED || state == SW_MAXIMIZE)
					state = SW_MAXIMIZE;
				else
				{
					state = SW_RESTORE;
					ShowWindow(hWnd, SW_RESTORE);
				}
			}
			for (i = 0; i < GetSplitterCount(); i++)
				SetSplitterPos(i, nSplitterOffset[i]);
			SetWindowState(state);

			for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
				Picker_SaveColumnWidths(GetDlgItem(hMain, s_nPickers[i]));

			GetWindowRect(hWnd, &rect);
			area.x		= rect.left;
			area.y		= rect.top;
			area.width	= rect.right  - rect.left;
			area.height = rect.bottom - rect.top;
			SetWindowArea(&area);

			/* Save the users current game options and default game */
			nItem = Picker_GetSelectedItem(hwndList);
			SetDefaultGame(ModifyThe(driver_list::driver(nItem).name));

			/* hide window to prevent orphan empty rectangles on the taskbar */
			/* ShowWindow(hWnd,SW_HIDE); */
			DestroyWindow( hWnd );

			/* PostQuitMessage(0); */
			break;
		}

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

		/*
          Check to see if the mouse has been moved by the user since
          startup. I'd like this checking to be done only in the
          main WinProc (here), but somehow the WM_MOUSEDOWN messages
          are eaten up before they reach MameWindowProc. That's why
          there is one check for each of the subclassed windows too.

          POSSIBLE BUGS:
          I've included this check in the subclassed windows, but a
          mose move in either the title bar, the menu, or the
          toolbar will not generate a WM_MOUSEOVER message. At least
          not one that I know how to pick up. A solution could maybe
          be to subclass those too, but that's too much work :)
        */

	case WM_MOUSEMOVE:
	{
		if (MouseHasBeenMoved())
			ShowCursor(TRUE);

		if (g_listview_dragging)
			MouseMoveListViewDrag(MAKEPOINTS(lParam));
		else
			/* for splitters */
			OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;
	}

	case WM_LBUTTONUP:
	    if (g_listview_dragging)
		    ButtonUpListViewDrag(MAKEPOINTS(lParam));
		else
		   /* for splitters */
		   OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_NOTIFY:
		/* Where is this message intended to go */
		{
			LPNMHDR lpNmHdr = (LPNMHDR)lParam;

			/* Fetch tooltip text */
			if (lpNmHdr->code == TTN_NEEDTEXT)
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
				CopyToolTipText(lpttt);
			}

			if (lpNmHdr->hwndFrom == hTreeView)
				return TreeViewNotify(lpNmHdr);

			GetClassName(lpNmHdr->hwndFrom, szClass, sizeof(szClass) / sizeof(szClass[0]));
			if (!_tcscmp(szClass, TEXT("SysListView32")))
				return Picker_HandleNotify(lpNmHdr);
			if (!_tcscmp(szClass, TEXT("SysTabControl32")))
				return TabView_HandleNotify(lpNmHdr);
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;

			GetClassName(lpDis->hwndItem, szClass, sizeof(szClass) / sizeof(szClass[0]));
			if (!_tcscmp(szClass, TEXT("SysListView32")))
				Picker_HandleDrawItem(GetDlgItem(hMain, lpDis->CtlID), lpDis);
		}
		break;

	case WM_MEASUREITEM :
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;

		// tell the list view that each row (item) should be just taller than our font

		//DefWindowProc(hWnd, message, wParam, lParam);
		//dprintf("default row height calculation gives %u\n",lpmis->itemHeight);

		TEXTMETRIC tm;
		HDC hDC = GetDC(NULL);
		HFONT hFontOld = (HFONT)SelectObject(hDC,hFont);

		GetTextMetrics(hDC,&tm);

		lpmis->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
		if (lpmis->itemHeight < 17)
			lpmis->itemHeight = 17;
		//dprintf("we would do %u\n",tm.tmHeight + tm.tmExternalLeading + 1);
		SelectObject(hDC,hFontOld);
		ReleaseDC(NULL,hDC);

		return TRUE;
	}

	case WM_MAME32_FILECHANGED:
		{
			char szFileName[256];
			char *s;
			int nGameIndex;
			int (*pfnGetAuditResults)(int driver_index) = NULL;
			void (*pfnSetAuditResults)(int driver_index, int audit_results) = NULL;

			switch(HIWORD(wParam))
			{
				case 0:
					pfnGetAuditResults = GetRomAuditResults;
					pfnSetAuditResults = SetRomAuditResults;
					break;
				case 1:
					pfnGetAuditResults = GetSampleAuditResults;
					pfnSetAuditResults = SetSampleAuditResults;
					break;
			}

			if (pfnGetAuditResults && pfnSetAuditResults)
			{
				int nParentIndex = -1;

				snprintf(szFileName, sizeof(szFileName), "%s", (LPCSTR) lParam);
				s = strchr(szFileName, '.');
				if (s)
					*s = '\0';
				s = strchr(szFileName, '\\');
				if (s)
					*s = '\0';

				for (nGameIndex = 0; nGameIndex  < driver_list::total(); nGameIndex++)
				{
					for (nParentIndex = nGameIndex; nGameIndex == -1; nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex)))
					{
						if (core_stricmp(driver_list::driver(nParentIndex).name, szFileName)==0)
						{
							if (pfnGetAuditResults(nGameIndex) != UNKNOWN)
							{
								pfnSetAuditResults(nGameIndex, UNKNOWN);
								PostMessage(hMain, WM_MAME32_AUDITGAME, wParam, nGameIndex);
							}
							break;
						}
					}
				}
			}
		}
		break;

	case WM_MAME32_AUDITGAME:
		{
			LV_FINDINFO lvfi;

			int nGameIndex = lParam;

			switch(HIWORD(wParam))
			{
				case 0:
					MameUIVerifyRomSet(nGameIndex, 0);
					break;
				case 1:
					MameUIVerifySampleSet(nGameIndex);
					break;
			}

			memset(&lvfi, 0, sizeof(lvfi));
			lvfi.flags	= LVFI_PARAM;
			lvfi.lParam = nGameIndex;

			i = ListView_FindItem(hwndList, -1, &lvfi);
			if (i != -1)
			{
				res = ListView_RedrawItems(hwndList, i, i);
			}
		}
		break;

	default:

		break;
	}
	res++;
	return DefWindowProc(hWnd, message, wParam, lParam);
}

static int HandleKeyboardGUIMessage(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR: /* List-View controls use this message for searching the items "as user types" */
			//MessageBox(NULL,"wm_char message arrived","TitleBox",MB_OK);
			return TRUE;

		case WM_KEYDOWN:
			KeyboardKeyDown(0, wParam, lParam);
			return TRUE;

		case WM_KEYUP:
			KeyboardKeyUp(0, wParam, lParam);
			return TRUE;

		case WM_SYSKEYDOWN:
			KeyboardKeyDown(1, wParam, lParam);
			return TRUE;

		case WM_SYSKEYUP:
			KeyboardKeyUp(1, wParam, lParam);
			return TRUE;
	}

	return FALSE;	/* message not processed */
}

static BOOL PumpMessage()
{
	MSG msg;

	if (!GetMessage(&msg, NULL, 0, 0))
		return FALSE;

	if (IsWindow(hMain))
	{
		BOOL absorbed_key = FALSE;
		if (GetKeyGUI())
			absorbed_key = HandleKeyboardGUIMessage(msg.hwnd, msg.message,
													msg.wParam, msg.lParam);
		else
			absorbed_key = TranslateAccelerator(hMain, hAccel, &msg);

		if (!absorbed_key)
		{
			if (!IsDialogMessage(hMain, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return TRUE;
}

static BOOL FolderCheck(void)
{
	char *pDescription = NULL;
	int nGameIndex = 0;
	int i=0;
	int iStep = 0;
	LV_FINDINFO lvfi;
	int nCount = ListView_GetItemCount(hwndList);
	BOOL changed = FALSE;
	BOOL res = 0;

	MSG msg;
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		res = ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		SetRomAuditResults(nGameIndex, UNKNOWN);
	}
	if( nCount > 0)
		ProgressBarShow();
	else
		return FALSE;
	if( nCount < 100 )
		iStep = 100 / nCount;
	else
		iStep = nCount/100;
	UpdateListView();
	UpdateWindow(hMain);
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask	 = LVIF_PARAM;
		res = ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		if (GetRomAuditResults(nGameIndex) == UNKNOWN)
		{
			MameUIVerifyRomSet(nGameIndex, 0);
			changed = TRUE;
		}

		if (GetSampleAuditResults(nGameIndex) == UNKNOWN)
		{
			MameUIVerifySampleSet(nGameIndex);
			changed = TRUE;
		}

		lvfi.flags	= LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		i = ListView_FindItem(hwndList, -1, &lvfi);
		if (changed && i != -1)
		{
			res = ListView_RedrawItems(hwndList, i, i);
			while( PeekMessage( &msg, hwndList, 0, 0, PM_REMOVE ) != 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		changed = FALSE;
		if ((i % iStep) == 0)
			ProgressBarStepParam(i, nCount);
	}
	ProgressBarHide();
	pDescription = ModifyThe(driver_list::driver(Picker_GetSelectedItem(hwndList)).description);
	SetStatusBarText(0, pDescription);
	UpdateStatusBar();
	res++;
	return TRUE;
}

static BOOL GameCheck(void)
{
	LV_FINDINFO lvfi;
	int i = 0;
	BOOL changed = FALSE;
	BOOL res = 0;

	if (game_index == 0)
		ProgressBarShow();

	if (game_index >= driver_list::total())
	{
		bDoGameCheck = FALSE;
		ProgressBarHide();
		ResetWhichGamesInFolders();
		ResetListView(); // reset the list after F5
		return FALSE;
	}

	if (GetRomAuditResults(game_index) == UNKNOWN)
	{
		MameUIVerifyRomSet(game_index, 0);
		changed = TRUE;
	}

	if (GetSampleAuditResults(game_index) == UNKNOWN)
	{
		MameUIVerifySampleSet(game_index);
		changed = TRUE;
	}

	lvfi.flags	= LVFI_PARAM;
	lvfi.lParam = game_index;

	i = ListView_FindItem(hwndList, -1, &lvfi);
	if (changed && i != -1)
		res = ListView_RedrawItems(hwndList, i, i);
	if ((game_index % progBarStep) == 0)
		ProgressBarStep();
	game_index++;
	res++;
	return changed;
}

static BOOL OnIdle(HWND hWnd)
{
	static int bFirstTime = TRUE;
	static int bResetList = TRUE;

	char *pDescription = NULL;
	int driver_index = 0;

	if (bFirstTime)
	{
		bResetList = FALSE;
		bFirstTime = FALSE;
	}
	if (bDoGameCheck)
	{
		bResetList |= GameCheck();
		return idle_work;
	}
	// NPW 17-Jun-2003 - Commenting this out because it is redundant
	// and it causes the game to reset back to the original game after an F5
	// refresh
	//driver_index = GetGameNameIndex(GetDefaultGame());
	//SetSelectedPickItem(driver_index);

	// in case it's not found, get it back
	driver_index = Picker_GetSelectedItem(hwndList);

	pDescription = ModifyThe(driver_list::driver(driver_index).description);
	SetStatusBarText(0, pDescription);
	idle_work = FALSE;
	UpdateStatusBar();
	bFirstTime = TRUE;

// don't need this any more 2014-01-26
//	if (!idle_work)
//		PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, TRUE),(LPARAM)NULL);
	return idle_work;
}

static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static BOOL firstTime = TRUE;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();
	if (firstTime == FALSE)
		OnSizeSplitter(hMain);
	//firstTime = FALSE;
	/* Update the splitters structures as appropriate */
	RecalcSplitters();
	if (firstTime == FALSE)
		ResizePickerControls(hMain);
	firstTime = FALSE;
	UpdateScreenShot();
}



static HWND GetResizeItemWindow(HWND hParent, const ResizeItem *ri)
{
	HWND hControl;
	if (ri->type == RA_ID)
		hControl = GetDlgItem(hParent, ri->u.id);
	else
		hControl = ri->u.hwnd;
	return hControl;
}



static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hTheFont, BOOL bRedraw)
{
	int i = 0;
	HWND hControl = 0;

	for (i = 0; r->items[i].type != RA_END; i++)
	{
		hControl = GetResizeItemWindow(hParent, &r->items[i]);
		if (r->items[i].setfont)
		{
			SetWindowFont(hControl, hTheFont, bRedraw);
		}
		/* Take care of subcontrols, if appropriate */
		if (r->items[i].subwindow != NULL)
			SetAllWindowsFont(hControl, (const Resize*)r->items[i].subwindow, hTheFont, bRedraw);
	}
}



static void ResizeWindow(HWND hParent, Resize *r)
{
	int cmkindex = 0, dx = 0, dy = 0;
	HWND hControl = 0;
	RECT parent_rect, rect;
	const ResizeItem *ri;
	POINT p = {0, 0};

	if (hParent == NULL)
		return;

	/* Calculate change in width and height of parent window */
	GetClientRect(hParent, &parent_rect);
	dy = parent_rect.bottom - r->rect.bottom;
	dx = parent_rect.right - r->rect.right;
	ClientToScreen(hParent, &p);

	while (r->items[cmkindex].type != RA_END)
	{
		int width = 0, height = 0;
		ri = &r->items[cmkindex];
		if (ri->type == RA_ID)
			hControl = GetDlgItem(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == NULL)
		{
			cmkindex++;
			continue;
		}

		/* Get control's rectangle relative to parent */
		GetWindowRect(hControl, &rect);
		OffsetRect(&rect, -p.x, -p.y);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;
		//Sanity Check the child rect
		if (parent_rect.top > rect.top)
			rect.top = parent_rect.top;
		if (parent_rect.left > rect.left)
			rect.left = parent_rect.left;
		if (parent_rect.bottom < rect.bottom) {
			rect.bottom = parent_rect.bottom;
			//ensure we have at least a minimal height
			rect.top = rect.bottom - height;
			if (rect.top < parent_rect.top) {
				rect.top = parent_rect.top;
			}
		}
		if (parent_rect.right < rect.right) {
			rect.right = parent_rect.right;
			//ensure we have at least a minimal width
			rect.left = rect.right - width;
			if (rect.left < parent_rect.left) {
				rect.left = parent_rect.left;
			}
		}
		MoveWindow(hControl, rect.left, rect.top,
				   (rect.right - rect.left),
				   (rect.bottom - rect.top), TRUE);

		/* Take care of subcontrols, if appropriate */
		if (ri->subwindow != NULL)
			ResizeWindow(hControl, (Resize*)ri->subwindow);

		cmkindex++;
	}

	/* Record parent window's new location */
	memcpy(&r->rect, &parent_rect, sizeof(RECT));
}

static void ProgressBarShow()
{
	RECT rect;
	int  widths[2] = {150, -1};

	if (driver_list::total() < 100)
		progBarStep = 100 / driver_list::total();
	else
		progBarStep = (driver_list::total() / 100);

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
	SendMessage(hProgWnd, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, driver_list::total()));
	SendMessage(hProgWnd, PBM_SETSTEP, (WPARAM)progBarStep, 0);
	SendMessage(hProgWnd, PBM_SETPOS, 0, 0);

	StatusBar_GetItemRect(hStatusBar, 1, &rect);

	MoveWindow(hProgWnd, rect.left, rect.top,
			   rect.right - rect.left,
			   rect.bottom - rect.top, TRUE);

	bProgressShown = TRUE;
}

static void ProgressBarHide()
{
	RECT rect;
	int  widths[4];
	HDC  hDC = 0;
	SIZE size;
	int  numParts = 4;

	if (hProgWnd == NULL)
	{
		return;
	}

	hDC = GetDC(hProgWnd);

	ShowWindow(hProgWnd, SW_HIDE);

	GetTextExtentPoint32(hDC, TEXT("MMX"), 3, &size);
	widths[3] = size.cx;
	GetTextExtentPoint32(hDC, TEXT("MMMM games"), 10, &size);
	widths[2] = size.cx;
	//Just specify 24 instead of 30, gives us sufficient space to display the message, and saves some space
	GetTextExtentPoint32(hDC, TEXT("Screen flip support is missing"), 24, &size);
	widths[1] = size.cx;

	ReleaseDC(hProgWnd, hDC);

	widths[0] = -1;
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);
	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);
	UpdateStatusBar();

	bProgressShown = FALSE;
}

static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {150, -1};

		SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
		StatusBar_GetItemRect(hStatusBar, 1, &rect);
		MoveWindow(hProgWnd, rect.left, rect.top,
				   rect.right  - rect.left,
				   rect.bottom - rect.top, TRUE);
	}
	else
	{
		ProgressBarHide();
	}
}

static void ProgressBarStepParam(int iGameIndex, int nGameCount)
{
	SetStatusBarTextF(0, "Game search %d%% complete",((iGameIndex + 1) * 100) / nGameCount);
	if (iGameIndex == 0)
		ShowWindow(hProgWnd, SW_SHOW);
	SendMessage(hProgWnd, PBM_STEPIT, 0, 0);
}

static void ProgressBarStep()
{
	ProgressBarStepParam(game_index, driver_list::total());
}

static HWND InitProgressBar(HWND hParent)
{
	RECT rect;

	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	rect.left += 150;

	return CreateWindowEx(WS_EX_STATICEDGE,
			PROGRESS_CLASS,
			TEXT("Progress Bar"),
			WS_CHILD | PBS_SMOOTH,
			rect.left,
			rect.top,
			rect.right	- rect.left,
			rect.bottom - rect.top,
			hParent,
			NULL,
			hInst,
			NULL);
}

static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int   i = 0;
	int   iButton = lpttt->hdr.idFrom;
	static TCHAR String[1024];
	BOOL bConverted = FALSE;
	TCHAR* t_gameinfostatus;

	/* Map command ID to string index */
	for (i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			bConverted = TRUE;
			break;
		}
	}
	if( bConverted )
	{
		/* Check for valid parameter */
		if(iButton > NUM_TOOLTIPS)
		{
			_tcscpy(String,TEXT("Invalid Button Index"));
		}
		else
		{
			_tcscpy(String,szTbStrings[iButton]);
		}
	}
	else if ( iButton <= 2 )
	{
		//Statusbar
		SendMessage(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 200);
		if( iButton != 1)
			SendMessage(hStatusBar, SB_GETTEXT, (WPARAM)iButton, (LPARAM)&String );
		else {
			//for first pane we get the Status directly, to get the line breaks
			t_gameinfostatus = ui_wstring_from_utf8( GameInfoStatus(Picker_GetSelectedItem(hwndList), FALSE));
			if( !t_gameinfostatus )
				return;
			_tcscpy(String, t_gameinfostatus);
			free(t_gameinfostatus);
		}
	}
	else
		_tcscpy(String,TEXT("Invalid Button Index"));

	lpttt->lpszText = String;
}

static HWND InitToolbar(HWND hParent)
{
	HWND hToolBar = CreateToolbarEx(hParent,
						   WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
						   CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
						   1,
						   8,
						   hInst,
						   IDB_TOOLBAR,
						   tbb,
						   NUM_TOOLBUTTONS,
						   16,
						   16,
						   0,
						   0,
						   sizeof(TBBUTTON));
	RECT rect;
	int idx = 0;
	int iPosX = 0, iPosY = 0, iHeight = 0;

	// get Edit Control position
	idx = SendMessage(hToolBar, TB_BUTTONCOUNT, (WPARAM)0, (LPARAM)0) - 1;
	SendMessage(hToolBar, TB_GETITEMRECT, (WPARAM)idx, (LPARAM)&rect);
	iPosX = rect.right + 10;
	iPosY = rect.top + 1;
	iHeight = rect.bottom - rect.top - 2;

	// create Edit Control
	win_create_window_ex_utf8( 0L, "Edit", SEARCH_PROMPT, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT,
					iPosX, iPosY, 200, iHeight, hToolBar, (HMENU)ID_TOOLBAR_EDIT, hInst, NULL );

	return hToolBar;
}

static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = GetMenu(hParent);

	popstr[0].hMenu    = 0;
	popstr[0].uiString = 0;
	popstr[1].hMenu    = hMenu;
	popstr[1].uiString = IDS_UI_FILE;
	popstr[2].hMenu    = GetSubMenu(hMenu, 1);
	popstr[2].uiString = IDS_VIEW_TOOLBAR;
	popstr[3].hMenu    = 0;
	popstr[3].uiString = 0;

	return CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
							  CCS_BOTTOM | SBARS_SIZEGRIP | SBT_TOOLTIPS,
							  TEXT("Ready"),
							  hParent,
							  2);
}


static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT  fuFlags	= (UINT)HIWORD(wParam);
	HMENU hMainMenu = NULL;
	int   iMenu 	= 0;

	/* Handle non-system popup menu descriptions. */
	if (  (fuFlags & MF_POPUP)
	&&	(!(fuFlags & MF_SYSMENU)))
	{
		for (iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if ((HMENU)lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break ;
			}
		}
	}

	if (hMainMenu)
	{
		/* Display helpful text in status bar */
		MenuHelp(WM_MENUSELECT, wParam, lParam, hMainMenu, hInst, hStatusBar, (UINT *)&popstr[iMenu]);
	}
	else
	{
		UINT nZero = 0;
		MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, hInst, hStatusBar, &nZero);
	}

	return 0;
}

static void UpdateStatusBar()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int games_shown = 0;
	int i = -1;

	if (!lpFolder)
		return;

	while (1)
	{
		i = FindGame(lpFolder,i+1);
		if (i == -1)
			break;

		if (!GameFiltered(i, lpFolder->m_dwFlags))
			games_shown++;
	}

	/* Show number of games in the current 'View' in the status bar */
	SetStatusBarTextF(2, g_szGameCountString, games_shown);

	i = Picker_GetSelectedItem(hwndList);

	if (games_shown == 0)
		DisableSelection();
	else
	{
		const char* pStatus = GameInfoStatus(i, FALSE);
		SetStatusBarText(1, pStatus);
	}
}

static void UpdateHistory(void)
{
	HDC hDC;
	RECT rect;
	TEXTMETRIC tm ;
	int nLines = 0, nLineHeight = 0;
	//DWORD dwStyle = GetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_STYLE);
	have_history = FALSE;

	if (GetSelectedPick() >= 0)
	{
		char *histText = GetGameHistory(Picker_GetSelectedItem(hwndList));

		have_history = (histText && histText[0]) ? TRUE : FALSE;
		win_set_window_text_utf8(GetDlgItem(hMain, IDC_HISTORY), histText);
	}

	if (have_history && GetShowScreenShot()
		&& ((TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
			(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ))
	{
		Edit_GetRect(GetDlgItem(hMain, IDC_HISTORY),&rect);
		nLines = Edit_GetLineCount(GetDlgItem(hMain, IDC_HISTORY) );
		hDC = GetDC(GetDlgItem(hMain, IDC_HISTORY));
		GetTextMetrics (hDC, &tm);
		nLineHeight = tm.tmHeight - tm.tmInternalLeading;
		if( ( (rect.bottom - rect.top) / nLineHeight) < (nLines) )
		{
			//more than one Page, so show Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY), SB_VERT, 0, nLines, TRUE);
		}
		else
		{
			//hide Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY),SB_VERT, 0, 0, TRUE);

		}
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_SHOW);
	}
	else
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_HIDE);

}


static void DisableSelection()
{
	MENUITEMINFO mmi;
	HMENU hMenu = GetMenu(hMain);
	BOOL prev_have_selection = have_selection;

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = (TCHAR *) TEXT("&Play");
	mmi.cch = _tcslen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);

	SetStatusBarText(0, "No Selection");
	SetStatusBarText(1, "");
	SetStatusBarText(3, "");

	have_selection = FALSE;

	if (prev_have_selection != have_selection)
		UpdateScreenShot();
}

static void EnableSelection(int nGame)
{
	TCHAR buf[200];
	const char * pText;
	MENUITEMINFO mmi;
	HMENU hMenu = GetMenu(hMain);
	TCHAR* t_description;

	MyFillSoftwareList(nGame, FALSE);

	t_description = ui_wstring_from_utf8(ConvertAmpersandString(ModifyThe(driver_list::driver(nGame).description)));
	if( !t_description )
		return;

	_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), g_szPlayGameString, t_description);
	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = buf;
	mmi.cch = _tcslen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mmi);

	pText = ModifyThe(driver_list::driver(nGame).description);
	SetStatusBarText(0, pText);
	/* Add this game's status to the status bar */
	pText = GameInfoStatus(nGame, FALSE);
	SetStatusBarText(1, pText);

	// Show number of software_list items in box at bottom right.
	int items = SoftwareList_GetNumberOfItems();
	if (items)
	{
		sprintf((char *)pText, "%d", items);
		SetStatusBarText(3, pText);
	}
	else
		SetStatusBarText(3, "");

	/* If doing updating game status */

	EnableMenuItem(hMenu, ID_FILE_PLAY, MF_ENABLED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_ENABLED);

	if (!oldControl)
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	if (bProgressShown && bListReady == TRUE)
		SetDefaultGame(ModifyThe(driver_list::driver(nGame).name));

	have_selection = TRUE;

	UpdateScreenShot();

	free(t_description);
}

static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y)
{
	RECT rcClient;
	HRGN rgnBitmap = 0;
	HPALETTE hPAL = 0;
	HDC hDC = GetDC(hWnd);
	int i = 0, j = 0;
	HDC htempDC = 0;
	HBITMAP oldBitmap = 0;

	/* x and y are offsets within the background image that should be at 0,0 in hWnd */

	/* So we don't paint over the control's border */
	GetClientRect(hWnd, &rcClient);

	htempDC = CreateCompatibleDC(hDC);
	oldBitmap = (HBITMAP)SelectObject(htempDC, hBackground);

	if (hRgn == NULL)
	{
		/* create a region of our client area */
		rgnBitmap = CreateRectRgnIndirect(&rcClient);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteBitmap(rgnBitmap);
	}
	else
	{
		/* use the passed in region */
		SelectClipRgn(hDC, hRgn);
	}

	hPAL = GetBackgroundPalette();
	if (hPAL == NULL)
		hPAL = CreateHalftonePalette(hDC);

	if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		SelectPalette(htempDC, hPAL, FALSE);
		RealizePalette(htempDC);
	}

	for (i = rcClient.left-x; i < rcClient.right; i += bmDesc.bmWidth)
		for (j = rcClient.top-y; j < rcClient.bottom; j += bmDesc.bmHeight)
			BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

	SelectObject(htempDC, oldBitmap);
	DeleteDC(htempDC);

	if (GetBackgroundPalette() == NULL)
	{
		DeletePalette(hPAL);
		hPAL = NULL;
	}

	ReleaseDC(hWnd, hDC);
}

static LPCSTR GetCloneParentName(int nItem)
{
	int nParentIndex = -1;

	if (DriverIsClone(nItem) == TRUE)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));
		if( nParentIndex >= 0)
			return ModifyThe(driver_list::driver(nParentIndex).description);
	}
	return "";
}

static BOOL TreeViewNotify(LPNMHDR nm)
{
	switch (nm->code)
	{
		case TVN_SELCHANGED :
		{
			HTREEITEM hti = TreeView_GetSelection(hTreeView);
			TVITEM tvi;

			tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
			tvi.hItem = hti;

			if (TreeView_GetItem(hTreeView, &tvi))
			{
				SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
				if (bListReady)
				{
					ResetListView();
					MessUpdateSoftwareList();
					UpdateScreenShot();
				}
			}
			return TRUE;
		}
		case TVN_BEGINLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

			if (folder->m_dwFlags & F_CUSTOM)
			{
				// user can edit custom folder names
				g_in_treeview_edit = TRUE;
				return FALSE;
			}
			// user can't edit built in folder names
			return TRUE;
		}
		case TVN_ENDLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;
			char* utf8_szText;
			BOOL result = 0;
			g_in_treeview_edit = FALSE;

			if (ptvdi->item.pszText == NULL || _tcslen(ptvdi->item.pszText) == 0)
				return FALSE;

			utf8_szText = ui_utf8_from_wstring(ptvdi->item.pszText);
			if( !utf8_szText )
				return FALSE;

			result = TryRenameCustomFolder(folder, utf8_szText);

			free(utf8_szText);

			return result;
		}
	}
	return FALSE;
}



static void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	// Right button was clicked on header

	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	lastColumnClick = nColumn;
	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	DestroyMenu(hMenuLoad);
}



char* ConvertAmpersandString(const char *s)
{
	/* takes a string and changes any ampersands to double ampersands,
       for setting text of window controls that don't allow us to disable
       the ampersand underlining.
     */
	/* returns a static buffer--use before calling again */

	static char buf[256];
	char *ptr;

	ptr = buf;
	while (*s)
	{
		if (*s == '&')
			*ptr++ = *s;
		*ptr++ = *s++;
	}
	*ptr = 0;

	return buf;
}

static int GUI_seq_pressed(const input_seq *seq)
{
	int codenum = 0;
	int res = 1;
	int invert = 0;
	int count = 0;

	for (codenum = 0; (*seq)[codenum] != input_seq::end_code; codenum++)
	{
		input_code code = (*seq)[codenum];

		if (code == input_seq::not_code)
			invert = !invert;

		else if (code == input_seq::or_code)
		{
			if (res && count)
				return 1;
			res = 1;
			count = 0;
		}
		else
		{
			if (res)
			{
				if ((keyboard_state[(int)(code.item_id())] != 0) == invert)
					res = 0;
			}
			invert = 0;
			++count;
		}
	}
	return res && count;
}

static void check_for_GUI_action(void)
{
	int i = 0;

	for (i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		const input_seq *is = &(GUISequenceControl[i].is);

		if (GUI_seq_pressed(is))
		{
			dprintf("seq =%s pressed\n", GUISequenceControl[i].name);
			switch (GUISequenceControl[i].func_id)
			{
			case ID_GAME_AUDIT:
			case ID_GAME_PROPERTIES:
			case ID_CONTEXT_FILTERS:
			case ID_UI_START:
				KeyboardStateClear(); /* beacuse whe won't receive KeyUp mesage when we loose focus */
				break;
			default:
				break;
			}
			SendMessage(hMain, WM_COMMAND, GUISequenceControl[i].func_id, 0);
		}
	}
}

static void KeyboardStateClear(void)
{
	memset(keyboard_state, 0, sizeof(keyboard_state));
	dprintf("keyboard gui state cleared.\n");
}


static void KeyboardKeyDown(int syskey, int vk_code, int special)
{
	int i = 0, found = 0;
	int icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_RALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_RCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_RSHIFT.item_id());
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_LALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_LCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_LSHIFT.item_id());
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < ARRAY_LENGTH(win_key_trans_table); i++)
		{
			if ( vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		dprintf("VK_code pressed not found =  %i\n",vk_code);
		//MessageBox(NULL,"keydown message arrived not processed","TitleBox",MB_OK);
		return;
	}
	dprintf("VK_code pressed found =  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special);
	keyboard_state[icode] = true;
	check_for_GUI_action();
}

static void KeyboardKeyUp(int syskey, int vk_code, int special)
{
	int i, found = 0;
	int icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_RALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_RCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_RSHIFT.item_id());
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_LALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_LCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_LSHIFT.item_id());
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < ARRAY_LENGTH(win_key_trans_table); i++)
		{
			if (vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		dprintf("VK_code released not found =  %i\n",vk_code);
		//MessageBox(NULL,"keyup message arrived not processed","TitleBox",MB_OK);
		return;
	}
	keyboard_state[icode] = false;
	dprintf("VK_code released found=  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special );
	check_for_GUI_action();
}

static void PollGUIJoystick()
{
	// For the exec timer, will keep track of how long the button has been pressed
	static int exec_counter = 0;
	const char* exec_command;
	TCHAR* t_exec_command;

	if (in_emulation)
		return;

	if (g_pJoyGUI == NULL)
		return;

	g_pJoyGUI->poll_joysticks();


	// User pressed UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2),GetUIJoyUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_UP, 0);
	}

	// User pressed DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2),GetUIJoyDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_DOWN, 0);
	}

	// User pressed LEFT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2),GetUIJoyLeft(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_LEFT, 0);
	}

	// User pressed RIGHT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2),GetUIJoyRight(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_RIGHT, 0);
	}

	// User pressed START GAME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyStart(0), GetUIJoyStart(1), GetUIJoyStart(2),GetUIJoyStart(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_START, 0);
	}

	// User pressed PAGE UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageUp(0), GetUIJoyPageUp(1), GetUIJoyPageUp(2),GetUIJoyPageUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_PGUP, 0);
	}

	// User pressed PAGE DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageDown(0), GetUIJoyPageDown(1), GetUIJoyPageDown(2),GetUIJoyPageDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_PGDOWN, 0);
	}

	// User pressed HOME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHome(0), GetUIJoyHome(1), GetUIJoyHome(2),GetUIJoyHome(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HOME, 0);
	}

	// User pressed END
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyEnd(0), GetUIJoyEnd(1), GetUIJoyEnd(2),GetUIJoyEnd(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_END, 0);
	}

	// User pressed CHANGE SCREENSHOT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoySSChange(0), GetUIJoySSChange(1), GetUIJoySSChange(2),GetUIJoySSChange(3))))
	{
		SendMessage(hMain, WM_COMMAND, IDC_SSFRAME, 0);
	}

	// User pressed SCROLL HISTORY UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryUp(0), GetUIJoyHistoryUp(1), GetUIJoyHistoryUp(2),GetUIJoyHistoryUp(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_UP, 0);
	}

	// User pressed SCROLL HISTORY DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryDown(0), GetUIJoyHistoryDown(1), GetUIJoyHistoryDown(2),GetUIJoyHistoryDown(3))))
	{
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_DOWN, 0);
	}

	// User pressed EXECUTE COMMANDLINE
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyExec(0), GetUIJoyExec(1), GetUIJoyExec(2),GetUIJoyExec(3))))
	{
		if (++exec_counter >= GetExecWait()) // Button has been pressed > exec timeout
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si;

			// Reset counter
			exec_counter = 0;

			ZeroMemory( &si, sizeof(si) );
			ZeroMemory( &pi, sizeof(pi) );
			si.dwFlags = STARTF_FORCEONFEEDBACK;
			si.cb = sizeof(si);

			exec_command = GetExecCommand();
			t_exec_command = ui_wstring_from_utf8(exec_command);
			if( !t_exec_command )
				return;
			CreateProcess(NULL, t_exec_command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

			free(t_exec_command);

			// We will not wait for the process to finish cause it might be a background task
			// The process won't get closed when MAME32 closes either.

			// But close the handles cause we won't need them anymore. Will not close process.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	else
	{
		// Button has been released within the timeout period, reset the counter
		exec_counter = 0;
	}
}

static void SetView(int menu_id)
{
	BOOL force_reset = FALSE;
	int i = 0;

	// first uncheck previous menu item, check new one
	CheckMenuRadioItem(GetMenu(hMain), ID_VIEW_LARGE_ICON, ID_VIEW_GROUPED, menu_id, MF_CHECKED);
	ToolBar_CheckButton(s_hToolBar, menu_id, MF_CHECKED);

	if (Picker_GetViewID(hwndList) == VIEW_GROUPED || menu_id == ID_VIEW_GROUPED)
	{
		// this changes the sort order, so redo everything
		force_reset = TRUE;
	}

	for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetViewID(GetDlgItem(hMain, s_nPickers[i]), menu_id - ID_VIEW_LARGE_ICON);

	if (force_reset)
	{
		for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
			Picker_Sort(GetDlgItem(hMain, s_nPickers[i]));
	}
}

static void ResetListView()
{
	int i = -1;
	int current_game = 0;
	LV_ITEM lvi;
	BOOL no_selection = FALSE;
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	HRESULT res = 0;
	BOOL b_res = 0;

	if (!lpFolder)
		return;

	/* If the last folder was empty, no_selection is TRUE */
	if (have_selection == FALSE)
		no_selection = TRUE;

	current_game = Picker_GetSelectedItem(hwndList);

	SetWindowRedraw(hwndList,FALSE);

	b_res = ListView_DeleteAllItems(hwndList);

	// hint to have it allocate it all at once
	ListView_SetItemCount(hwndList,driver_list::total());

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.stateMask = 0;

	do
	{
		/* Add the games that are in this folder */
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (GameFiltered(i, lpFolder->m_dwFlags))
				continue;

			lvi.iItem    = i;
			lvi.iSubItem = 0;
			lvi.lParam   = i;
			lvi.pszText  = LPSTR_TEXTCALLBACK;
			lvi.iImage   = I_IMAGECALLBACK;
			res = ListView_InsertItem(hwndList, &lvi);
		}
	} while (i != -1);

	Picker_Sort(hwndList);

	if (bListReady)
	{
		/* If last folder was empty, select the first item in this folder */
		if (no_selection)
			Picker_SetSelectedPick(hwndList, 0);
		else
			Picker_SetSelectedItem(hwndList, current_game);
	}

	/*RS Instead of the Arrange Call that was here previously on all Views
         We now need to set the ViewMode for SmallIcon again,
         for an explanation why, see SetView*/
	if (GetViewMode() == VIEW_SMALL_ICONS)
		SetView(ID_VIEW_SMALL_ICON);

	SetWindowRedraw(hwndList, TRUE);

	UpdateStatusBar();
	res++;
	b_res++;
}

static void UpdateGameList(BOOL bUpdateRomAudit, BOOL bUpdateSampleAudit)
{
	for (int i = 0; i < driver_list::total(); i++)
	{
		if (bUpdateRomAudit && DriverUsesRoms(i))
			SetRomAuditResults(i, UNKNOWN);
		if (bUpdateSampleAudit && DriverUsesSamples(i))
			SetSampleAuditResults(i, UNKNOWN);
	}

	idle_work	 = TRUE;
	bDoGameCheck = TRUE;
	game_index	 = 0;

	ReloadIcons();

	// Let REFRESH also load new background if found
	LoadBackgroundBitmap();
	InvalidateRect(hMain,NULL,TRUE);
	Picker_ResetIdle(hwndList);
}

UINT_PTR CALLBACK CFHookProc(
  HWND hdlg,      // handle to dialog box
  UINT uiMsg,     // message identifier
  WPARAM wParam,  // message parameter
  LPARAM lParam   // message parameter
)
{
	int iIndex = 0, i = 0;
	COLORREF cCombo=0, cList=0;
	switch (uiMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)TEXT("Custom"));
			iIndex = SendDlgItemMessage(hdlg, cmb4, CB_GETCOUNT, 0, 0);
			cList = GetListFontColor();
			SendDlgItemMessage(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex-1,(LPARAM)cList );
			for( i = 0; i< iIndex; i++)
			{
				cCombo = SendDlgItemMessage(hdlg, cmb4, CB_GETITEMDATA,(WPARAM)i,0 );
				if( cList == cCombo)
				{
					SendDlgItemMessage(hdlg, cmb4, CB_SETCURSEL,(WPARAM)i,0 );
					break;
				}
			}
			break;
		case WM_COMMAND:
			if( LOWORD(wParam) == cmb4)
			{
				switch (HIWORD(wParam))
				{
					case CBN_SELCHANGE:  // The color ComboBox changed selection
						iIndex = (int)SendDlgItemMessage(hdlg, cmb4, CB_GETCURSEL, 0, 0L);
						if( iIndex == SendDlgItemMessage(hdlg, cmb4, CB_GETCOUNT, 0, 0)-1)
						{
							//Custom color selected
							cList = GetListFontColor();
							PickColor(&cList);
							SendDlgItemMessage(hdlg, cmb4, CB_DELETESTRING, iIndex, 0);
							SendDlgItemMessage(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)TEXT("Custom"));
							SendDlgItemMessage(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex,(LPARAM)cList);
							SendDlgItemMessage(hdlg, cmb4, CB_SETCURSEL,(WPARAM)iIndex,0 );
							return TRUE;
						}
				}
			}
			break;
	}
	return FALSE;
}

static void PickFont(void)
{
	LOGFONT font;
	CHOOSEFONT cf;
	TCHAR szClass[128];
	HWND hWnd;
	HRESULT res = 0;
	BOOL b_res = 0;

	GetListFont(&font);
	font.lfQuality = DEFAULT_QUALITY;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner   = hMain;
	cf.lpLogFont   = &font;
	cf.lpfnHook = &CFHookProc;
	cf.rgbColors   = GetListFontColor();
	cf.Flags	   = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_ENABLEHOOK;
	if (!ChooseFont(&cf))
		return;

	SetListFont(&font);
	if (hFont != NULL)
		DeleteFont(hFont);
	hFont = CreateFontIndirect(&font);
	if (hFont != NULL)
	{
		COLORREF textColor = cf.rgbColors;

		if (textColor == RGB(255,255,255))
			textColor = RGB(240, 240, 240);

		SetAllWindowsFont(hMain, &main_resize, hFont, TRUE);

		hWnd = GetWindow(hMain, GW_CHILD);
		while(hWnd)
		{
			if (GetClassName(hWnd, szClass, sizeof(szClass) / sizeof(szClass[0])))
			{
				if (!_tcscmp(szClass, TEXT("SysListView32")))
					b_res = ListView_SetTextColor(hWnd, textColor);
				else
				if (!_tcscmp(szClass, TEXT("SysTreeView32")))
					res = TreeView_SetTextColor(hTreeView, textColor);
			}
			hWnd = GetWindow(hWnd, GW_HWNDNEXT);
		}
		SetListFontColor(cf.rgbColors);
		ResetListView();
	}
	res++;
	b_res++;
}

static void PickColor(COLORREF *cDefault)
{
	CHOOSECOLOR cc;
	COLORREF choice_colors[16];
	int i = 0;

	for (i=0;i<16;i++)
		choice_colors[i] = GetCustomColor(i);

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner   = hMain;
	cc.rgbResult   = *cDefault;
	cc.lpCustColors = choice_colors;
	cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
	if (!ChooseColor(&cc))
		return;
	for (i=0;i<16;i++)
		SetCustomColor(i,choice_colors[i]);
	*cDefault = cc.rgbResult;
}

static void PickCloneColor(void)
{
	COLORREF cClonecolor;
	cClonecolor = GetListCloneColor();
	PickColor( &cClonecolor);
	SetListCloneColor(cClonecolor);
	InvalidateRect(hwndList,NULL,FALSE);
}

static BOOL MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	int i = 0;
	LPTREEFOLDER folder;
	char* utf8_szFile;
	BOOL res = 0;

	switch (id)
	{
	case ID_FILE_PLAY:
		MamePlayGame();
		return TRUE;

	case ID_FILE_PLAY_RECORD:
		MamePlayRecordGame();
		return TRUE;

	case ID_FILE_PLAY_BACK:
		MamePlayBackGame();
		return TRUE;

	case ID_FILE_PLAY_RECORD_WAVE:
		MamePlayRecordWave();
		return TRUE;

	case ID_FILE_PLAY_RECORD_MNG:
		MamePlayRecordMNG();
		return TRUE;

	case ID_FILE_PLAY_RECORD_AVI:
		MamePlayRecordAVI();
		return TRUE;

	case ID_FILE_LOADSTATE :
		MameLoadState();
		return TRUE;

	case ID_FILE_AUDIT:
		AuditDialog(hMain, 1);
		ResetWhichGamesInFolders();
		ResetListView();
		SetFocus(hwndList);
		return TRUE;

	case ID_FILE_AUDIT_X:
		AuditDialog(hMain, 2);
		ResetWhichGamesInFolders();
		ResetListView();
		SetFocus(hwndList);
		return TRUE;

	case ID_FILE_EXIT:
		PostMessage(hMain, WM_CLOSE, 0, 0);
		return TRUE;

	case ID_VIEW_LARGE_ICON:
		SetView(ID_VIEW_LARGE_ICON);
		return TRUE;

	case ID_VIEW_SMALL_ICON:
		SetView(ID_VIEW_SMALL_ICON);
		ResetListView();
		return TRUE;

	case ID_VIEW_LIST_MENU:
		SetView(ID_VIEW_LIST_MENU);
		return TRUE;

	case ID_VIEW_DETAIL:
		SetView(ID_VIEW_DETAIL);
		return TRUE;

	case ID_VIEW_GROUPED:
		SetView(ID_VIEW_GROUPED);
		return TRUE;

	/* Arrange Icons submenu */
	case ID_VIEW_BYGAME:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_GAMES);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYDIRECTORY:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_DIRECTORY);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYMANUFACTURER:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_MANUFACTURER);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTIMESPLAYED:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_PLAYED);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTYPE:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_TYPE);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYYEAR:
		SetSortReverse(FALSE);
		SetSortColumn(COLUMN_YEAR);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_FOLDERS:
		bShowTree = !bShowTree;
		SetShowFolderList(bShowTree);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		UpdateScreenShot();
		break;

	case ID_VIEW_TOOLBARS:
		bShowToolBar = !bShowToolBar;
		SetShowToolBar(bShowToolBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(s_hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_STATUS:
		bShowStatusBar = !bShowStatusBar;
		SetShowStatusBar(bShowStatusBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_PAGETAB:
		bShowTabCtrl = !bShowTabCtrl;
		SetShowTabCtrl(bShowTabCtrl);
		ShowWindow(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		InvalidateRect(hMain,NULL,TRUE);
		break;

        /*
          Switches to fullscreen mode. No check mark handling
          for this item cause in fullscreen mode the menu won't
          be visible anyways.
        */
	case ID_VIEW_FULLSCREEN:
		SwitchFullScreenMode();
		break;

	case ID_TOOLBAR_EDIT:
		{
			std::string buf;
			HWND hToolbarEdit;

			buf = win_get_window_text_utf8(hwndCtl);
			switch (codeNotify)
			{
			case TOOLBAR_EDIT_ACCELERATOR_PRESSED:
				hToolbarEdit = GetDlgItem( s_hToolBar, ID_TOOLBAR_EDIT);
				SetFocus(hToolbarEdit);
				break;
			case EN_CHANGE:
				//put search routine here first, add a 200ms timer later.
				if ((!_stricmp(buf.c_str(), SEARCH_PROMPT) && !_stricmp(g_SearchText, "")) ||
					(!_stricmp(g_SearchText, SEARCH_PROMPT) && !_stricmp(buf.c_str(), "")))
				{
					strcpy(g_SearchText, buf.c_str());
				}
				else
				{
					strcpy(g_SearchText, buf.c_str());
					ResetListView();
				}
				break;
			case EN_SETFOCUS:
				if (!_stricmp(buf.c_str(), SEARCH_PROMPT))
					win_set_window_text_utf8(hwndCtl, "");
				break;
			case EN_KILLFOCUS:
				if (*buf.c_str() == 0)
					win_set_window_text_utf8(hwndCtl, SEARCH_PROMPT);
				break;
			}
		}
		break;

	case ID_GAME_AUDIT:
		InitGameAudit(0);
		if (!oldControl)
		{
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), OPTIONS_GAME, -1, Picker_GetSelectedItem(hwndList), AUDIT_PAGE);
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
	   break;

	/* ListView Context Menu */
	case ID_CONTEXT_ADD_CUSTOM:
	{
		DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CUSTOM_FILE),
			hMain,AddCustomFileDialogProc,Picker_GetSelectedItem(hwndList));
		SetFocus(hwndList);
		break;
	}

	case ID_CONTEXT_REMOVE_CUSTOM:
	{
		RemoveCurrentGameCustomFolder();
		break;
	}

	/* Tree Context Menu */
	case ID_CONTEXT_FILTERS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_FILTERS), hMain, FilterDialogProc) == TRUE)
			ResetListView();
		SetFocus(hwndList);
		return TRUE;

		// ScreenShot Context Menu
		// select current tab
	case ID_VIEW_TAB_SCREENSHOT :
	case ID_VIEW_TAB_FLYER :
	case ID_VIEW_TAB_CABINET :
	case ID_VIEW_TAB_MARQUEE :
	case ID_VIEW_TAB_TITLE :
	case ID_VIEW_TAB_CONTROL_PANEL :
	case ID_VIEW_TAB_PCB :
	case ID_VIEW_TAB_HISTORY:
		if (id == ID_VIEW_TAB_HISTORY && GetShowTab(TAB_HISTORY) == FALSE)
			break;

		TabView_SetCurrentTab(hTabCtrl, id - ID_VIEW_TAB_SCREENSHOT);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

		// toggle tab's existence
	case ID_TOGGLE_TAB_SCREENSHOT :
	case ID_TOGGLE_TAB_FLYER :
	case ID_TOGGLE_TAB_CABINET :
	case ID_TOGGLE_TAB_MARQUEE :
	case ID_TOGGLE_TAB_TITLE :
	case ID_TOGGLE_TAB_CONTROL_PANEL :
	case ID_TOGGLE_TAB_PCB :
	case ID_TOGGLE_TAB_HISTORY :
	{
		int toggle_flag = id - ID_TOGGLE_TAB_SCREENSHOT;

		if (AllowedToSetShowTab(toggle_flag,!GetShowTab(toggle_flag)) == FALSE)
		{
			// attempt to hide the last tab
			// should show error dialog? hide picture area? or ignore?
			break;
		}

		SetShowTab(toggle_flag,!GetShowTab(toggle_flag));

		TabView_Reset(hTabCtrl);

		if (TabView_GetCurrentTab(hTabCtrl) == toggle_flag && GetShowTab(toggle_flag) == FALSE)
		{
			// we're deleting the tab we're on, so go to the next one
			TabView_CalculateNextTab(hTabCtrl);
		}


		// Resize the controls in case we toggled to another history
		// mode (and the history control needs resizing).

		ResizePickerControls(hMain);
		UpdateScreenShot();

		TabView_UpdateSelection(hTabCtrl);

		break;
	}

	/* Header Context Menu */
	case ID_SORT_ASCENDING:
		SetSortReverse(FALSE);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_SORT_DESCENDING:
		SetSortReverse(TRUE);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (DialogBox(GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_COLUMNS), hMain, ColumnDialogProc) == TRUE)
			ResetColumnDisplay(FALSE);
		SetFocus(hwndList);
		return TRUE;

	/* View Menu -  MESSUI: not used any more 2014-01-26 */
	case ID_VIEW_LINEUPICONS:
		if( codeNotify == FALSE)
			ResetListView();
		else
		{
			/*it was sent after a refresh (F5) was done, we only reset the View if "available" is the selected folder
              as it doesn't affect the others. */
			folder = GetSelectedFolder();
			if( folder )
			{
				if (folder->m_nFolderId == FOLDER_AVAILABLE )
					ResetListView();

			}
		}
		break;

	case ID_GAME_PROPERTIES:
		if (!oldControl)
		{
			folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
			InitPropertyPage(hInst, hwnd, GetSelectedPickItemIcon(), OPTIONS_GAME, folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
			{
				extern BOOL g_bModifiedSoftwarePaths;
				if (g_bModifiedSoftwarePaths)
				{
					g_bModifiedSoftwarePaths = FALSE;
					MessUpdateSoftwareList();
				}
			}
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_PROPERTIES:
		if (!oldControl)
		{
			OPTIONS_TYPE curOptType = OPTIONS_SOURCE;
			folder = GetSelectedFolder();
			if(folder->m_nFolderId == FOLDER_VECTOR)
				curOptType = OPTIONS_VECTOR;

			InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), curOptType, folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_SOURCEPROPERTIES:
	{
		int game = Picker_GetSelectedItem(hwndList);
		folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(game));
		InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), OPTIONS_SOURCE, folder->m_nFolderId, game);
		UpdateStatusBar();
		SetFocus(hwndList);
		return true;
		//folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
		//InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), (folder->m_nFolderId == FOLDER_VECTOR) ? OPTIONS_VECTOR : OPTIONS_SOURCE , folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
		//UpdateStatusBar();
		//break;
	}

	case ID_FOLDER_VECTORPROPERTIES:
		if (!oldControl)
		{
			folder = GetFolderByID( FOLDER_VECTOR );
			InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), OPTIONS_VECTOR, folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_AUDIT:
		FolderCheck();
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_VIEW_PICTURE_AREA :
		ToggleScreenShot();
		break;

	case ID_VIEW_SOFTWARE_AREA :
		ToggleSoftware();
		break;

	case ID_UPDATE_GAMELIST:
		UpdateGameList(TRUE, TRUE);
		break;

	case ID_OPTIONS_FONT:
		PickFont();
		return TRUE;

	case ID_OPTIONS_CLONE_COLOR:
		PickCloneColor();
		return TRUE;

	case ID_OPTIONS_DEFAULTS:
		/* Check the return value to see if changes were applied */
		if (!oldControl)
		{
			InitDefaultPropertyPage(hInst, hwnd);
		}
		SetFocus(hwndList);
		return TRUE;

	case ID_OPTIONS_DIR:
		{
			int nResult = DialogBox(GetModuleHandle(NULL),
					MAKEINTRESOURCE(IDD_DIRECTORIES),
					hMain,
					DirectoriesDialogProc);

			SaveDefaultOptions();
			SaveOptions();

			BOOL bUpdateRoms    = ((nResult & DIRDLG_ROMS) == DIRDLG_ROMS) ? TRUE : FALSE;
			BOOL bUpdateSamples = ((nResult & DIRDLG_SAMPLES) == DIRDLG_SAMPLES) ? TRUE : FALSE;
			BOOL bUpdateSoftware = ((nResult & DIRDLG_SL) == DIRDLG_SL) ? TRUE : FALSE;

			if (bUpdateSoftware)
				MessUpdateSoftwareList();

			if (s_pWatcher)
			{
				if (bUpdateRoms)
					DirWatcher_Watch(s_pWatcher, 0, GetRomDirs(), TRUE);
				if (bUpdateSamples)
					DirWatcher_Watch(s_pWatcher, 1, GetSampleDirs(), TRUE);
			}

			/* update game list */
			if (bUpdateRoms == TRUE || bUpdateSamples == TRUE)
				UpdateGameList(bUpdateRoms, bUpdateSamples);

			SetFocus(hwndList);
		}
		return TRUE;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RESET), hMain, ResetDialogProc) == TRUE)
		{
			// these may have been changed
			SaveDefaultOptions();
			SaveOptions();
			DestroyWindow(hwnd);
			PostQuitMessage(0);
		}
		else
		{
			ResetListView();
			SetFocus(hwndList);
		}
		return TRUE;

	case ID_OPTIONS_INTERFACE:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INTERFACE_OPTIONS),
				  hMain, InterfaceDialogProc);
		SaveOptions();

		KillTimer(hMain, SCREENSHOT_TIMER);
		if( GetCycleScreenshot() > 0)
		{
			SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL ); // Scale to seconds
		}

		return TRUE;

	case ID_OPTIONS_BG:
		{
			// More c++ stupidity with strings
			// Get the path from the existing filename; if no filename go to root
			TCHAR* t_bgdir = TEXT(".");
			const char *s = GetBgDir();
			std::string as;
			util::zippath_parent(as, s);
			size_t t1 = as.length()-1;
			if (as[t1] == '\\') as[t1]='\0';
			t1 = as.find(':');
			if (t1 > 0)
				t_bgdir = ui_wstring_from_utf8(as.c_str());

			OPENFILENAME OFN;
			static TCHAR szFile[MAX_PATH] = TEXT("\0");
			if( !t_bgdir )
				return FALSE;

			OFN.lStructSize       = sizeof(OPENFILENAME);
			OFN.hwndOwner         = hMain;
			OFN.hInstance         = 0;
			OFN.lpstrFilter       = TEXT("Image Files (*.png)\0*.PNG\0");
			OFN.lpstrCustomFilter = NULL;
			OFN.nMaxCustFilter    = 0;
			OFN.nFilterIndex      = 1;
			OFN.lpstrFile         = szFile;
			OFN.nMaxFile          = sizeof(szFile);
			OFN.lpstrFileTitle    = NULL;
			OFN.nMaxFileTitle     = 0;
			OFN.lpstrInitialDir   = t_bgdir;
			OFN.lpstrTitle        = TEXT("Select a Background Image");
			OFN.nFileOffset       = 0;
			OFN.nFileExtension    = 0;
			OFN.lpstrDefExt       = NULL;
			OFN.lCustData         = 0;
			OFN.lpfnHook          = NULL;
			OFN.lpTemplateName    = NULL;
			OFN.Flags             = OFN_NOCHANGEDIR | OFN_SHOWHELP | OFN_EXPLORER;

			if (GetOpenFileName(&OFN))
			{
				free(t_bgdir);
				utf8_szFile = ui_utf8_from_wstring(szFile);
				if( !utf8_szFile )
					return FALSE;

				// Make this file as the new default
				SetBgDir(utf8_szFile);

				// Display new background
				LoadBackgroundBitmap();
				InvalidateRect(hMain, NULL, TRUE);
				free(utf8_szFile);
				return TRUE;
			}
			free(t_bgdir);
		}
		break;

	case ID_HELP_ABOUT:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT),
				  hMain, AboutDialogProc);
		SetFocus(hwndList);
		return TRUE;

	case IDOK :
		/* cmk -- might need to check more codes here, not sure */
		if (codeNotify != EN_CHANGE && codeNotify != EN_UPDATE)
		{
			/* enter key */
			if (g_in_treeview_edit)
			{
				res = TreeView_EndEditLabelNow(hTreeView, FALSE);
				return TRUE;
			}
			else
				if (have_selection)
					MamePlayGame();
		}
		break;

	case IDCANCEL : /* esc key */
		if (g_in_treeview_edit)
			res = TreeView_EndEditLabelNow(hTreeView, TRUE);
		break;

	case IDC_PLAY_GAME :
		if (have_selection)
			MamePlayGame();
		break;

	case ID_UI_START:
		SetFocus(hwndList);
		MamePlayGame();
		break;

	case ID_UI_UP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
		break;

	case ID_UI_DOWN:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
		break;

	case ID_UI_PGUP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - ListView_GetCountPerPage(hwndList));
		break;

	case ID_UI_PGDOWN:
		if ( (GetSelectedPick() + ListView_GetCountPerPage(hwndList)) < ListView_GetItemCount(hwndList) )
			Picker_SetSelectedPick(hwndList,  GetSelectedPick() + ListView_GetCountPerPage(hwndList) );
		else
			Picker_SetSelectedPick(hwndList,  ListView_GetItemCount(hwndList)-1 );
		break;

	case ID_UI_HOME:
		Picker_SetSelectedPick(hwndList, 0);
		break;

	case ID_UI_END:
		Picker_SetSelectedPick(hwndList,  ListView_GetItemCount(hwndList)-1 );
		break;
	case ID_UI_LEFT:
		/* hmmmmm..... */
		SendMessage(hwndList,WM_HSCROLL, SB_LINELEFT, 0);
		break;

	case ID_UI_RIGHT:
		/* hmmmmm..... */
		SendMessage(hwndList,WM_HSCROLL, SB_LINERIGHT, 0);
		break;
	case ID_UI_HISTORY_UP:
		/* hmmmmm..... */
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEUP, 0);
		}
		break;

	case ID_UI_HISTORY_DOWN:
		/* hmmmmm..... */
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEDOWN, 0);
		}
		break;

	case IDC_SSFRAME:
		TabView_CalculateNextTab(hTabCtrl);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

	case ID_CONTEXT_SELECT_RANDOM:
		SetRandomPickItem();
		break;

	case ID_CONTEXT_RESET_PLAYSTATS:
		ResetPlayTime( Picker_GetSelectedItem(hwndList) );
		ResetPlayCount( Picker_GetSelectedItem(hwndList) );
		res = ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RENAME_CUSTOM :
		TreeView_EditLabel(hTreeView,TreeView_GetSelection(hTreeView));
		break;

	default:
		if (id >= ID_CONTEXT_SHOW_FOLDER_START && id < ID_CONTEXT_SHOW_FOLDER_END)
		{
			ToggleShowFolder(id - ID_CONTEXT_SHOW_FOLDER_START);
			break;
		}
		for (i = 0; g_helpInfo[i].nMenuItem > 0; i++)
		{
			if (g_helpInfo[i].nMenuItem == id)
			{
				printf("%X: %ls\n",g_helpInfo[i].bIsHtmlHelp, g_helpInfo[i].lpFile);
				if (i == 1) // get current whatsnew.txt from mamedev.org
				{
					std::string version = std::string(GetVersionString()); // turn version string into std
					version.erase(1,1); // take out the decimal point
					version.erase(4, std::string::npos); // take out the date
					std::string url = "http://mamedev.org/releases/whatsnew_" + version + ".txt"; // construct url
					std::wstring stemp = s2ws(url); // convert to wide string (yeah, typical c++ mess)
					LPCWSTR result = stemp.c_str(); // then convert to const wchar_t*
					ShellExecute(hMain, TEXT("open"), result, TEXT(""), NULL, SW_SHOWNORMAL); // show web page
				}
				else
				if (g_helpInfo[i].bIsHtmlHelp)
//					HelpFunction(hMain, g_helpInfo[i].lpFile, HH_DISPLAY_TOPIC, 0);
					ShellExecute(hMain, TEXT("open"), g_helpInfo[i].lpFile, TEXT(""), NULL, SW_SHOWNORMAL);
//				else
//					DisplayTextFile(hMain, g_helpInfo[i].lpFile);
				return FALSE;
			}
		}
		return MessCommand(hwnd, id, hwndCtl, codeNotify);
	}
	res++;
	return FALSE;
}

static void LoadBackgroundBitmap()
{
	HGLOBAL hDIBbg;

	if (hBackground)
	{
		DeleteBitmap(hBackground);
		hBackground = 0;
	}

	if (hPALbg)
	{
		DeletePalette(hPALbg);
		hPALbg = 0;
	}

	if (LoadDIBBG(&hDIBbg, &hPALbg)) // screenshot.c
	{
		HDC hDC = GetDC(hwndList);
		hBackground = DIBToDDB(hDC, hDIBbg, &bmDesc);
		GlobalFree(hDIBbg);
		ReleaseDC(hwndList, hDC);
	}
}

static void ResetColumnDisplay(BOOL first_time)
{
	int driver_index = 0;

	if (!first_time)
		Picker_ResetColumnDisplay(hwndList);

	ResetListView();

	driver_index = GetGameNameIndex(GetDefaultGame());
	Picker_SetSelectedItem(hwndList, driver_index);
}

static int GamePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	return GetIconForDriver(nItem);
}

static const TCHAR *GamePicker_GetItemString(HWND hwndPicker, int nItem, int nColumn,
	TCHAR *pszBuffer, UINT nBufferLength)
{
	const TCHAR *s = NULL;
	const char* utf8_s = NULL;
	char playtime_buf[256];

	switch(nColumn)
	{
		case COLUMN_GAMES:
			/* Driver description */
			utf8_s = ModifyThe(driver_list::driver(nItem).description);
			break;

		case COLUMN_ORIENTATION:
			utf8_s = DriverIsVertical(nItem) ? "Vertical" : "Horizontal";
			break;

#ifdef SHOW_COLUMN_ROMS
		case COLUMN_ROMS:
			utf8_s = GetAuditString(GetRomAuditResults(nItem));
			break;
#endif
#ifdef SHOW_COLUMN_SAMPLES
		case COLUMN_SAMPLES:
			/* Samples */
			if (DriverUsesSamples(nItem))
				utf8_s = GetAuditString(GetSampleAuditResults(nItem));
			else
				s = TEXT("-");
			break;
#endif
		case COLUMN_DIRECTORY:
			/* Driver name (directory) */
			utf8_s = driver_list::driver(nItem).name;
			break;

		case COLUMN_SRCDRIVERS:
			/* Source drivers */
			utf8_s = GetDriverFilename(nItem);
			break;

		case COLUMN_PLAYTIME:
			/* Source drivers */
			GetTextPlayTime(nItem, playtime_buf);
			utf8_s = playtime_buf;
			break;

		case COLUMN_TYPE:
			{
				machine_config config(*&driver_list::driver(nItem),MameUIGlobal());
				/* Vector/Raster */
				if (isDriverVector(&config))
					s = TEXT("Vector");
				else
					s = TEXT("Raster");
			}
			break;

		case COLUMN_TRACKBALL:
			/* Trackball */
			if (DriverUsesTrackball(nItem))
				s = TEXT("Yes");
			else
				s = TEXT("No");
			break;

		case COLUMN_PLAYED:
			/* times played */
			_sntprintf(pszBuffer, nBufferLength, TEXT("%i"), GetPlayCount(nItem));
			s = pszBuffer;
			break;

		case COLUMN_MANUFACTURER:
			/* Manufacturer */
			utf8_s = driver_list::driver(nItem).manufacturer;
			break;

		case COLUMN_YEAR:
			/* Year */
			utf8_s = driver_list::driver(nItem).year;
			break;

		case COLUMN_CLONE:
			utf8_s = GetCloneParentName(nItem);
			break;
	}

	if( utf8_s )
	{
		TCHAR* t_s = ui_wstring_from_utf8(utf8_s);
		if( !t_s )
			return s;

		_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_s);
		free(t_s);

		s = pszBuffer;
	}

	return s;
}

static void GamePicker_LeavingItem(HWND hwndPicker, int nItem)
{
	// leaving item
	// printf("leaving %s\n",driver_list::driver(nItem).name);
	g_szSelectedSoftware[0] = 0;
	g_szSelectedDevice[0] = 0;
	g_szSelectedItem[0] = 0;
}

static void GamePicker_EnteringItem(HWND hwndPicker, int nItem)
{
	// printf("entering %s\n",driver_list::driver(nItem).name);

	EnableSelection(nItem);
	MessReadMountedSoftware(nItem);
	// decide if it is valid to load a savestate
	if (driver_list::driver(nItem).flags & MACHINE_SUPPORTS_SAVE)
		EnableMenuItem(GetMenu(hMain), ID_FILE_LOADSTATE, MFS_ENABLED);
	else
		EnableMenuItem(GetMenu(hMain), ID_FILE_LOADSTATE, MFS_GRAYED);
}

static int GamePicker_FindItemParent(HWND hwndPicker, int nItem)
{
	return GetParentRomSetIndex(&driver_list::driver(nItem));
}

/* Initialize the Picker and List controls */
static void InitListView()
{
	LVBKIMAGE bki;
	//TCHAR path[MAX_PATH];
	TCHAR* t_bgdir;
	BOOL res = 0;

	static const struct PickerCallbacks s_gameListCallbacks =
	{
		SetSortColumn,				/* pfnSetSortColumn */
		GetSortColumn,				/* pfnGetSortColumn */
		SetSortReverse,				/* pfnSetSortReverse */
		GetSortReverse,				/* pfnGetSortReverse */
		SetViewMode,				/* pfnSetViewMode */
		GetViewMode,				/* pfnGetViewMode */
		SetColumnWidths,			/* pfnSetColumnWidths */
		GetColumnWidths,			/* pfnGetColumnWidths */
		SetColumnOrder,				/* pfnSetColumnOrder */
		GetColumnOrder,				/* pfnGetColumnOrder */
		SetColumnShown,				/* pfnSetColumnShown */
		GetColumnShown,				/* pfnGetColumnShown */
		GetOffsetClones,			/* pfnGetOffsetChildren */

		GamePicker_Compare,			/* pfnCompare */
		MamePlayGame,				/* pfnDoubleClick */
		GamePicker_GetItemString,	/* pfnGetItemString */
		GamePicker_GetItemImage,	/* pfnGetItemImage */
		GamePicker_LeavingItem,		/* pfnLeavingItem */
		GamePicker_EnteringItem,	/* pfnEnteringItem */
		BeginListViewDrag,			/* pfnBeginListViewDrag */
		GamePicker_FindItemParent,	/* pfnFindItemParent */
		OnIdle,							/* pfnIdle */
		GamePicker_OnHeaderContextMenu,	/* pfnOnHeaderContextMenu */
		GamePicker_OnBodyContextMenu	/* pfnOnBodyContextMenu */
	};

	struct PickerOptions opts;

	// subclass the list view
	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_gameListCallbacks;
	opts.nColumnCount = COLUMN_MAX;
	opts.ppszColumnNames = column_names;
	SetupPicker(hwndList, &opts);

	res = ListView_SetTextBkColor(hwndList, CLR_NONE);
	res = ListView_SetBkColor(hwndList, CLR_NONE);
	t_bgdir = ui_wstring_from_utf8(GetBgDir());
	if( !t_bgdir )
		return;

	bki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	bki.pszImage = t_bgdir; //path;
	if( hBackground )
		res = ListView_SetBkImage(hwndList, &bki);

	CreateIcons();

	ResetColumnDisplay(TRUE);

	// Allow selection to change the default saved game
	bListReady = TRUE;
	res++;
	free(t_bgdir);
}

static void AddDriverIcon(int nItem,int default_icon_index)
{
	HICON hIcon = 0;
	int nParentIndex = -1;

	/* if already set to rom or clone icon, we've been here before */
	if (icon_index[nItem] == 1 || icon_index[nItem] == 3)
		return;

	hIcon = LoadIconFromFile((char *)driver_list::driver(nItem).name);
	if (hIcon == NULL)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));
		if( nParentIndex >= 0)
		{
			hIcon = LoadIconFromFile((char *)driver_list::driver(nParentIndex).name);
			nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex));
			if (hIcon == NULL && nParentIndex >= 0)
				hIcon = LoadIconFromFile((char *)driver_list::driver(nParentIndex).name);
		}
	}

	if (hIcon != NULL)
	{
		int nIconPos = ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		if (nIconPos != -1)
			icon_index[nItem] = nIconPos;
		DestroyIcon(hIcon);
	}
	if (icon_index[nItem] == 0)
		icon_index[nItem] = default_icon_index;
}

static void DestroyIcons(void)
{
	if (hSmall != NULL)
	{
		ImageList_Destroy(hSmall);
		hSmall = NULL;
	}

	if (icon_index != NULL)
	{
		int i;
		for (i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	if (hLarge != NULL)
	{
		ImageList_Destroy(hLarge);
		hLarge = NULL;
	}

	if (hHeaderImages != NULL)
	{
		ImageList_Destroy(hHeaderImages);
		hHeaderImages = NULL;
	}

}

static void ReloadIcons(void)
{
	HICON hIcon;
	INT i = 0;

	// clear out all the images
	ImageList_RemoveAll(hSmall);
	ImageList_RemoveAll(hLarge);

	if (icon_index != NULL)
	{
		for (i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	for (i = 0; g_iconData[i].icon_name; i++)
	{
		hIcon = LoadIconFromFile((char *) g_iconData[i].icon_name);
		if (hIcon == NULL)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(g_iconData[i].resource));

		ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		DestroyIcon(hIcon);
	}
}

static DWORD GetShellLargeIconSize(void)
{
	DWORD  dwSize = 32, dwLength = 512, dwType = REG_SZ;
	TCHAR  szBuffer[512];
	HKEY   hKey;
	LONG   lRes = 0;
	LPTSTR tErrorMessage = NULL;

	/* Get the Key */
	lRes = RegOpenKey(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop\\WindowMetrics"), &hKey);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &tErrorMessage);
		MessageBox(GetMainWindow(), tErrorMessage, TEXT("Large shell icon size registry access"), MB_OK | MB_ICONERROR);
		LocalFree(tErrorMessage);
		return dwSize;
	}

	/* Save the last size */
	lRes = RegQueryValueEx(hKey, TEXT("Shell Icon Size"), NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &tErrorMessage);
		MessageBox(GetMainWindow(), tErrorMessage, TEXT("Large shell icon size registry query"), MB_OK | MB_ICONERROR);
		LocalFree(tErrorMessage);
		RegCloseKey(hKey);
		return dwSize;
	}

	dwSize = _ttol(szBuffer);
	if (dwSize < 32)
		dwSize = 32;

	if (dwSize > 48)
		dwSize = 48;

	/* Clean up */
	RegCloseKey(hKey);
	return dwSize;
}

static DWORD GetShellSmallIconSize(void)
{
	DWORD dwSize = ICONMAP_WIDTH;

	if (dwSize < 48)
	{
		if (dwSize < 32)
			dwSize = 16;
		else
			dwSize = 32;
	}
	else
	{
		dwSize = 48;
	}
	return dwSize;
}

// create iconlist for Listview control
static void CreateIcons(void)
{
	DWORD dwSmallIconSize = GetShellSmallIconSize();
	DWORD dwLargeIconSize = GetShellLargeIconSize();
	HICON hIcon;
	int icon_count = 0;
	DWORD dwStyle;
	int i = 0;
	int grow = 5000;

	while(g_iconData[icon_count].icon_name)
		icon_count++;

	// the current window style affects the sizing of the rows when changing
	// between list views, so put it in small icon mode temporarily while we associate
	// our image list
	//
	// using large icon mode instead kills the horizontal scrollbar when doing
	// full refresh, which seems odd (it should recreate the scrollbar when
	// set back to report mode, for example, but it doesn't).

	dwStyle = GetWindowLong(hwndList,GWL_STYLE);
	SetWindowLong(hwndList,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = ImageList_Create(dwSmallIconSize, dwSmallIconSize,
		ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (NULL == hSmall) {
		win_message_box_utf8(hwndList, "Cannot allocate small icon image list", "Allocation error - Exiting", IDOK);
		PostQuitMessage(0);
	}

	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize,
		ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (NULL == hLarge) {
		win_message_box_utf8(hwndList, "Cannot allocate large icon image list", "Allocation error - Exiting", IDOK);
		PostQuitMessage(0);
	}

	ReloadIcons();

	// Associate the image lists with the list view control.
	(void)ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
	(void)ListView_SetImageList(hwndList, hLarge, LVSIL_NORMAL);

	// restore our view
	SetWindowLong(hwndList,GWL_STYLE,dwStyle);

	CreateMessIcons();

	// Now set up header specific stuff
	hHeaderImages = ImageList_Create(8,8,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_UP));
	ImageList_AddIcon(hHeaderImages,hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_DOWN));
	ImageList_AddIcon(hHeaderImages,hIcon);

	for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetHeaderImageList(GetDlgItem(hMain, s_nPickers[i]), hHeaderImages);
}



static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem)
{
	int value = 0;  /* Default to 0, for unknown case */
	const char *name1 = NULL;
	const char *name2 = NULL;
	char file1[MAX_PATH];
	char file2[MAX_PATH];
	int nTemp1=0, nTemp2=0;

	switch (sort_subitem)
	{
	case COLUMN_GAMES:
		return core_stricmp(ModifyThe(driver_list::driver(index1).description),
			ModifyThe(driver_list::driver(index2).description));

	case COLUMN_ORIENTATION:
		nTemp1 = DriverIsVertical(index1) ? 1 : 0;
		nTemp2 = DriverIsVertical(index2) ? 1 : 0;
		value = nTemp1 - nTemp2;
		break;

#ifdef SHOW_COLUMN_SAMPLES
	case COLUMN_SAMPLES:
		nTemp1 = -1;
		if (DriverUsesSamples(index1))
		{
			int audit_result = GetSampleAuditResults(index1);
			if (IsAuditResultKnown(audit_result))
			{
				if (IsAuditResultYes(audit_result))
					nTemp1 = 1;
				else
					nTemp1 = 0;
			}
			else
				nTemp1 = 2;
		}

		nTemp2 = -1;
		if (DriverUsesSamples(index2))
		{
			int audit_result = GetSampleAuditResults(index1);
			if (IsAuditResultKnown(audit_result))
			{
				if (IsAuditResultYes(audit_result))
					nTemp2 = 1;
				else
					nTemp2 = 0;
			}
			else
				nTemp2 = 2;
		}
		value = nTemp2 - nTemp1;
		break;
#endif

	case COLUMN_DIRECTORY:
		value = core_stricmp(driver_list::driver(index1).name, driver_list::driver(index2).name);
		break;

	case COLUMN_SRCDRIVERS:
		strcpy(file1, GetDriverFilename(index1));
		strcpy(file2, GetDriverFilename(index2));
		value = core_stricmp(file1, file2);
		break;

	case COLUMN_PLAYTIME:
		value = GetPlayTime(index1) - GetPlayTime(index2);
		break;

	case COLUMN_TYPE:
		{
			machine_config config1(driver_list::driver(index1),MameUIGlobal());
			machine_config config2(driver_list::driver(index2),MameUIGlobal());

			value = isDriverVector(&config1) - isDriverVector(&config2);
		}
		break;

	case COLUMN_TRACKBALL:
		value = DriverUsesTrackball(index1) - DriverUsesTrackball(index2);
		break;

	case COLUMN_PLAYED:
		value = GetPlayCount(index1) - GetPlayCount(index2);
		break;

	case COLUMN_MANUFACTURER:
		value = core_stricmp(driver_list::driver(index1).manufacturer, driver_list::driver(index2).manufacturer);
		break;

	case COLUMN_YEAR:
		value = core_stricmp(driver_list::driver(index1).year, driver_list::driver(index2).year);
		break;

	case COLUMN_CLONE:
		name1 = GetCloneParentName(index1);
		name2 = GetCloneParentName(index2);

		if (*name1 == '\0')
			name1 = NULL;
		if (*name2 == '\0')
			name2 = NULL;

		if (NULL == name1 && NULL == name2)
			value = 0;
		else if (name2 == NULL)
			value = -1;
		else if (name1 == NULL)
			value = 1;
		else
			value = core_stricmp(name1, name2);
		break;
	}

	// Handle same comparisons here
	if (0 == value && COLUMN_GAMES != sort_subitem)
	{
		value = GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);
	}

	return value;
}

int GetSelectedPick()
{
	/* returns index of listview selected item */
	/* This will return -1 if not found */
	return ListView_GetNextItem(hwndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}

static HICON GetSelectedPickItemIcon()
{
	LV_ITEM lvi;
	BOOL res = 0;

	lvi.iItem = GetSelectedPick();
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	res = ListView_GetItem(hwndList, &lvi);
	res++;
	return ImageList_GetIcon(hLarge, lvi.iImage, ILD_TRANSPARENT);
}

static void SetRandomPickItem()
{
	int nListCount = ListView_GetItemCount(hwndList);

	if (nListCount > 0)
		Picker_SetSelectedPick(hwndList, rand() % nListCount);
}

BOOL CommonFileDialog(common_file_dialog_proc cfd, char *filename, int filetype)
{
	BOOL success;
	UINT16 i;
	OPENFILENAME ofn;
	std::string dirname;
	TCHAR* t_filename;
	TCHAR t_filename_buffer[MAX_PATH]  = {0, };
	char *utf8_filename;

	// convert the filename to UTF-8 and copy into buffer
	t_filename = ui_wstring_from_utf8(filename);
	if (t_filename != NULL)
	{
		_sntprintf(t_filename_buffer, ARRAY_LENGTH(t_filename_buffer), TEXT("%s"), t_filename);
		free(t_filename);
	}

	ofn.lStructSize       = sizeof(ofn);
	ofn.hwndOwner         = hMain;
	ofn.hInstance         = NULL;
	switch (filetype)
	{
	case FILETYPE_INPUT_FILES :
		ofn.lpstrFilter   = TEXT("input files (*.inp,*.zip,*.7z)\0*.inp;*.zip;*.7z\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("inp");
		dirname = GetInpDir();
		break;
	case FILETYPE_SAVESTATE_FILES :
		ofn.lpstrFilter   = TEXT("savestate files (*.sta)\0*.sta;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("sta");
		dirname = GetStateDir();
		break;
	case FILETYPE_WAVE_FILES :
		ofn.lpstrFilter   = TEXT("sounds (*.wav)\0*.wav;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("wav");
		dirname = GetImgDir();
		break;
	case FILETYPE_MNG_FILES :
		ofn.lpstrFilter   = TEXT("videos (*.mng)\0*.mng;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("mng");
		dirname = GetImgDir();
		break;
	case FILETYPE_AVI_FILES :
		ofn.lpstrFilter   = TEXT("videos (*.avi)\0*.avi;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("avi");
		dirname = GetImgDir();
		break;
	case FILETYPE_EFFECT_FILES :
		ofn.lpstrFilter   = TEXT("effects (*.png)\0*.png;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("png");
		dirname = GetArtDir();
		break;
	case FILETYPE_JOYMAP_FILES :
		ofn.lpstrFilter   = TEXT("maps (*.map,*.txt)\0*.map;*.txt;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("map");
		dirname = GetCtrlrDir();
		break;
	case FILETYPE_DEBUGSCRIPT_FILES :
		ofn.lpstrFilter   = TEXT("scripts (*.txt,*.dat)\0*.txt;*.dat;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("txt");
		dirname = GetInpDir();
		break;
	case FILETYPE_CHEAT_FILE :
		ofn.lpstrFilter   = TEXT("cheats (*.dat)\0*.dat;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("dat");
		dirname = ".";
		break;
	case FILETYPE_HISTORY_FILE :
		ofn.lpstrFilter   = TEXT("history (*.dat)\0*.dat;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("dat");
		dirname = ".";
		break;
	case FILETYPE_MAMEINFO_FILE :
		ofn.lpstrFilter   = TEXT("mameinfo (*.dat)\0*.dat;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("dat");
		dirname = ".";
		break;
	}
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 1;
	ofn.lpstrFile         = t_filename_buffer;
	ofn.nMaxFile          = ARRAY_LENGTH(t_filename_buffer);
	ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFileTitle     = 0;

	// Only want first directory
	i = dirname.find(";");
	if (i > 0)
		dirname.resize(i);
	if (dirname.empty())
		dirname = ".";
	ofn.lpstrInitialDir   = ui_wstring_from_utf8(dirname.c_str());

	ofn.lpstrTitle        = NULL;
	ofn.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL;

	success = cfd(&ofn);
	if (success)
	{
		//dprintf("got filename %s nFileExtension %u\n",filename,ofn.nFileExtension);
		/*GetDirectory(filename,last_directory,sizeof(last_directory));*/
	}

	utf8_filename = ui_utf8_from_wstring(t_filename_buffer);
	if (utf8_filename != NULL)
	{
		snprintf(filename, MAX_PATH, "%s", utf8_filename);
		free(utf8_filename);
	}

	return success;
}

void SetStatusBarText(int part_index, const char *message)
{
	TCHAR* t_message = ui_wstring_from_utf8(message);
	if( !t_message )
		return;
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM) part_index, (LPARAM)(LPCTSTR) win_tstring_strdup(t_message));
	free(t_message);
}

void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	SetStatusBarText(part_index, buf);
}

static void CLIB_DECL ATTR_PRINTF(1,2) MameMessageBox(const char *fmt, ...)
{
	char buf[2048];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	win_message_box_utf8(GetMainWindow(), buf, MAMEUINAME, MB_OK | MB_ICONERROR);
	va_end(va);
}

static void MamePlayBackGame()
{
	char filename[MAX_PATH];
	*filename = 0;

	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_INPUT_FILES))
	{
		osd_file::error fileerr;
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char bare_fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];
		char fname[MAX_PATH];
		play_options playopts;

		_splitpath(filename, drive, dir, bare_fname, ext);

		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		emu_file pPlayBack(MameUIGlobal().input_directory(), OPEN_FLAG_READ);
		fileerr = pPlayBack.open(fname);
		if (fileerr != osd_file::error::NONE)
		{
			MameMessageBox("Could not open '%s' as a valid input file.", filename);
			return;
		}

		// check for game name embedded in .inp header
		inp_header header;

		/* read the header and verify that it is a modern version; if not, print an error */
		if (!header.read(pPlayBack))
		{
			MameMessageBox("Input file is corrupt or invalid (missing header)");
			return;
		}
		if ((!header.check_magic()) || (header.get_majversion() != inp_header::MAJVERSION))
		{
			MameMessageBox("Input file invalid or in an older, unsupported format");
			return;
		}

		std::string const sysname = header.get_sysname();
		nGame = -1;
		for (int i = 0; i < driver_list::total(); i++) // find game and play it
		{
			if (driver_list::driver(i).name == sysname)
			{
				nGame = i;
				break;
			}
		}
		if (nGame == -1)
		{
			MameMessageBox("Game \"%s\" cannot be found", sysname.c_str());
			return;
		}

		memset(&playopts, 0, sizeof(playopts));
		playopts.playback = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MameLoadState()
{
	int nGame=0;
	char filename[MAX_PATH];
	char selected_filename[MAX_PATH];
	play_options playopts;

	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
	{
		strcpy(filename, driver_list::driver(nGame).name);
		strcpy(selected_filename, driver_list::driver(nGame).name);
	}
	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_SAVESTATE_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char ext[_MAX_EXT];

		char path[MAX_PATH];
		char fname[MAX_PATH];
		char bare_fname[_MAX_FNAME];
		char *state_fname;
		//int rc;

		_splitpath(filename, drive, dir, bare_fname, ext);

		// parse path
		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		{
			state_fname = filename;
		}

		emu_file pSaveState(MameUIGlobal().state_directory(), OPEN_FLAG_READ);
		osd_file::error fileerr = pSaveState.open(state_fname);
		if (fileerr != osd_file::error::NONE)
		{
			MameMessageBox("Could not open '%s' as a valid savestate file.", filename);
			return;
		}

		// call the MAME core function to check the save state file
		//rc = state_manager::check_file(NULL, pSaveState, selected_filename, MameMessageBox);
		//if (rc)

		memset(&playopts, 0, sizeof(playopts));

		playopts.state = state_fname;
		playopts_apply = 0x57;

		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayRecordGame()
{
	int  nGame = 0;
	char filename[MAX_PATH];
	*filename = 0;

	nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_INPUT_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];
		play_options playopts;

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		memset(&playopts, 0, sizeof(playopts));
		strcat(fname, ".inp");
		playopts.record = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

void MamePlayGame(void)
{
	play_options playopts;

	int nGame = Picker_GetSelectedItem(hwndList);

	memset(&playopts, 0, sizeof(playopts));
	MamePlayGameWithOptions(nGame, &playopts);
}

static void MamePlayRecordWave()
{
	char filename[MAX_PATH];
	play_options playopts;

	int nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_WAVE_FILES))
	{
		memset(&playopts, 0, sizeof(playopts));
		playopts.wavwrite = filename;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayRecordMNG()
{
	char filename[MAX_PATH] = { 0, };

	int nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_MNG_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];
		play_options playopts;

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		memset(&playopts, 0, sizeof(playopts));
		strcat(fname, ".mng");
		playopts.mngwrite = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}

static void MamePlayRecordAVI()
{
	char filename[MAX_PATH] = { 0, };

	int nGame = Picker_GetSelectedItem(hwndList);
	strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_AVI_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];
		play_options playopts;

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		memset(&playopts, 0, sizeof(playopts));
		strcat(fname, ".avi");
		playopts.aviwrite = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


static void MamePlayGameWithOptions(int nGame, const play_options *playopts)
{
	DWORD dwExitCode = 0;
	BOOL res = 0;

	if (!MessApproveImageList(hMain, nGame))
		return;

	if (g_pJoyGUI != NULL)
		KillTimer(hMain, JOYGUI_TIMER);
	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	in_emulation = TRUE;

	dwExitCode = RunMAME(nGame, playopts);
	if (dwExitCode == 0)
	{
		IncrementPlayCount(nGame);
		res = ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
	}
	else
	{
		ShowWindow(hMain, SW_SHOW);
	}

	in_emulation = FALSE;

	// re-sort if sorting on # of times played
	if (GetSortColumn() == COLUMN_PLAYED)
		Picker_Sort(hwndList);

	UpdateStatusBar();

	ShowWindow(hMain, SW_SHOW);
	SetFocus(hwndList);

	if (g_pJoyGUI != NULL)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to seconds
	res++;
}

/* Toggle ScreenShot ON/OFF */
static void ToggleScreenShot(void)
{
	BOOL showScreenShot = GetShowScreenShot();

	SetShowScreenShot((showScreenShot) ? FALSE : TRUE);
	UpdateScreenShot();

	/* Redraw list view */
	if (hBackground != NULL && showScreenShot)
		InvalidateRect(hwndList, NULL, FALSE);
}

static void ToggleSoftware(void)
{
	BOOL showSoftware = GetShowSoftware();

	SetShowSoftware((showSoftware) ? FALSE : TRUE);
	UpdateSoftware();

	/* Redraw list view */
	if (hBackground != NULL && showSoftware)
		InvalidateRect(hwndList, NULL, FALSE);
}

static void AdjustMetrics(void)
{
	HDC hDC;
	TEXTMETRIC tm;
	int xtraX = 0, xtraY = 0;
	AREA area;
	int  offX=0, offY=0;
	int  maxX=0, maxY=0;
	COLORREF textColor;
	TCHAR szClass[128];
	HWND hWnd;
	HRESULT res=0;
	BOOL b_res=0;

	/* WM_SETTINGCHANGE also */
	xtraX  = GetSystemMetrics(SM_CXFIXEDFRAME); /* Dialog frame width */
	xtraY  = GetSystemMetrics(SM_CYFIXEDFRAME); /* Dialog frame height */
	xtraY += GetSystemMetrics(SM_CYMENUSIZE);	/* Menu height */
	xtraY += GetSystemMetrics(SM_CYCAPTION);	/* Caption Height */
	maxX   = GetSystemMetrics(SM_CXSCREEN); 	/* Screen Width */
	maxY   = GetSystemMetrics(SM_CYSCREEN); 	/* Screen Height */

	hDC = GetDC(hMain);
	GetTextMetrics (hDC, &tm);

	/* Convert MIN Width/Height from Dialog Box Units to pixels. */
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight 	  / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	ReleaseDC(hMain, hDC);

	if ((textColor = GetListFontColor()) == RGB(255, 255, 255))
		textColor = RGB(240, 240, 240);

	hWnd = GetWindow(hMain, GW_CHILD);
	while(hWnd)
	{
		if (GetClassName(hWnd, szClass, sizeof(szClass) / sizeof(szClass[0])))
		{
			if (!_tcscmp(szClass, TEXT("SysListView32")))
			{
				b_res = ListView_SetBkColor(hWnd, GetSysColor(COLOR_WINDOW));
				b_res = ListView_SetTextColor(hWnd, textColor);
			}
			else if (!_tcscmp(szClass, TEXT("SysTreeView32")))
			{
				res = TreeView_SetBkColor(hTreeView, GetSysColor(COLOR_WINDOW));
				res = TreeView_SetTextColor(hTreeView, textColor);
			}
		}
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}

	GetWindowArea(&area);

	offX = area.x + area.width;
	offY = area.y + area.height;

	if (offX > maxX)
	{
		offX = maxX;
		area.x = (offX - area.width > 0) ? (offX - area.width) : 0;
	}
	if (offY > maxY)
	{
		offY = maxY;
		area.y = (offY - area.height > 0) ? (offY - area.height) : 0;
	}

	SetWindowArea(&area);
	SetWindowPos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	res++;
	b_res++;
}

int FindIconIndex(int nIconResource)
{
	for(int i = 0; g_iconData[i].icon_name; i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}
	return -1;
}

int FindIconIndexByName(const char *icon_name)
{
	for (int i = 0; g_iconData[i].icon_name; i++)
	{
		if (!strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}
	return -1;
}

static int GetIconForDriver(int nItem)
{
	int iconRoms = 1;

	if (DriverUsesRoms(nItem))
	{
		int audit_result = GetRomAuditResults(nItem);
		if (IsAuditResultKnown(audit_result) == FALSE)
			return 2;
///#ifdef SHOW_MISSING_ROMS_ICON
		if (IsAuditResultYes(audit_result))
			iconRoms = 1;
		else
			iconRoms = 0;
///#endif
	}

	// iconRoms is now either 0 (no roms), 1 (roms), or 2 (unknown)

	/* these are indices into icon_names, which maps into our image list
     * also must match IDI_WIN_NOROMS + iconRoms
     */

	// Show Red-X if the ROMs are present and flagged as NOT WORKING
	if (iconRoms == 1 && DriverIsBroken(nItem))
		iconRoms = FindIconIndex(IDI_WIN_REDX);

	// show clone icon if we have roms and game is working
	if (iconRoms == 1 && DriverIsClone(nItem))
		iconRoms = FindIconIndex(IDI_WIN_CLONE);

	// if we have the roms, then look for a custom per-game icon to override
	if (iconRoms == 1 || iconRoms == 3)
	{
		if (icon_index[nItem] == 0)
			AddDriverIcon(nItem,iconRoms);
		iconRoms = icon_index[nItem];
	}

	return iconRoms;
}

static BOOL HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hTreeMenu;
	HMENU hMenu;
	TVHITTESTINFO hti;
	POINT pt;
	BOOL res = 0;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_TREE))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	/* select the item that was right clicked or shift-F10'ed */
	hti.pt = pt;
	ScreenToClient(hTreeView,&hti.pt);
	(void)TreeView_HitTest(hTreeView,&hti);
	if ((hti.flags & TVHT_ONITEM) != 0)
		res = TreeView_SelectItem(hTreeView,hti.hItem);

	hTreeMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_CONTEXT_TREE));

	InitTreeContextMenu(hTreeMenu);

	hMenu = GetSubMenu(hTreeMenu, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hTreeMenu);
	res++;
	return TRUE;
}



static void GamePicker_OnBodyContextMenu(POINT pt)
{
	HMENU hMenuLoad;
	HMENU hMenu;

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
	hMenu = GetSubMenu(hMenuLoad, 0);
	InitBodyContextMenu(hMenu);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	DestroyMenu(hMenuLoad);
}



static BOOL HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HMENU hMenuLoad;
	HMENU hMenu;
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_SSPICTURE) && (HWND)wParam != GetDlgItem(hWnd, IDC_SSFRAME))
		return FALSE;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_SCREENSHOT));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

static void UpdateMenu(HMENU hMenu)
{
	TCHAR buf[200];
	MENUITEMINFO mItem;
	int nGame = Picker_GetSelectedItem(hwndList);
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int i = 0;
	//const char *pParent;
	TCHAR* t_description;

	if (have_selection)
	{
		t_description = ui_wstring_from_utf8(ConvertAmpersandString(ModifyThe(driver_list::driver(nGame).description)));
		if( !t_description )
			return;

		_sntprintf(buf, ARRAY_LENGTH(buf), g_szPlayGameString, t_description);

		mItem.cbSize	 = sizeof(mItem);
		mItem.fMask 	 = MIIM_TYPE;
		mItem.fType 	 = MFT_STRING;
		mItem.dwTypeData = buf;
		mItem.cch = _tcslen(mItem.dwTypeData);

		SetMenuItemInfo(hMenu, ID_FILE_PLAY, FALSE, &mItem);

		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);

		free(t_description);
	}
	else
	{
		EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
	}

	if (oldControl)
	{
		EnableMenuItem(hMenu, ID_CUSTOMIZE_FIELDS, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES,  MF_GRAYED);
		EnableMenuItem(hMenu, ID_OPTIONS_DEFAULTS, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
		EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_ENABLED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_GRAYED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_GRAYED);
	}
	//pParent = GetFolderNameByID(lpFolder->m_nParent+1);

	if (lpFolder->m_dwFlags & F_INIEDIT)
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_ENABLED);
	else
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_GRAYED);

	CheckMenuRadioItem(hMenu, ID_VIEW_TAB_SCREENSHOT, ID_VIEW_TAB_HISTORY,
		ID_VIEW_TAB_SCREENSHOT + TabView_GetCurrentTab(hTabCtrl), MF_BYCOMMAND);

	// set whether we're showing the tab control or not
	if (bShowTabCtrl)
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_UNCHECKED);


	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		// disable menu items for tabs we're not currently showing
		if (GetShowTab(i))
			EnableMenuItem(hMenu,ID_VIEW_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu,ID_VIEW_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_GRAYED);

		// check toggle menu items
		if (GetShowTab(i))
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_SCREENSHOT + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (GetShowFolder(i))
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

}

void InitTreeContextMenu(HMENU hTreeMenu)
{
	MENUITEMINFO mii;
	HMENU hMenu;
	int i = 0;
	extern const FOLDERDATA g_folderData[];

	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	hMenu = GetSubMenu(hTreeMenu, 0);

	if (GetMenuItemInfo(hMenu,3,TRUE,&mii) == FALSE)
	{
		dprintf("can't find show folders context menu\n");
		return;
	}

	if (mii.hSubMenu == NULL)
	{
		dprintf("can't find submenu for show folders context menu\n");
		return;
	}

	hMenu = mii.hSubMenu;

	for (i=0;g_folderData[i].m_lpTitle != NULL;i++)
	{
		TCHAR* t_title = ui_wstring_from_utf8(g_folderData[i].m_lpTitle);
		if( !t_title )
			return;

		mii.fMask = MIIM_TYPE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.dwTypeData = t_title;
		mii.cch = _tcslen(mii.dwTypeData);
		mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;


		// menu in resources has one empty item (needed for the submenu to setup properly)
		// so overwrite this one, append after
		if (i == 0)
			SetMenuItemInfo(hMenu,ID_CONTEXT_SHOW_FOLDER_START,FALSE,&mii);
		else
			InsertMenuItem(hMenu,i,FALSE,&mii);

		free(t_title);
	}

}

void InitBodyContextMenu(HMENU hBodyContextMenu)
{
	LPTREEFOLDER lpFolder;
	TCHAR tmp[256];
	MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	if (GetMenuItemInfo(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii) == FALSE)
	{
		dprintf("can't find show folders context menu\n");
		return;
	}
	lpFolder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
	_sntprintf(tmp,ARRAY_LENGTH(tmp),TEXT("Properties for %s"),lpFolder->m_lptTitle );
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = tmp;
	mii.cch = _tcslen(mii.dwTypeData);
	mii.wID = ID_FOLDER_SOURCEPROPERTIES;

	// menu in resources has one default item
	// so overwrite this one
	SetMenuItemInfo(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,FALSE,&mii);
	if( ! DriverIsVector(Picker_GetSelectedItem(hwndList) ) )
		EnableMenuItem(hBodyContextMenu, ID_FOLDER_VECTORPROPERTIES, MF_GRAYED);
}


void ToggleShowFolder(int folder)
{
	int current_id = GetCurrentFolderID();

	SetWindowRedraw(hwndList,FALSE);

	SetShowFolder(folder,!GetShowFolder(folder));

	ResetTreeViewFolders();
	SelectTreeViewFolder(current_id);

	SetWindowRedraw(hwndList,TRUE);
}

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hBackground)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
			{
				if (MouseHasBeenMoved())
					ShowCursor(TRUE);
				break;
			}

			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT:
			{
				POINT p = { 0, 0 };

				/* get base point of background bitmap */
				MapWindowPoints(hWnd,hTreeView,&p,1);
				PaintBackgroundImage(hWnd, NULL, p.x, p.y);
				/* to ensure our parent procedure repaints the whole client area */
				InvalidateRect(hWnd, NULL, FALSE);
				break;
			}
		}
	}
	return CallWindowProc(g_lpHistoryWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(TRUE);
			break;
		}

	case WM_NCHITTEST :
		{
			POINT pt;
			RECT  rect;
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);

			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			GetWindowRect(hHistory, &rect);
			// check if they clicked on the picture area (leave 6 pixel no man's land
			// by the history window to reduce mistaken clicks)
			// no more no man's land, the Cursor changes when Edit control is left, should be enough feedback
			if (have_history &&
				( ( (TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
					(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ||
					(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == FALSE) ) &&
//					(rect.top - 6) < pt.y && pt.y < (rect.bottom + 6) ) )
					PtInRect( &rect, pt ) ) )

			{
				return HTTRANSPARENT;
			}
			else
			{
				return HTCLIENT;
			}
		}
		break;
	case WM_CONTEXTMENU:
		if ( HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;
	}

	if (hBackground)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND :
			return TRUE;
		case WM_PAINT :
			{
				RECT rect,nodraw_rect;
				HRGN region,nodraw_region;
				POINT p = { 0, 0 };

				/* get base point of background bitmap */
				MapWindowPoints(hWnd,hTreeView,&p,1);

				/* get big region */
				GetClientRect(hWnd,&rect);
				region = CreateRectRgnIndirect(&rect);

				if (IsWindowVisible(GetDlgItem(hMain,IDC_HISTORY)))
				{
					/* don't draw over this window */
					GetWindowRect(GetDlgItem(hMain,IDC_HISTORY),&nodraw_rect);
					MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
					CombineRgn(region,region,nodraw_region,RGN_DIFF);
					DeleteObject(nodraw_region);
				}
				if (IsWindowVisible(GetDlgItem(hMain,IDC_SSPICTURE)))
				{
					/* don't draw over this window */
					GetWindowRect(GetDlgItem(hMain,IDC_SSPICTURE),&nodraw_rect);
					MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
					CombineRgn(region,region,nodraw_region,RGN_DIFF);
					DeleteObject(nodraw_region);
				}

				PaintBackgroundImage(hWnd,region,p.x,p.y);

				DeleteObject(region);

				/* to ensure our parent procedure repaints the whole client area */
				InvalidateRect(hWnd, NULL, FALSE);

				break;
			}
		}
	}
	return CallWindowProc(g_lpPictureFrameWndProc, hWnd, uMsg, wParam, lParam);
}

static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND :
		return TRUE;
	case WM_PAINT :
		{
			PAINTSTRUCT ps;
			HDC hdc,hdc_temp;
			RECT rect;
			HBITMAP old_bitmap;

			int width,height;

			RECT rect2;
			HBRUSH hBrush;
			HBRUSH holdBrush;
			HRGN region1, region2;
			int nBordersize;
			nBordersize = GetScreenshotBorderSize();
			hBrush = CreateSolidBrush(GetScreenshotBorderColor());

			hdc = BeginPaint(hWnd,&ps);

			hdc_temp = CreateCompatibleDC(hdc);
			if (ScreenShotLoaded())
			{
				width = GetScreenShotWidth();
				height = GetScreenShotHeight();

				old_bitmap = (HBITMAP)SelectObject(hdc_temp,GetScreenShotHandle());
			}
			else
			{
				BITMAP bmp;

				GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);
				width = bmp.bmWidth;
				height = bmp.bmHeight;

				old_bitmap = (HBITMAP)SelectObject(hdc_temp,hMissing_bitmap);
			}

			GetClientRect(hWnd,&rect);

			rect2 = rect;
			//Configurable Borders around images
			rect.bottom -= nBordersize;
			if( rect.bottom < 0)
				rect.bottom = rect2.bottom;
			rect.right -= nBordersize;
			if( rect.right < 0)
				rect.right = rect2.right;
			rect.top += nBordersize;
			if( rect.top > rect.bottom )
				rect.top = rect2.top;
			rect.left += nBordersize;
			if( rect.left > rect.right )
				rect.left = rect2.left;
			region1 = CreateRectRgnIndirect(&rect);
			region2 = CreateRectRgnIndirect(&rect2);
			CombineRgn(region2,region2,region1,RGN_DIFF);
			holdBrush = (HBRUSH)SelectObject(hdc, hBrush);

			FillRgn(hdc,region2, hBrush );
			SelectObject(hdc, holdBrush);
			DeleteBrush(hBrush);

			SetStretchBltMode(hdc,STRETCH_HALFTONE);
			StretchBlt(hdc,nBordersize,nBordersize,rect.right-rect.left,rect.bottom-rect.top,
				hdc_temp,0,0,width,height,SRCCOPY);
			SelectObject(hdc_temp,old_bitmap);
			DeleteDC(hdc_temp);
			DeleteObject(region1);
			DeleteObject(region2);

			EndPaint(hWnd,&ps);

			return TRUE;
		}
	}

	return CallWindowProc(g_lpPictureWndProc, hWnd, uMsg, wParam, lParam);
}

static void RemoveCurrentGameCustomFolder(void)
{
	RemoveGameCustomFolder(Picker_GetSelectedItem(hwndList));
}

static void RemoveGameCustomFolder(int driver_index)
{
	TREEFOLDER **folders;
	int num_folders = 0;

	GetFolders(&folders,&num_folders);

	for (int i=0;i<num_folders;i++)
	{
	    if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nFolderId == GetCurrentFolderID())
		{
			int current_pick_index;

			RemoveFromCustomFolder(folders[i],driver_index);

			if (driver_index == Picker_GetSelectedItem(hwndList))
			{
			/* if we just removed the current game,
                  move the current selection so that when we rebuild the listview it
                  leaves the cursor on next or previous one */

				current_pick_index = GetSelectedPick();
				Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
				if (current_pick_index == GetSelectedPick()) /* we must have deleted the last item */
					Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
			}

			ResetListView();
			return;
		}
	}
	MessageBox(GetMainWindow(), TEXT("Error searching for custom folder"), TEXT(MAMEUINAME), MB_OK | MB_ICONERROR);

}


static void BeginListViewDrag(NM_LISTVIEW *pnmv)
{
	LV_ITEM lvi;
	POINT pt;
	BOOL res = 0;

	lvi.iItem = pnmv->iItem;
	lvi.mask	 = LVIF_PARAM;
	res = ListView_GetItem(hwndList, &lvi);

	game_dragged = lvi.lParam;

	pt.x = 0;
	pt.y = 0;

	/* Tell the list view control to create an image to use for dragging. */
	himl_drag = ListView_CreateDragImage(hwndList,pnmv->iItem,&pt);

	/* Start the drag operation. */
	ImageList_BeginDrag(himl_drag, 0, 0, 0);

	pt = pnmv->ptAction;
	ClientToScreen(hwndList,&pt);
	ImageList_DragEnter(GetDesktopWindow(),pt.x,pt.y);

	/* Hide the mouse cursor, and direct mouse input to the parent window. */
	SetCapture(hMain);

	prev_drag_drop_target = NULL;

	g_listview_dragging = TRUE;
	res++;
}

static void MouseMoveListViewDrag(POINTS p)
{
	HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	BOOL res;

	POINT pt;
	pt.x = p.x;
	pt.y = p.y;

	ClientToScreen(hMain,&pt);

	ImageList_DragMove(pt.x,pt.y);

	MapWindowPoints(GetDesktopWindow(),hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);

	if (htiTarget != prev_drag_drop_target)
	{
		ImageList_DragShowNolock(FALSE);
		if (htiTarget != NULL)
			res = TreeView_SelectDropTarget(hTreeView,htiTarget);
		else
			res = TreeView_SelectDropTarget(hTreeView,NULL);
		ImageList_DragShowNolock(TRUE);

		prev_drag_drop_target = htiTarget;
	}
	res++;
}

static void ButtonUpListViewDrag(POINTS p)
{
	POINT pt;
	HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	TVITEM tvi;
	BOOL res = 0;

	ReleaseCapture();

	ImageList_DragLeave(hwndList);
	ImageList_EndDrag();
	ImageList_Destroy(himl_drag);

	res = TreeView_SelectDropTarget(hTreeView,NULL);

	g_listview_dragging = FALSE;

	/* see where the game was dragged */

	pt.x = p.x;
	pt.y = p.y;

	MapWindowPoints(hMain,hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);
	if (htiTarget == NULL)
	{
		LVHITTESTINFO lvhtti;
		LPTREEFOLDER folder;
		RECT rcList;

		/* the user dragged a game onto something other than the treeview */
		/* try to remove if we're in a custom folder */

		/* see if it was dragged within the list view; if so, ignore */

		MapWindowPoints(hTreeView,hwndList,&pt,1);
		lvhtti.pt = pt;
		GetWindowRect(hwndList, &rcList);
		ClientToScreen(hwndList, &pt);
		if( PtInRect(&rcList, pt) != 0 )
			return;

		folder = GetCurrentFolder();
		if (folder->m_dwFlags & F_CUSTOM)
		{
			/* dragged out of a custom folder, so let's remove it */
			RemoveCurrentGameCustomFolder();
		}
		return;
	}


	tvi.lParam = 0;
	tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = htiTarget;

	if (TreeView_GetItem(hTreeView, &tvi))
	{
		LPTREEFOLDER folder = (LPTREEFOLDER)tvi.lParam;
		AddToCustomFolder(folder,game_dragged);
	}
	res++;
}

static LPTREEFOLDER GetSelectedFolder(void)
{
	HTREEITEM htree;
	TVITEM tvi;
	BOOL res = 0;

	htree = TreeView_GetSelection(hTreeView);
	if(htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		res = TreeView_GetItem(hTreeView,&tvi);
		return (LPTREEFOLDER)tvi.lParam;
	}
	res++;
	return NULL;
}

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
static HICON GetSelectedFolderIcon(void)
{
	HTREEITEM htree;
	TVITEM tvi;
	HIMAGELIST hSmall_icon;
	LPTREEFOLDER folder;
	BOOL res = 0;

	htree = TreeView_GetSelection(hTreeView);
	if (htree != NULL)
	{
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		res = TreeView_GetItem(hTreeView,&tvi);

		folder = (LPTREEFOLDER)tvi.lParam;
		//hSmall_icon = TreeView_GetImageList(hTreeView,(int)tvi.iImage);
		hSmall_icon = NULL;
		return ImageList_GetIcon(hSmall_icon, tvi.iImage, ILD_TRANSPARENT);
	}
	return NULL;
}
#pragma GCC diagnostic error "-Wunused-but-set-variable"

/* Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView(void)
{
	BOOL res = 0;

	if( (GetViewMode() == VIEW_GROUPED) || (GetViewMode() == VIEW_DETAILS ) )
		res = ListView_RedrawItems(hwndList,ListView_GetTopIndex(hwndList),
			ListView_GetTopIndex(hwndList)+ ListView_GetCountPerPage(hwndList) );
	res++;
}

static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height)
{
	int 	destX=0, destY=0;
	int 	destW=0, destH=0;
	int		nBorder=0;
	RECT	rect;
	/* for scaling */
	int x=0, y=0;
	int rWidth=0, rHeight=0;
	double scale=0;
	BOOL bReduce = FALSE;

	GetClientRect(hWnd, &rect);

	// Scale the bitmap to the frame specified by the passed in hwnd
	if (ScreenShotLoaded())
	{
		x = GetScreenShotWidth();
		y = GetScreenShotHeight();
	}
	else
	{
		BITMAP bmp;
		GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

		x = bmp.bmWidth;
		y = bmp.bmHeight;
	}
	rWidth	= (rect.right  - rect.left);
	rHeight = (rect.bottom - rect.top);

	/* Limit the screen shot to max height of 264 */
	if (restrict_height == TRUE && rHeight > 264)
	{
		rect.bottom = rect.top + 264;
		rHeight = 264;
	}

	/* If the bitmap does NOT fit in the screenshot area */
	if (x > rWidth - 10 || y > rHeight - 10)
	{
		rect.right	-= 10;
		rect.bottom -= 10;
		rWidth	-= 10;
		rHeight -= 10;
		bReduce = TRUE;
		/* Try to scale it properly */
		/*  assumes square pixels, doesn't consider aspect ratio */
		if (x > y)
			scale = (double)rWidth / x;
		else
			scale = (double)rHeight / y;

		destW = (int)(x * scale);
		destH = (int)(y * scale);

		/* If it's still too big, scale again */
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth	/ destW;
			else
				scale = (double)rHeight / destH;

			destW = (int)(destW * scale);
			destH = (int)(destH * scale);
		}
	}
	else
	{
		if (GetStretchScreenShotLarger())
		{
			rect.right	-= 10;
			rect.bottom -= 10;
			rWidth	-= 10;
			rHeight -= 10;
			bReduce = TRUE;
			// Try to scale it properly
			// assumes square pixels, doesn't consider aspect ratio
			if (x < y)
				scale = (double)rWidth / x;
			else
				scale = (double)rHeight / y;

			destW = (int)(x * scale);
			destH = (int)(y * scale);

			// If it's too big, scale again
			if (destW > rWidth || destH > rHeight)
			{
				if (destW > rWidth)
					scale = (double)rWidth	/ destW;
				else
					scale = (double)rHeight / destH;

				destW = (int)(destW * scale);
				destH = (int)(destH * scale);
			}
		}
		else
		{
			// Use the bitmaps size if it fits
			destW = x;
			destH = y;
		}

	}


	destX = ((rWidth  - destW) / 2);
	destY = ((rHeight - destH) / 2);

	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}
	nBorder = GetScreenshotBorderSize();
	if( destX > nBorder+1)
		pRect->left = destX - nBorder;
	else
		pRect->left = 2;
	if( destY > nBorder+1)
		pRect->top = destY - nBorder;
	else
		pRect->top = 2;
	if( rWidth >= destX + destW + nBorder)
		pRect->right = destX + destW + nBorder;
	else
		pRect->right = rWidth - pRect->left;
	if( rHeight >= destY + destH + nBorder)
		pRect->bottom = destY + destH + nBorder;
	else
		pRect->bottom = rHeight - pRect->top;
}

/*
  Switches to either fullscreen or normal mode, based on the
  current mode.

  POSSIBLE BUGS:
  Removing the menu might cause problems later if some
  function tries to poll info stored in the menu. Don't
  know if you've done that, but this was the only way I
  knew to remove the menu dynamically.
*/

static void SwitchFullScreenMode(void)
{
	LONG lMainStyle=0;

	if (GetRunFullScreen())
	{
		// Return to normal

		// Restore the menu
		SetMenu(hMain, LoadMenu(hInst,MAKEINTRESOURCE(IDR_UI_MENU)));

		// Refresh the checkmarks
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, GetShowFolderList() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, GetShowToolBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, GetShowStatusBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, GetShowTabCtrl() ? MF_CHECKED : MF_UNCHECKED);

		// Add frame to dialog again
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle | WS_BORDER;
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);

		// Show the window maximized
		if( GetWindowState() == SW_MAXIMIZE )
		{
			ShowWindow(hMain, SW_NORMAL);
			ShowWindow(hMain, SW_MAXIMIZE);
		}
		else
			ShowWindow(hMain, SW_RESTORE);

		SetRunFullScreen(FALSE);
	}
	else
	{
		// Set to fullscreen

		// Remove menu
		SetMenu(hMain,NULL);

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);
		if( IsMaximized(hMain) )
		{
			ShowWindow(hMain, SW_NORMAL);
			SetWindowState( SW_MAXIMIZE );
		}
		ShowWindow(hMain, SW_MAXIMIZE);

		SetRunFullScreen(TRUE);
	}
}

/*
  Checks to see if the mouse has been moved since this func
  was first called (which is at startup). The reason for
  storing the startup coordinates of the mouse is that when
  a window is created it generates WM_MOUSEOVER events, even
  though the user didn't actually move the mouse. So we need
  to know when the WM_MOUSEOVER event is user-triggered.

  POSSIBLE BUGS:
  Gets polled at every WM_MOUSEMOVE so it might cause lag,
  but there's probably another way to code this that's
  way better?

*/

BOOL MouseHasBeenMoved(void)
{
	static int mouse_x = -1;
	static int mouse_y = -1;
	POINT p;

	GetCursorPos(&p);

	if (mouse_x == -1) // First time
	{
		mouse_x = p.x;
		mouse_y = p.y;
	}

	return (p.x != mouse_x || p.y != mouse_y);
}

/* End of source file */
