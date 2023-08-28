//
// Keyboard layouts
//
// grid of 45 x 6 keys, a normal small key is 2 cells wide
// at smallest resolution characters are 5px wide, and the 2-cell key takes 14 pixels:
//   2+5+5+1+1 = left edge + character + character + right edge + space
//

// For each keyboard layout, review:
//   RETROK to SDL mapping in libretro_sdl_keymap.h
//   SDL to ST mapping in keymap.c (Keymap_SymbolicToStScanCode)
//   ST keyboard reference: https://tho-otto.de/keyboards/
// Make sure each RETROK eventually maps to the correct ST scancode according to the reference.
// (This is independent of the user's host keyboard layout.
//  These are not real RETROK keyboard inputs, so it doesn't matter that e.g. RETROK_Z is RETROK_Y
//
// We also have access to special characters in the font set for the displayed key names:
//   hatari\src\gui-sdl\font5x8.bmp
//   hatari\src\gui-sdl\font10x16.bmp
//
// Where there is an alt-key, I've tried to indicate this with a lowercase letter if possible.

//
// Universal rows (F-keys, alt/space/caps/0/./enter)
//

const struct OSKey OSK_ROW0[] = {
	{0,1,0,0},
	{"F1",3,RETROK_F1,0},
	{"F2",3,RETROK_F2,0},
	{"F3",3,RETROK_F3,0},
	{"F4",3,RETROK_F4,0},
	{"F5",3,RETROK_F5,0},
	{"F6",3,RETROK_F6,0},
	{"F7",3,RETROK_F7,0},
	{"F8",3,RETROK_F8,0},
	{"F9",3,RETROK_F9,0},
	{"F10",3,RETROK_F10,0},
	{0,0,0,0}};

const struct OSKey OSK_ROW5[] = {
	{0,2,0,0},
	{"Alt",4,RETROK_LALT,OSK_PRESS_ALT},
	{"",18,RETROK_SPACE,0},
	{"Caps",4,RETROK_CAPSLOCK,0},
	{0,9,0,0},
	{"0",4,RETROK_KP0,0},
	{".",2,RETROK_KP_PERIOD,0},
	{"En",2,RETROK_KP_ENTER,0},
	{0,0,0,0}};

//
// US Layout
//

const struct OSKey OSK_ROW1_US[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2@",2,RETROK_2,0},
	{"3#",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6^",2,RETROK_6,0},
	{"7&",2,RETROK_7,0},
	{"8*",2,RETROK_8,0},
	{"9(",2,RETROK_9,0},
	{"0)",2,RETROK_0,0},
	{"-_",2,RETROK_MINUS,0},
	{"=+",2,RETROK_EQUALS,0},
	{"`~",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0}, // (Help)
	{"Und",3,RETROK_END,0}, // (Undo)
	{"(",2,RETROK_PAGEUP,0}, // (Left Paren)
	{")",2,RETROK_PAGEDOWN,0}, // (Right Paren)
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_US[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"[{",2,RETROK_LEFTBRACKET,0},
	{"]}",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_US[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{";:",2,RETROK_SEMICOLON,0},
	{"'\"",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\|",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_US[] = {
	{"Shift",5,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"Z",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",<",2,RETROK_COMMA,0},
	{".>",2,RETROK_PERIOD,0},
	{"/?",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_US[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_US,
	OSK_ROW2_US,
	OSK_ROW3_US,
	OSK_ROW4_US,
	OSK_ROW5,
};

//
// German Layout
//

const struct OSKey OSK_ROW1_DE[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3\xA7",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6&",2,RETROK_6,0},
	{"7/",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0=",2,RETROK_0,0},
	{"\xDF?",2,RETROK_MINUS,0},
	{"'`",2,RETROK_EQUALS,0},
	{"#^",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_DE[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Z",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xDC\\",2,RETROK_LEFTBRACKET,0},
	{"+*",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_DE[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xD6[",2,RETROK_SEMICOLON,0},
	{"\xC4]",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"~|",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_DE[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"<>",2,RETROK_LESS,0},
	{"Y",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",;",2,RETROK_COMMA,0},
	{".:",2,RETROK_PERIOD,0},
	{"-_",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_DE[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_DE,
	OSK_ROW2_DE,
	OSK_ROW3_DE,
	OSK_ROW4_DE,
	OSK_ROW5,
};

//
// French Layout
//

const struct OSKey OSK_ROW1_FR[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"&1",2,RETROK_1,0},
	{"\xE9""2",2,RETROK_2,0},
	{"\"3",2,RETROK_3,0},
	{"'4",2,RETROK_4,0},
	{"(5",2,RETROK_5,0},
	{"\xA7""6",2,RETROK_6,0},
	{"\xE8""7",2,RETROK_7,0},
	{"!8",2,RETROK_8,0},
	{"\xE7""9",2,RETROK_9,0},
	{"\xE0""0",2,RETROK_0,0},
	{")\xB0",2,RETROK_MINUS,0},
	{"-_",2,RETROK_EQUALS,0},
	{"`\xA3",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_FR[] = {
	{"Tab",3,RETROK_TAB,0},
	{"A",2,RETROK_q,0},
	{"Z",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"^[",2,RETROK_LEFTBRACKET,0},
	{"$]",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_FR[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"Q",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"M",2,RETROK_SEMICOLON,0},
	{"\xF9\\",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"#@",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_FR[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"<>",2,RETROK_LESS,0},
	{"W",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{",?",2,RETROK_m,0},
	{";.",2,RETROK_COMMA,0},
	{":/",2,RETROK_PERIOD,0},
	{"=+",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_FR[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_FR,
	OSK_ROW2_FR,
	OSK_ROW3_FR,
	OSK_ROW4_FR,
	OSK_ROW5,
};

//
// UK Layout
//

const struct OSKey OSK_ROW1_UK[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3\xA3",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6^",2,RETROK_6,0},
	{"7&",2,RETROK_7,0},
	{"8*",2,RETROK_8,0},
	{"9(",2,RETROK_9,0},
	{"0)",2,RETROK_0,0},
	{"-_",2,RETROK_MINUS,0},
	{"=+",2,RETROK_EQUALS,0},
	{"`\xAF",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_UK[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{";:",2,RETROK_SEMICOLON,0},
	{"'@",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"#~",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_UK[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"\\|",2,RETROK_LESS,0},
	{"Z",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",<",2,RETROK_COMMA,0},
	{".>",2,RETROK_PERIOD,0},
	{"/?",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_UK[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_UK,
	OSK_ROW2_US, // same as US
	OSK_ROW3_UK,
	OSK_ROW4_UK,
	OSK_ROW5,
};

//
// Spanish Layout
//

const struct OSKey OSK_ROW1_ES[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2\xBF",2,RETROK_2,0},
	{"3\xA3",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6/",2,RETROK_6,0},
	{"7&",2,RETROK_7,0},
	{"8*",2,RETROK_8,0},
	{"9(",2,RETROK_9,0},
	{"0)",2,RETROK_0,0},
	{"-_",2,RETROK_MINUS,0},
	{"=+",2,RETROK_EQUALS,0},
	{"\xE7~",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_ES[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"'[",2,RETROK_LEFTBRACKET,0},
	{"`]",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_ES[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xD1",2,RETROK_SEMICOLON,0},
	{";\xFC",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\#",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_ES[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"<>",2,RETROK_LESS,0},
	{"Z",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",?",2,RETROK_COMMA,0},
	{".!",2,RETROK_PERIOD,0},
	{"\xB0\xA7",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_ES[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_ES,
	OSK_ROW2_ES,
	OSK_ROW3_ES,
	OSK_ROW4_ES,
	OSK_ROW5,
};

//
// Italian Layout
//

const struct OSKey OSK_ROW1_IT[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3\xA3",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6&",2,RETROK_6,0},
	{"7/",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0=",2,RETROK_0,0},
	{"'?",2,RETROK_MINUS,0},
	{"\xEC^",2,RETROK_EQUALS,0},
	{"\xFC\xA7",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_IT[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xE8[",2,RETROK_LEFTBRACKET,0},
	{"+]",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_IT[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xF2@",2,RETROK_SEMICOLON,0},
	{"\xE0#",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\\xB0",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_IT[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"<>",2,RETROK_LESS,0},
	{"Z",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",;",2,RETROK_COMMA,0},
	{".:",2,RETROK_PERIOD,0},
	{"-_",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_IT[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_IT,
	OSK_ROW2_IT,
	OSK_ROW3_IT,
	OSK_ROW4_IT,
	OSK_ROW5,
};

//
// Swedish Layout
//

const struct OSKey OSK_ROW1_SE[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3#",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6&",2,RETROK_6,0},
	{"7/",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0=",2,RETROK_0,0},
	{"+?",2,RETROK_MINUS,0},
	{"\xC9",2,RETROK_EQUALS,0},
	{"'*",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_SE[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xE5[",2,RETROK_LEFTBRACKET,0},
	{"\xFC]",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_SE[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xD6",2,RETROK_SEMICOLON,0},
	{"\xE4`",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\^",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_SE[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"<>",2,RETROK_LESS,0},
	{"Z",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",;",2,RETROK_COMMA,0},
	{".:",2,RETROK_PERIOD,0},
	{"-_",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_SE[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_SE,
	OSK_ROW2_SE,
	OSK_ROW3_SE,
	OSK_ROW4_SE,
	OSK_ROW5,
};

//
// Swiss French Layout
//

const struct OSKey OSK_ROW1_CH_FR[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1+",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3*",2,RETROK_3,0},
	{"4\xE7",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6&",2,RETROK_6,0},
	{"7/",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0=",2,RETROK_0,0},
	{"'?",2,RETROK_MINUS,0},
	{"^`",2,RETROK_EQUALS,0},
	{"\xA7\xB0",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_CH_FR[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Z",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xE8\\",2,RETROK_LEFTBRACKET,0},
	{"\xA8#",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_CH_FR[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xE9[",2,RETROK_SEMICOLON,0},
	{"\xE0]",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"$~",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_CH_FR[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_CH_FR,
	OSK_ROW2_CH_FR,
	OSK_ROW3_CH_FR,
	OSK_ROW4_DE, // same as german
	OSK_ROW5,
};

//
// Swiss German Layout
//

const struct OSKey OSK_ROW1_CH_DE[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1+",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3*",2,RETROK_3,0},
	{"4\xE7",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6&",2,RETROK_6,0},
	{"7/",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0=",2,RETROK_0,0},
	{"'?",2,RETROK_MINUS,0},
	{"^`",2,RETROK_EQUALS,0},
	{"\xA7\xB0",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_CH_DE[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Z",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xFC\\",2,RETROK_LEFTBRACKET,0},
	{"\xA8#",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_CH_DE[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xF6[",2,RETROK_SEMICOLON,0},
	{"\xE4]",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"$~",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_CH_DE[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_CH_DE,
	OSK_ROW2_CH_DE,
	OSK_ROW3_CH_DE,
	OSK_ROW4_DE, // same as german
	OSK_ROW5,
};

//
// Finnish Layout
//

static const struct OSKey* const OSK_ROWS_FI[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_SE, // same as Swedish
	OSK_ROW2_SE,
	OSK_ROW3_SE,
	OSK_ROW4_SE,
	OSK_ROW5,
};

//
// Norwegian Layout
//

const struct OSKey OSK_ROW3_NO[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xD8",2,RETROK_SEMICOLON,0},
	{"\xE6`",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\^",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_NO[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_SE, // same as Swedish
	OSK_ROW2_SE,
	OSK_ROW3_NO,
	OSK_ROW4_SE,
	OSK_ROW5,
};

//
// Danish Layout
//

const struct OSKey OSK_ROW1_DK[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1!",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3#",2,RETROK_3,0},
	{"4$",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6&",2,RETROK_6,0},
	{"7/",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0=",2,RETROK_0,0},
	{"+?",2,RETROK_MINUS,0},
	{"\x92\x91",2,RETROK_EQUALS,0},
	{"\xC9",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_DK[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xE5[",2,RETROK_LEFTBRACKET,0},
	{"*]",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_DK[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"\xC6",2,RETROK_SEMICOLON,0},
	{"\xF8`",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\^",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_DK[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_DK,
	OSK_ROW2_DK,
	OSK_ROW3_DK,
	OSK_ROW4_SE, // same as Swedish
	OSK_ROW5,
};

//
// Dutch Layout
//

static const struct OSKey* const OSK_ROWS_NL[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_UK, // same as UK
	OSK_ROW2_US, // (UK row 2 same as US)
	OSK_ROW3_UK,
	OSK_ROW4_UK,
	OSK_ROW5,
};

//
// Czech Layout
//

const struct OSKey OSK_ROW1_CZ[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"\xD3",2,RETROK_1,0},
	{"E",2,RETROK_2,0}, // don't have Ě
	{"\x8A",2,RETROK_3,0},
	{"C",2,RETROK_4,0}, // don't have Č
	{"R",2,RETROK_5,0}, // don't have Ř
	{"\x8E",2,RETROK_6,0},
	{"\xDD",2,RETROK_7,0},
	{"\xC1",2,RETROK_8,0},
	{"\xCD",2,RETROK_9,0},
	{"\xC9",2,RETROK_0,0},
	{"=?",2,RETROK_MINUS,0},
	{"'`",2,RETROK_EQUALS,0},
	{"#^",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_CZ[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Z",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xDA",2,RETROK_LEFTBRACKET,0},
	{"D",2,RETROK_RIGHTBRACKET,0}, // don't have Ď
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7|",2,RETROK_KP7,0},
	{"8[",2,RETROK_KP8,0},
	{"9]",2,RETROK_KP9,0},
	{"-_",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_CZ[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"A",2,RETROK_a,0},
	{"S",2,RETROK_s,0},
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"L",2,RETROK_l,0},
	{"U",2,RETROK_SEMICOLON,0}, // don't have Ů
	{"T",2,RETROK_QUOTE,0}, // don't have Ť
	{"Ret",3,RETROK_RETURN,0},
	{"\xD1",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4$",2,RETROK_KP4,0},
	{"5%",2,RETROK_KP5,0},
	{"6&",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_CZ[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"<>",2,RETROK_LESS,0},
	{"Y",2,RETROK_z,0},
	{"X",2,RETROK_x,0},
	{"C",2,RETROK_c,0},
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"N",2,RETROK_n,0},
	{"M",2,RETROK_m,0},
	{",;",2,RETROK_COMMA,0},
	{".:",2,RETROK_PERIOD,0},
	{"-_",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1!",2,RETROK_KP1,0},
	{"2\"",2,RETROK_KP2,0},
	{"3#",2,RETROK_KP3,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW5_CZ[] = {
	{0,2,0,0},
	{"Alt",4,RETROK_LALT,OSK_PRESS_ALT},
	{"",18,RETROK_SPACE,0},
	{"Caps",4,RETROK_CAPSLOCK,0},
	{0,9,0,0},
	{"0=",4,RETROK_KP0,0}, // the = on the numpad 0 is special for Czech
	{".",2,RETROK_KP_PERIOD,0},
	{"En",2,RETROK_KP_ENTER,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_CZ[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_CZ,
	OSK_ROW2_CZ,
	OSK_ROW3_CZ,
	OSK_ROW4_CZ,
	OSK_ROW5_CZ, // only layout with special row 5
};

//
// Hungarian Layout
//

const struct OSKey OSK_ROW1_HU[] = {
	{"Es",2,RETROK_ESCAPE,0},
	{"1~",2,RETROK_1,0},
	{"2\"",2,RETROK_2,0},
	{"3+",2,RETROK_3,0},
	{"4!",2,RETROK_4,0},
	{"5%",2,RETROK_5,0},
	{"6/",2,RETROK_6,0},
	{"7=",2,RETROK_7,0},
	{"8(",2,RETROK_8,0},
	{"9)",2,RETROK_9,0},
	{"0\xA7",2,RETROK_0,0},
	{"\xF6\x94",2,RETROK_MINUS,0},
	{"\xFC\xA8",2,RETROK_EQUALS,0},
	{"\xF3\xB8",2,RETROK_BACKQUOTE,0},
	{"Bck",3,RETROK_BACKSPACE,0},
	{"Hlp",3,RETROK_PRINT,0},
	{"Und",3,RETROK_END,0},
	{"(",2,RETROK_PAGEUP,0},
	{")",2,RETROK_PAGEDOWN,0},
	{"/",2,RETROK_KP_DIVIDE,0},
	{"*",2,RETROK_KP_MULTIPLY,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW2_HU[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"E",2,RETROK_e,0},
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Z",2,RETROK_y,0},
	{"u\x80",2,RETROK_u,0},
	{"i\xCD",2,RETROK_i,0},
	{"O",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"\xF6\xF7",2,RETROK_LEFTBRACKET,0},
	{"\xFA\xD7",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_HU[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"a\xE4",2,RETROK_a,0},
	{"s\xF0",2,RETROK_s,0},
	{"d\xD0",2,RETROK_d,0},
	{"f[",2,RETROK_f,0},
	{"g]",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"j\xED",2,RETROK_j,0},
	{"kl",2,RETROK_k,0}, // don't have ł
	{"lL",2,RETROK_l,0}, // don't have Ł
	{"\xE9$",2,RETROK_SEMICOLON,0},
	{"\xE1\xDF",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\xFD\xA4",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_HU[] = {
	{"Shf",3,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"\xED<",2,RETROK_LESS,0},
	{"y>",2,RETROK_z,0},
	{"x#",2,RETROK_x,0},
	{"c&",2,RETROK_c,0},
	{"v@",2,RETROK_v,0},
	{"b{",2,RETROK_b,0},
	{"n}",2,RETROK_n,0},
	{"m<",2,RETROK_m,0},
	{",;",2,RETROK_COMMA,0},
	{".>",2,RETROK_PERIOD,0},
	{"-*",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_HU[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_HU,
	OSK_ROW2_HU,
	OSK_ROW3_HU,
	OSK_ROW4_HU,
	OSK_ROW5,
};

//
// Polish Layout
//

const struct OSKey OSK_ROW2_PL[] = {
	{"Tab",3,RETROK_TAB,0},
	{"Q",2,RETROK_q,0},
	{"W",2,RETROK_w,0},
	{"ee",2,RETROK_e,0}, // don't have ę
	{"R",2,RETROK_r,0},
	{"T",2,RETROK_t,0},
	{"Y",2,RETROK_y,0},
	{"U",2,RETROK_u,0},
	{"I",2,RETROK_i,0},
	{"o\xF3",2,RETROK_o,0},
	{"P",2,RETROK_p,0},
	{"[{",2,RETROK_LEFTBRACKET,0},
	{"]}",2,RETROK_RIGHTBRACKET,0},
	{0,2,0,0},
	{"Dl",2,RETROK_DELETE,0},
	{"In",2,RETROK_INSERT,0},
	{"\x1",2,RETROK_UP,0},
	{"Hm",2,RETROK_HOME,0},
	{"7",2,RETROK_KP7,0},
	{"8",2,RETROK_KP8,0},
	{"9",2,RETROK_KP9,0},
	{"-",2,RETROK_KP_MINUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW3_PL[] = {
	{"Ctrl",4,RETROK_LCTRL,OSK_PRESS_CTRL},
	{"aa",2,RETROK_a,0}, // don't have ą
	{"ss",2,RETROK_s,0}, // don't have ś
	{"D",2,RETROK_d,0},
	{"F",2,RETROK_f,0},
	{"G",2,RETROK_g,0},
	{"H",2,RETROK_h,0},
	{"J",2,RETROK_j,0},
	{"K",2,RETROK_k,0},
	{"ll",2,RETROK_l,0}, // don't have ł
	{";:",2,RETROK_SEMICOLON,0},
	{"'\"",2,RETROK_QUOTE,0},
	{"Ret",3,RETROK_RETURN,0},
	{"\\|",2,RETROK_BACKSLASH,0},
	{"\x4",2,RETROK_LEFT,0},
	{"\x2",2,RETROK_DOWN,0},
	{"\x3",2,RETROK_RIGHT,0},
	{"4",2,RETROK_KP4,0},
	{"5",2,RETROK_KP5,0},
	{"6",2,RETROK_KP6,0},
	{"+",2,RETROK_KP_PLUS,0},
	{0,0,0,0}};
const struct OSKey OSK_ROW4_PL[] = {
	{"Shift",5,RETROK_LSHIFT,OSK_PRESS_SHL},
	{"zz",2,RETROK_z,0}, // don't have ż
	{"Xz",2,RETROK_x,0}, // don't have ź
	{"cc",2,RETROK_c,0}, // don't have ć
	{"V",2,RETROK_v,0},
	{"B",2,RETROK_b,0},
	{"nn",2,RETROK_n,0}, // don't have ń
	{"M",2,RETROK_m,0},
	{",<",2,RETROK_COMMA,0},
	{".>",2,RETROK_PERIOD,0},
	{"/?",2,RETROK_SLASH,0},
	{"Shift",5,RETROK_RSHIFT,OSK_PRESS_SHR},
	{0,7,0,0},
	{"1",2,RETROK_KP1,0},
	{"2",2,RETROK_KP2,0},
	{"3",2,RETROK_KP3,0},
	{0,0,0,0}};

static const struct OSKey* const OSK_ROWS_PL[OSK_ROWS] = {
	OSK_ROW0,
	OSK_ROW1_US, // same as US
	OSK_ROW2_PL,
	OSK_ROW3_PL,
	OSK_ROW4_PL,
	OSK_ROW5,
};

//
// Layout collection
//

static const struct OSKey* const * const OSK_LAYOUTS[] = {
	OSK_ROWS_US,
	OSK_ROWS_DE,
	OSK_ROWS_FR,
	OSK_ROWS_UK,
	OSK_ROWS_ES,
	OSK_ROWS_IT,
	OSK_ROWS_SE,
	OSK_ROWS_CH_FR,
	OSK_ROWS_CH_DE,
	OSK_ROWS_FI,
	OSK_ROWS_NO,
	OSK_ROWS_DK,
	OSK_ROWS_NL,
	OSK_ROWS_CZ,
	OSK_ROWS_HU,
	OSK_ROWS_PL,
};
