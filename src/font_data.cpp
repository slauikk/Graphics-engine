#include "font_data.h"
#include <map>

std::map<char, unsigned char[12]> fontData;

void initFontData() {
    // 0-9
    fontData['0'][0] = 0x3E; fontData['0'][1] = 0x63; fontData['0'][2] = 0x73; fontData['0'][3] = 0x7B;
    fontData['0'][4] = 0x6F; fontData['0'][5] = 0x67; fontData['0'][6] = 0x63; fontData['0'][7] = 0x63;
    fontData['0'][8] = 0x63; fontData['0'][9] = 0x63; fontData['0'][10] = 0x63; fontData['0'][11] = 0x3E;
    
    fontData['1'][0] = 0x18; fontData['1'][1] = 0x1C; fontData['1'][2] = 0x18; fontData['1'][3] = 0x18;
    fontData['1'][4] = 0x18; fontData['1'][5] = 0x18; fontData['1'][6] = 0x18; fontData['1'][7] = 0x18;
    fontData['1'][8] = 0x18; fontData['1'][9] = 0x18; fontData['1'][10] = 0x18; fontData['1'][11] = 0x7E;
    
    fontData['2'][0] = 0x3E; fontData['2'][1] = 0x63; fontData['2'][2] = 0x60; fontData['2'][3] = 0x60;
    fontData['2'][4] = 0x30; fontData['2'][5] = 0x18; fontData['2'][6] = 0x0C; fontData['2'][7] = 0x06;
    fontData['2'][8] = 0x03; fontData['2'][9] = 0x03; fontData['2'][10] = 0x63; fontData['2'][11] = 0x7F;
    
    fontData['3'][0] = 0x3E; fontData['3'][1] = 0x63; fontData['3'][2] = 0x60; fontData['3'][3] = 0x60;
    fontData['3'][4] = 0x3C; fontData['3'][5] = 0x60; fontData['3'][6] = 0x60; fontData['3'][7] = 0x60;
    fontData['3'][8] = 0x60; fontData['3'][9] = 0x60; fontData['3'][10] = 0x63; fontData['3'][11] = 0x3E;
    
    fontData['4'][0] = 0x30; fontData['4'][1] = 0x38; fontData['4'][2] = 0x3C; fontData['4'][3] = 0x36;
    fontData['4'][4] = 0x33; fontData['4'][5] = 0x31; fontData['4'][6] = 0x7F; fontData['4'][7] = 0x30;
    fontData['4'][8] = 0x30; fontData['4'][9] = 0x30; fontData['4'][10] = 0x30; fontData['4'][11] = 0x30;
    
    fontData['5'][0] = 0x7F; fontData['5'][1] = 0x03; fontData['5'][2] = 0x03; fontData['5'][3] = 0x03;
    fontData['5'][4] = 0x3F; fontData['5'][5] = 0x60; fontData['5'][6] = 0x60; fontData['5'][7] = 0x60;
    fontData['5'][8] = 0x60; fontData['5'][9] = 0x60; fontData['5'][10] = 0x63; fontData['5'][11] = 0x3E;
    
    fontData['6'][0] = 0x3E; fontData['6'][1] = 0x63; fontData['6'][2] = 0x03; fontData['6'][3] = 0x03;
    fontData['6'][4] = 0x3F; fontData['6'][5] = 0x63; fontData['6'][6] = 0x63; fontData['6'][7] = 0x63;
    fontData['6'][8] = 0x63; fontData['6'][9] = 0x63; fontData['6'][10] = 0x63; fontData['6'][11] = 0x3E;
    
    fontData['7'][0] = 0x7F; fontData['7'][1] = 0x63; fontData['7'][2] = 0x60; fontData['7'][3] = 0x60;
    fontData['7'][4] = 0x30; fontData['7'][5] = 0x30; fontData['7'][6] = 0x18; fontData['7'][7] = 0x18;
    fontData['7'][8] = 0x0C; fontData['7'][9] = 0x0C; fontData['7'][10] = 0x0C; fontData['7'][11] = 0x0C;
    
    fontData['8'][0] = 0x3E; fontData['8'][1] = 0x63; fontData['8'][2] = 0x63; fontData['8'][3] = 0x63;
    fontData['8'][4] = 0x3E; fontData['8'][5] = 0x63; fontData['8'][6] = 0x63; fontData['8'][7] = 0x63;
    fontData['8'][8] = 0x63; fontData['8'][9] = 0x63; fontData['8'][10] = 0x63; fontData['8'][11] = 0x3E;
    
    fontData['9'][0] = 0x3E; fontData['9'][1] = 0x63; fontData['9'][2] = 0x63; fontData['9'][3] = 0x63;
    fontData['9'][4] = 0x63; fontData['9'][5] = 0x7E; fontData['9'][6] = 0x60; fontData['9'][7] = 0x60;
    fontData['9'][8] = 0x60; fontData['9'][9] = 0x60; fontData['9'][10] = 0x63; fontData['9'][11] = 0x3E;
    
    // A-Z
    fontData['A'][0] = 0x1C; fontData['A'][1] = 0x36; fontData['A'][2] = 0x63; fontData['A'][3] = 0x63;
    fontData['A'][4] = 0x63; fontData['A'][5] = 0x7F; fontData['A'][6] = 0x63; fontData['A'][7] = 0x63;
    fontData['A'][8] = 0x63; fontData['A'][9] = 0x63; fontData['A'][10] = 0x63; fontData['A'][11] = 0x63;
    
    fontData['B'][0] = 0x3F; fontData['B'][1] = 0x66; fontData['B'][2] = 0x66; fontData['B'][3] = 0x66;
    fontData['B'][4] = 0x3E; fontData['B'][5] = 0x66; fontData['B'][6] = 0x66; fontData['B'][7] = 0x66;
    fontData['B'][8] = 0x66; fontData['B'][9] = 0x66; fontData['B'][10] = 0x66; fontData['B'][11] = 0x3F;
    
    fontData['C'][0] = 0x3C; fontData['C'][1] = 0x66; fontData['C'][2] = 0x03; fontData['C'][3] = 0x03;
    fontData['C'][4] = 0x03; fontData['C'][5] = 0x03; fontData['C'][6] = 0x03; fontData['C'][7] = 0x03;
    fontData['C'][8] = 0x03; fontData['C'][9] = 0x03; fontData['C'][10] = 0x66; fontData['C'][11] = 0x3C;
    
    fontData['D'][0] = 0x1F; fontData['D'][1] = 0x33; fontData['D'][2] = 0x63; fontData['D'][3] = 0x63;
    fontData['D'][4] = 0x63; fontData['D'][5] = 0x63; fontData['D'][6] = 0x63; fontData['D'][7] = 0x63;
    fontData['D'][8] = 0x63; fontData['D'][9] = 0x63; fontData['D'][10] = 0x33; fontData['D'][11] = 0x1F;
    
    fontData['E'][0] = 0x7F; fontData['E'][1] = 0x03; fontData['E'][2] = 0x03; fontData['E'][3] = 0x03;
    fontData['E'][4] = 0x3F; fontData['E'][5] = 0x03; fontData['E'][6] = 0x03; fontData['E'][7] = 0x03;
    fontData['E'][8] = 0x03; fontData['E'][9] = 0x03; fontData['E'][10] = 0x03; fontData['E'][11] = 0x7F;
    
    fontData['F'][0] = 0x7F; fontData['F'][1] = 0x03; fontData['F'][2] = 0x03; fontData['F'][3] = 0x03;
    fontData['F'][4] = 0x3F; fontData['F'][5] = 0x03; fontData['F'][6] = 0x03; fontData['F'][7] = 0x03;
    fontData['F'][8] = 0x03; fontData['F'][9] = 0x03; fontData['F'][10] = 0x03; fontData['F'][11] = 0x03;
    
    fontData['G'][0] = 0x3C; fontData['G'][1] = 0x66; fontData['G'][2] = 0x03; fontData['G'][3] = 0x03;
    fontData['G'][4] = 0x03; fontData['G'][5] = 0x73; fontData['G'][6] = 0x63; fontData['G'][7] = 0x63;
    fontData['G'][8] = 0x63; fontData['G'][9] = 0x63; fontData['G'][10] = 0x66; fontData['G'][11] = 0x3C;
    
    fontData['H'][0] = 0x63; fontData['H'][1] = 0x63; fontData['H'][2] = 0x63; fontData['H'][3] = 0x63;
    fontData['H'][4] = 0x63; fontData['H'][5] = 0x7F; fontData['H'][6] = 0x63; fontData['H'][7] = 0x63;
    fontData['H'][8] = 0x63; fontData['H'][9] = 0x63; fontData['H'][10] = 0x63; fontData['H'][11] = 0x63;
    
    fontData['I'][0] = 0x3C; fontData['I'][1] = 0x18; fontData['I'][2] = 0x18; fontData['I'][3] = 0x18;
    fontData['I'][4] = 0x18; fontData['I'][5] = 0x18; fontData['I'][6] = 0x18; fontData['I'][7] = 0x18;
    fontData['I'][8] = 0x18; fontData['I'][9] = 0x18; fontData['I'][10] = 0x18; fontData['I'][11] = 0x3C;
    
    fontData['J'][0] = 0x78; fontData['J'][1] = 0x30; fontData['J'][2] = 0x30; fontData['J'][3] = 0x30;
    fontData['J'][4] = 0x30; fontData['J'][5] = 0x30; fontData['J'][6] = 0x30; fontData['J'][7] = 0x33;
    fontData['J'][8] = 0x33; fontData['J'][9] = 0x33; fontData['J'][10] = 0x33; fontData['J'][11] = 0x1E;
    
    fontData['K'][0] = 0x63; fontData['K'][1] = 0x33; fontData['K'][2] = 0x1B; fontData['K'][3] = 0x0F;
    fontData['K'][4] = 0x07; fontData['K'][5] = 0x0F; fontData['K'][6] = 0x1B; fontData['K'][7] = 0x33;
    fontData['K'][8] = 0x63; fontData['K'][9] = 0x63; fontData['K'][10] = 0x63; fontData['K'][11] = 0x63;
    
    fontData['L'][0] = 0x03; fontData['L'][1] = 0x03; fontData['L'][2] = 0x03; fontData['L'][3] = 0x03;
    fontData['L'][4] = 0x03; fontData['L'][5] = 0x03; fontData['L'][6] = 0x03; fontData['L'][7] = 0x03;
    fontData['L'][8] = 0x03; fontData['L'][9] = 0x03; fontData['L'][10] = 0x03; fontData['L'][11] = 0x7F;
    
    fontData['M'][0] = 0x63; fontData['M'][1] = 0x77; fontData['M'][2] = 0x7F; fontData['M'][3] = 0x6B;
    fontData['M'][4] = 0x63; fontData['M'][5] = 0x63; fontData['M'][6] = 0x63; fontData['M'][7] = 0x63;
    fontData['M'][8] = 0x63; fontData['M'][9] = 0x63; fontData['M'][10] = 0x63; fontData['M'][11] = 0x63;
    
    fontData['N'][0] = 0x63; fontData['N'][1] = 0x67; fontData['N'][2] = 0x6F; fontData['N'][3] = 0x7B;
    fontData['N'][4] = 0x73; fontData['N'][5] = 0x63; fontData['N'][6] = 0x63; fontData['N'][7] = 0x63;
    fontData['N'][8] = 0x63; fontData['N'][9] = 0x63; fontData['N'][10] = 0x63; fontData['N'][11] = 0x63;
    
    fontData['O'][0] = 0x3E; fontData['O'][1] = 0x63; fontData['O'][2] = 0x63; fontData['O'][3] = 0x63;
    fontData['O'][4] = 0x63; fontData['O'][5] = 0x63; fontData['O'][6] = 0x63; fontData['O'][7] = 0x63;
    fontData['O'][8] = 0x63; fontData['O'][9] = 0x63; fontData['O'][10] = 0x63; fontData['O'][11] = 0x3E;
    
    fontData['P'][0] = 0x3F; fontData['P'][1] = 0x63; fontData['P'][2] = 0x63; fontData['P'][3] = 0x63;
    fontData['P'][4] = 0x63; fontData['P'][5] = 0x3F; fontData['P'][6] = 0x03; fontData['P'][7] = 0x03;
    fontData['P'][8] = 0x03; fontData['P'][9] = 0x03; fontData['P'][10] = 0x03; fontData['P'][11] = 0x03;
    
    fontData['Q'][0] = 0x3E; fontData['Q'][1] = 0x63; fontData['Q'][2] = 0x63; fontData['Q'][3] = 0x63;
    fontData['Q'][4] = 0x63; fontData['Q'][5] = 0x63; fontData['Q'][6] = 0x6B; fontData['Q'][7] = 0x73;
    fontData['Q'][8] = 0x67; fontData['Q'][9] = 0x63; fontData['Q'][10] = 0x66; fontData['Q'][11] = 0x3D;
    
    fontData['R'][0] = 0x3F; fontData['R'][1] = 0x63; fontData['R'][2] = 0x63; fontData['R'][3] = 0x63;
    fontData['R'][4] = 0x63; fontData['R'][5] = 0x3F; fontData['R'][6] = 0x1B; fontData['R'][7] = 0x33;
    fontData['R'][8] = 0x63; fontData['R'][9] = 0x63; fontData['R'][10] = 0x63; fontData['R'][11] = 0x63;
    
    fontData['S'][0] = 0x3E; fontData['S'][1] = 0x63; fontData['S'][2] = 0x03; fontData['S'][3] = 0x03;
    fontData['S'][4] = 0x0E; fontData['S'][5] = 0x38; fontData['S'][6] = 0x60; fontData['S'][7] = 0x60;
    fontData['S'][8] = 0x60; fontData['S'][9] = 0x60; fontData['S'][10] = 0x63; fontData['S'][11] = 0x3E;
    
    fontData['T'][0] = 0x7E; fontData['T'][1] = 0x18; fontData['T'][2] = 0x18; fontData['T'][3] = 0x18;
    fontData['T'][4] = 0x18; fontData['T'][5] = 0x18; fontData['T'][6] = 0x18; fontData['T'][7] = 0x18;
    fontData['T'][8] = 0x18; fontData['T'][9] = 0x18; fontData['T'][10] = 0x18; fontData['T'][11] = 0x18;
    
    fontData['U'][0] = 0x63; fontData['U'][1] = 0x63; fontData['U'][2] = 0x63; fontData['U'][3] = 0x63;
    fontData['U'][4] = 0x63; fontData['U'][5] = 0x63; fontData['U'][6] = 0x63; fontData['U'][7] = 0x63;
    fontData['U'][8] = 0x63; fontData['U'][9] = 0x63; fontData['U'][10] = 0x63; fontData['U'][11] = 0x3E;
    
    fontData['V'][0] = 0x63; fontData['V'][1] = 0x63; fontData['V'][2] = 0x63; fontData['V'][3] = 0x63;
    fontData['V'][4] = 0x63; fontData['V'][5] = 0x63; fontData['V'][6] = 0x63; fontData['V'][7] = 0x36;
    fontData['V'][8] = 0x36; fontData['V'][9] = 0x1C; fontData['V'][10] = 0x1C; fontData['V'][11] = 0x08;
    
    fontData['W'][0] = 0x63; fontData['W'][1] = 0x63; fontData['W'][2] = 0x63; fontData['W'][3] = 0x63;
    fontData['W'][4] = 0x63; fontData['W'][5] = 0x63; fontData['W'][6] = 0x6B; fontData['W'][7] = 0x6B;
    fontData['W'][8] = 0x7F; fontData['W'][9] = 0x77; fontData['W'][10] = 0x63; fontData['W'][11] = 0x63;
    
    fontData['X'][0] = 0x63; fontData['X'][1] = 0x63; fontData['X'][2] = 0x36; fontData['X'][3] = 0x36;
    fontData['X'][4] = 0x1C; fontData['X'][5] = 0x1C; fontData['X'][6] = 0x1C; fontData['X'][7] = 0x36;
    fontData['X'][8] = 0x36; fontData['X'][9] = 0x63; fontData['X'][10] = 0x63; fontData['X'][11] = 0x63;
    
    fontData['Y'][0] = 0x66; fontData['Y'][1] = 0x66; fontData['Y'][2] = 0x66; fontData['Y'][3] = 0x66;
    fontData['Y'][4] = 0x3C; fontData['Y'][5] = 0x18; fontData['Y'][6] = 0x18; fontData['Y'][7] = 0x18;
    fontData['Y'][8] = 0x18; fontData['Y'][9] = 0x18; fontData['Y'][10] = 0x18; fontData['Y'][11] = 0x18;
    
    fontData['Z'][0] = 0x7F; fontData['Z'][1] = 0x60; fontData['Z'][2] = 0x60; fontData['Z'][3] = 0x30;
    fontData['Z'][4] = 0x18; fontData['Z'][5] = 0x0C; fontData['Z'][6] = 0x06; fontData['Z'][7] = 0x03;
    fontData['Z'][8] = 0x03; fontData['Z'][9] = 0x03; fontData['Z'][10] = 0x03; fontData['Z'][11] = 0x7F;
    
    // a-z 
    
    fontData['a'][0] = 0x00; fontData['a'][1] = 0x00; fontData['a'][2] = 0x3E; fontData['a'][3] = 0x60;
    fontData['a'][4] = 0x7E; fontData['a'][5] = 0x63; fontData['a'][6] = 0x63; fontData['a'][7] = 0x63;
    fontData['a'][8] = 0x63; fontData['a'][9] = 0x63; fontData['a'][10] = 0x63; fontData['a'][11] = 0x7E;
    
    fontData['b'][0] = 0x03; fontData['b'][1] = 0x03; fontData['b'][2] = 0x3B; fontData['b'][3] = 0x67;
    fontData['b'][4] = 0x63; fontData['b'][5] = 0x63; fontData['b'][6] = 0x63; fontData['b'][7] = 0x63;
    fontData['b'][8] = 0x63; fontData['b'][9] = 0x67; fontData['b'][10] = 0x3B; fontData['b'][11] = 0x00;
    
    fontData['c'][0] = 0x00; fontData['c'][1] = 0x00; fontData['c'][2] = 0x3E; fontData['c'][3] = 0x63;
    fontData['c'][4] = 0x03; fontData['c'][5] = 0x03; fontData['c'][6] = 0x03; fontData['c'][7] = 0x03;
    fontData['c'][8] = 0x03; fontData['c'][9] = 0x63; fontData['c'][10] = 0x3E; fontData['c'][11] = 0x00;
    
    fontData['d'][0] = 0x60; fontData['d'][1] = 0x60; fontData['d'][2] = 0x6E; fontData['d'][3] = 0x73;
    fontData['d'][4] = 0x63; fontData['d'][5] = 0x63; fontData['d'][6] = 0x63; fontData['d'][7] = 0x63;
    fontData['d'][8] = 0x63; fontData['d'][9] = 0x73; fontData['d'][10] = 0x6E; fontData['d'][11] = 0x00;
    
    fontData['e'][0] = 0x00; fontData['e'][1] = 0x00; fontData['e'][2] = 0x3E; fontData['e'][3] = 0x63;
    fontData['e'][4] = 0x63; fontData['e'][5] = 0x7F; fontData['e'][6] = 0x03; fontData['e'][7] = 0x03;
    fontData['e'][8] = 0x03; fontData['e'][9] = 0x63; fontData['e'][10] = 0x3E; fontData['e'][11] = 0x00;
    
    fontData['f'][0] = 0x38; fontData['f'][1] = 0x0C; fontData['f'][2] = 0x0C; fontData['f'][3] = 0x3E;
    fontData['f'][4] = 0x0C; fontData['f'][5] = 0x0C; fontData['f'][6] = 0x0C; fontData['f'][7] = 0x0C;
    fontData['f'][8] = 0x0C; fontData['f'][9] = 0x0C; fontData['f'][10] = 0x0C; fontData['f'][11] = 0x00;
    
    fontData['g'][0] = 0x00; fontData['g'][1] = 0x00; fontData['g'][2] = 0x6E; fontData['g'][3] = 0x73;
    fontData['g'][4] = 0x63; fontData['g'][5] = 0x63; fontData['g'][6] = 0x63; fontData['g'][7] = 0x63;
    fontData['g'][8] = 0x63; fontData['g'][9] = 0x73; fontData['g'][10] = 0x6E; fontData['g'][11] = 0x60;
    
    fontData['h'][0] = 0x03; fontData['h'][1] = 0x03; fontData['h'][2] = 0x3B; fontData['h'][3] = 0x67;
    fontData['h'][4] = 0x63; fontData['h'][5] = 0x63; fontData['h'][6] = 0x63; fontData['h'][7] = 0x63;
    fontData['h'][8] = 0x63; fontData['h'][9] = 0x63; fontData['h'][10] = 0x63; fontData['h'][11] = 0x00;
    
    fontData['i'][0] = 0x18; fontData['i'][1] = 0x00; fontData['i'][2] = 0x1C; fontData['i'][3] = 0x18;
    fontData['i'][4] = 0x18; fontData['i'][5] = 0x18; fontData['i'][6] = 0x18; fontData['i'][7] = 0x18;
    fontData['i'][8] = 0x18; fontData['i'][9] = 0x18; fontData['i'][10] = 0x3C; fontData['i'][11] = 0x00;
    
    fontData['j'][0] = 0x30; fontData['j'][1] = 0x00; fontData['j'][2] = 0x38; fontData['j'][3] = 0x30;
    fontData['j'][4] = 0x30; fontData['j'][5] = 0x30; fontData['j'][6] = 0x30; fontData['j'][7] = 0x30;
    fontData['j'][8] = 0x33; fontData['j'][9] = 0x33; fontData['j'][10] = 0x1E; fontData['j'][11] = 0x00;
    
    fontData['k'][0] = 0x03; fontData['k'][1] = 0x03; fontData['k'][2] = 0x33; fontData['k'][3] = 0x1B;
    fontData['k'][4] = 0x0F; fontData['k'][5] = 0x07; fontData['k'][6] = 0x0F; fontData['k'][7] = 0x1B;
    fontData['k'][8] = 0x33; fontData['k'][9] = 0x63; fontData['k'][10] = 0x63; fontData['k'][11] = 0x00;
    
    fontData['l'][0] = 0x1C; fontData['l'][1] = 0x18; fontData['l'][2] = 0x18; fontData['l'][3] = 0x18;
    fontData['l'][4] = 0x18; fontData['l'][5] = 0x18; fontData['l'][6] = 0x18; fontData['l'][7] = 0x18;
    fontData['l'][8] = 0x18; fontData['l'][9] = 0x18; fontData['l'][10] = 0x3C; fontData['l'][11] = 0x00;
    
    fontData['m'][0] = 0x00; fontData['m'][1] = 0x00; fontData['m'][2] = 0x77; fontData['m'][3] = 0x7F;
    fontData['m'][4] = 0x6B; fontData['m'][5] = 0x63; fontData['m'][6] = 0x63; fontData['m'][7] = 0x63;
    fontData['m'][8] = 0x63; fontData['m'][9] = 0x63; fontData['m'][10] = 0x63; fontData['m'][11] = 0x00;
    
    fontData['n'][0] = 0x00; fontData['n'][1] = 0x00; fontData['n'][2] = 0x3B; fontData['n'][3] = 0x67;
    fontData['n'][4] = 0x63; fontData['n'][5] = 0x63; fontData['n'][6] = 0x63; fontData['n'][7] = 0x63;
    fontData['n'][8] = 0x63; fontData['n'][9] = 0x63; fontData['n'][10] = 0x63; fontData['n'][11] = 0x00;
    
    fontData['o'][0] = 0x00; fontData['o'][1] = 0x00; fontData['o'][2] = 0x3E; fontData['o'][3] = 0x63;
    fontData['o'][4] = 0x63; fontData['o'][5] = 0x63; fontData['o'][6] = 0x63; fontData['o'][7] = 0x63;
    fontData['o'][8] = 0x63; fontData['o'][9] = 0x63; fontData['o'][10] = 0x3E; fontData['o'][11] = 0x00;
    
    fontData['p'][0] = 0x00; fontData['p'][1] = 0x00; fontData['p'][2] = 0x3B; fontData['p'][3] = 0x67;
    fontData['p'][4] = 0x63; fontData['p'][5] = 0x63; fontData['p'][6] = 0x63; fontData['p'][7] = 0x63;
    fontData['p'][8] = 0x67; fontData['p'][9] = 0x3B; fontData['p'][10] = 0x03; fontData['p'][11] = 0x03;
    
    fontData['q'][0] = 0x00; fontData['q'][1] = 0x00; fontData['q'][2] = 0x6E; fontData['q'][3] = 0x73;
    fontData['q'][4] = 0x63; fontData['q'][5] = 0x63; fontData['q'][6] = 0x63; fontData['q'][7] = 0x63;
    fontData['q'][8] = 0x73; fontData['q'][9] = 0x6E; fontData['q'][10] = 0x60; fontData['q'][11] = 0x60;
    
    fontData['r'][0] = 0x00; fontData['r'][1] = 0x00; fontData['r'][2] = 0x3B; fontData['r'][3] = 0x67;
    fontData['r'][4] = 0x03; fontData['r'][5] = 0x03; fontData['r'][6] = 0x03; fontData['r'][7] = 0x03;
    fontData['r'][8] = 0x03; fontData['r'][9] = 0x03; fontData['r'][10] = 0x03; fontData['r'][11] = 0x00;
    
    fontData['s'][0] = 0x00; fontData['s'][1] = 0x00; fontData['s'][2] = 0x3E; fontData['s'][3] = 0x63;
    fontData['s'][4] = 0x03; fontData['s'][5] = 0x0E; fontData['s'][6] = 0x38; fontData['s'][7] = 0x60;
    fontData['s'][8] = 0x60; fontData['s'][9] = 0x63; fontData['s'][10] = 0x3E; fontData['s'][11] = 0x00;
    
    fontData['t'][0] = 0x0C; fontData['t'][1] = 0x0C; fontData['t'][2] = 0x3E; fontData['t'][3] = 0x0C;
    fontData['t'][4] = 0x0C; fontData['t'][5] = 0x0C; fontData['t'][6] = 0x0C; fontData['t'][7] = 0x0C;
    fontData['t'][8] = 0x0C; fontData['t'][9] = 0x6C; fontData['t'][10] = 0x38; fontData['t'][11] = 0x00;
    
    fontData['u'][0] = 0x00; fontData['u'][1] = 0x00; fontData['u'][2] = 0x63; fontData['u'][3] = 0x63;
    fontData['u'][4] = 0x63; fontData['u'][5] = 0x63; fontData['u'][6] = 0x63; fontData['u'][7] = 0x63;
    fontData['u'][8] = 0x63; fontData['u'][9] = 0x73; fontData['u'][10] = 0x6E; fontData['u'][11] = 0x00;
    
    fontData['v'][0] = 0x00; fontData['v'][1] = 0x00; fontData['v'][2] = 0x63; fontData['v'][3] = 0x63;
    fontData['v'][4] = 0x63; fontData['v'][5] = 0x36; fontData['v'][6] = 0x36; fontData['v'][7] = 0x1C;
    fontData['v'][8] = 0x1C; fontData['v'][9] = 0x08; fontData['v'][10] = 0x00; fontData['v'][11] = 0x00;
    
    fontData['w'][0] = 0x00; fontData['w'][1] = 0x00; fontData['w'][2] = 0x63; fontData['w'][3] = 0x63;
    fontData['w'][4] = 0x63; fontData['w'][5] = 0x6B; fontData['w'][6] = 0x6B; fontData['w'][7] = 0x7F;
    fontData['w'][8] = 0x77; fontData['w'][9] = 0x63; fontData['w'][10] = 0x00; fontData['w'][11] = 0x00;
    
    fontData['x'][0] = 0x00; fontData['x'][1] = 0x00; fontData['x'][2] = 0x63; fontData['x'][3] = 0x36;
    fontData['x'][4] = 0x1C; fontData['x'][5] = 0x08; fontData['x'][6] = 0x1C; fontData['x'][7] = 0x36;
    fontData['x'][8] = 0x63; fontData['x'][9] = 0x63; fontData['x'][10] = 0x00; fontData['x'][11] = 0x00;
    
    fontData['y'][0] = 0x00; fontData['y'][1] = 0x00; fontData['y'][2] = 0x63; fontData['y'][3] = 0x63;
    fontData['y'][4] = 0x63; fontData['y'][5] = 0x63; fontData['y'][6] = 0x63; fontData['y'][7] = 0x73;
    fontData['y'][8] = 0x6E; fontData['y'][9] = 0x60; fontData['y'][10] = 0x63; fontData['y'][11] = 0x3E;
    
    fontData['z'][0] = 0x00; fontData['z'][1] = 0x00; fontData['z'][2] = 0x7F; fontData['z'][3] = 0x60;
    fontData['z'][4] = 0x30; fontData['z'][5] = 0x18; fontData['z'][6] = 0x0C; fontData['z'][7] = 0x06;
    fontData['z'][8] = 0x03; fontData['z'][9] = 0x7F; fontData['z'][10] = 0x00; fontData['z'][11] = 0x00;
    
    // Special characters
    fontData[':'][0] = 0x00; fontData[':'][1] = 0x00; fontData[':'][2] = 0x18; fontData[':'][3] = 0x18;
    fontData[':'][4] = 0x00; fontData[':'][5] = 0x00; fontData[':'][6] = 0x00; fontData[':'][7] = 0x00;
    fontData[':'][8] = 0x18; fontData[':'][9] = 0x18; fontData[':'][10] = 0x00; fontData[':'][11] = 0x00;
    
    fontData['.'][0] = 0x00; fontData['.'][1] = 0x00; fontData['.'][2] = 0x00; fontData['.'][3] = 0x00;
    fontData['.'][4] = 0x00; fontData['.'][5] = 0x00; fontData['.'][6] = 0x00; fontData['.'][7] = 0x00;
    fontData['.'][8] = 0x00; fontData['.'][9] = 0x00; fontData['.'][10] = 0x18; fontData['.'][11] = 0x18;
    
    fontData[','][0] = 0x00; fontData[','][1] = 0x00; fontData[','][2] = 0x00; fontData[','][3] = 0x00;
    fontData[','][4] = 0x00; fontData[','][5] = 0x00; fontData[','][6] = 0x00; fontData[','][7] = 0x00;
    fontData[','][8] = 0x18; fontData[','][9] = 0x18; fontData[','][10] = 0x0C; fontData[','][11] = 0x00;
    
    fontData[';'][0] = 0x00; fontData[';'][1] = 0x00; fontData[';'][2] = 0x18; fontData[';'][3] = 0x18;
    fontData[';'][4] = 0x00; fontData[';'][5] = 0x00; fontData[';'][6] = 0x00; fontData[';'][7] = 0x00;
    fontData[';'][8] = 0x18; fontData[';'][9] = 0x18; fontData[';'][10] = 0x0C; fontData[';'][11] = 0x00;
    
    fontData['!'][0] = 0x18; fontData['!'][1] = 0x18; fontData['!'][2] = 0x18; fontData['!'][3] = 0x18;
    fontData['!'][4] = 0x18; fontData['!'][5] = 0x18; fontData['!'][6] = 0x18; fontData['!'][7] = 0x18;
    fontData['!'][8] = 0x00; fontData['!'][9] = 0x00; fontData['!'][10] = 0x18; fontData['!'][11] = 0x18;
    
    fontData['?'][0] = 0x3E; fontData['?'][1] = 0x63; fontData['?'][2] = 0x60; fontData['?'][3] = 0x30;
    fontData['?'][4] = 0x18; fontData['?'][5] = 0x18; fontData['?'][6] = 0x18; fontData['?'][7] = 0x18;
    fontData['?'][8] = 0x00; fontData['?'][9] = 0x00; fontData['?'][10] = 0x18; fontData['?'][11] = 0x18;

    fontData['>'][0] = 0x00; fontData['>'][1] = 0x03; fontData['>'][2] = 0x06; fontData['>'][3] = 0x0C;
    fontData['>'][4] = 0x18; fontData['>'][5] = 0x30; fontData['>'][6] = 0x18; fontData['>'][7] = 0x0C;
    fontData['>'][8] = 0x06; fontData['>'][9] = 0x03; fontData['>'][10] = 0x00; fontData['>'][11] = 0x00;
    
    fontData['('][0] = 0x30; fontData['('][1] = 0x18; fontData['('][2] = 0x0C; fontData['('][3] = 0x06;
    fontData['('][4] = 0x03; fontData['('][5] = 0x03; fontData['('][6] = 0x03; fontData['('][7] = 0x03;
    fontData['('][8] = 0x06; fontData['('][9] = 0x0C; fontData['('][10] = 0x18; fontData['('][11] = 0x30;
    
    fontData[')'][0] = 0x06; fontData[')'][1] = 0x0C; fontData[')'][2] = 0x18; fontData[')'][3] = 0x30;
    fontData[')'][4] = 0x60; fontData[')'][5] = 0x60; fontData[')'][6] = 0x60; fontData[')'][7] = 0x60;
    fontData[')'][8] = 0x30; fontData[')'][9] = 0x18; fontData[')'][10] = 0x0C; fontData[')'][11] = 0x06;
    
    fontData['['][0] = 0x3C; fontData['['][1] = 0x0C; fontData['['][2] = 0x0C; fontData['['][3] = 0x0C;
    fontData['['][4] = 0x0C; fontData['['][5] = 0x0C; fontData['['][6] = 0x0C; fontData['['][7] = 0x0C;
    fontData['['][8] = 0x0C; fontData['['][9] = 0x0C; fontData['['][10] = 0x0C; fontData['['][11] = 0x3C;
    
    fontData[']'][0] = 0x3C; fontData[']'][1] = 0x30; fontData[']'][2] = 0x30; fontData[']'][3] = 0x30;
    fontData[']'][4] = 0x30; fontData[']'][5] = 0x30; fontData[']'][6] = 0x30; fontData[']'][7] = 0x30;
    fontData[']'][8] = 0x30; fontData[']'][9] = 0x30; fontData[']'][10] = 0x30; fontData[']'][11] = 0x3C;
    
    fontData['-'][0] = 0x00; fontData['-'][1] = 0x00; fontData['-'][2] = 0x00; fontData['-'][3] = 0x00;
    fontData['-'][4] = 0x00; fontData['-'][5] = 0x7F; fontData['-'][6] = 0x7F; fontData['-'][7] = 0x00;
    fontData['-'][8] = 0x00; fontData['-'][9] = 0x00; fontData['-'][10] = 0x00; fontData['-'][11] = 0x00;
    
    fontData['_'][0] = 0x00; fontData['_'][1] = 0x00; fontData['_'][2] = 0x00; fontData['_'][3] = 0x00;
    fontData['_'][4] = 0x00; fontData['_'][5] = 0x00; fontData['_'][6] = 0x00; fontData['_'][7] = 0x00;
    fontData['_'][8] = 0x00; fontData['_'][9] = 0x00; fontData['_'][10] = 0x00; fontData['_'][11] = 0x7F;
    
    fontData['='][0] = 0x00; fontData['='][1] = 0x00; fontData['='][2] = 0x00; fontData['='][3] = 0x00;
    fontData['='][4] = 0x7F; fontData['='][5] = 0x7F; fontData['='][6] = 0x00; fontData['='][7] = 0x00;
    fontData['='][8] = 0x7F; fontData['='][9] = 0x7F; fontData['='][10] = 0x00; fontData['='][11] = 0x00;
    
    fontData['+'][0] = 0x00; fontData['+'][1] = 0x00; fontData['+'][2] = 0x18; fontData['+'][3] = 0x18;
    fontData['+'][4] = 0x18; fontData['+'][5] = 0x7E; fontData['+'][6] = 0x7E; fontData['+'][7] = 0x18;
    fontData['+'][8] = 0x18; fontData['+'][9] = 0x18; fontData['+'][10] = 0x00; fontData['+'][11] = 0x00;
    
    fontData['*'][0] = 0x00; fontData['*'][1] = 0x00; fontData['*'][2] = 0x36; fontData['*'][3] = 0x36;
    fontData['*'][4] = 0x1C; fontData['*'][5] = 0x7F; fontData['*'][6] = 0x7F; fontData['*'][7] = 0x1C;
    fontData['*'][8] = 0x36; fontData['*'][9] = 0x36; fontData['*'][10] = 0x00; fontData['*'][11] = 0x00;
    
    fontData['/'][0] = 0x00; fontData['/'][1] = 0x60; fontData['/'][2] = 0x30; fontData['/'][3] = 0x18;
    fontData['/'][4] = 0x0C; fontData['/'][5] = 0x06; fontData['/'][6] = 0x03; fontData['/'][7] = 0x06;
    fontData['/'][8] = 0x0C; fontData['/'][9] = 0x18; fontData['/'][10] = 0x30; fontData['/'][11] = 0x60;
    
    fontData['\\'][0] = 0x00; fontData['\\'][1] = 0x03; fontData['\\'][2] = 0x06; fontData['\\'][3] = 0x0C;
    fontData['\\'][4] = 0x18; fontData['\\'][5] = 0x30; fontData['\\'][6] = 0x60; fontData['\\'][7] = 0x30;
    fontData['\\'][8] = 0x18; fontData['\\'][9] = 0x0C; fontData['\\'][10] = 0x06; fontData['\\'][11] = 0x03;
    
    fontData['@'][0] = 0x3E; fontData['@'][1] = 0x63; fontData['@'][2] = 0x6F; fontData['@'][3] = 0x6B;
    fontData['@'][4] = 0x6B; fontData['@'][5] = 0x6F; fontData['@'][6] = 0x03; fontData['@'][7] = 0x03;
    fontData['@'][8] = 0x63; fontData['@'][9] = 0x63; fontData['@'][10] = 0x63; fontData['@'][11] = 0x3E;
    
    fontData['#'][0] = 0x00; fontData['#'][1] = 0x36; fontData['#'][2] = 0x36; fontData['#'][3] = 0x7F;
    fontData['#'][4] = 0x7F; fontData['#'][5] = 0x36; fontData['#'][6] = 0x36; fontData['#'][7] = 0x7F;
    fontData['#'][8] = 0x7F; fontData['#'][9] = 0x36; fontData['#'][10] = 0x36; fontData['#'][11] = 0x00;
    
    fontData['$'][0] = 0x18; fontData['$'][1] = 0x3E; fontData['$'][2] = 0x63; fontData['$'][3] = 0x03;
    fontData['$'][4] = 0x0E; fontData['$'][5] = 0x38; fontData['$'][6] = 0x60; fontData['$'][7] = 0x63;
    fontData['$'][8] = 0x63; fontData['$'][9] = 0x3E; fontData['$'][10] = 0x18; fontData['$'][11] = 0x18;
    
    fontData['%'][0] = 0x00; fontData['%'][1] = 0x63; fontData['%'][2] = 0x66; fontData['%'][3] = 0x0C;
    fontData['%'][4] = 0x18; fontData['%'][5] = 0x30; fontData['%'][6] = 0x60; fontData['%'][7] = 0x30;
    fontData['%'][8] = 0x18; fontData['%'][9] = 0x0C; fontData['%'][10] = 0x66; fontData['%'][11] = 0x63;
    
    fontData['&'][0] = 0x1C; fontData['&'][1] = 0x36; fontData['&'][2] = 0x36; fontData['&'][3] = 0x1C;
    fontData['&'][4] = 0x3E; fontData['&'][5] = 0x63; fontData['&'][6] = 0x63; fontData['&'][7] = 0x63;
    fontData['&'][8] = 0x63; fontData['&'][9] = 0x73; fontData['&'][10] = 0x6E; fontData['&'][11] = 0x00;
    
    fontData['^'][0] = 0x18; fontData['^'][1] = 0x3C; fontData['^'][2] = 0x66; fontData['^'][3] = 0x00;
    fontData['^'][4] = 0x00; fontData['^'][5] = 0x00; fontData['^'][6] = 0x00; fontData['^'][7] = 0x00;
    fontData['^'][8] = 0x00; fontData['^'][9] = 0x00; fontData['^'][10] = 0x00; fontData['^'][11] = 0x00;
    
    fontData['~'][0] = 0x00; fontData['~'][1] = 0x00; fontData['~'][2] = 0x00; fontData['~'][3] = 0x00;
    fontData['~'][4] = 0x31; fontData['~'][5] = 0x4A; fontData['~'][6] = 0x44; fontData['~'][7] = 0x00;
    fontData['~'][8] = 0x00; fontData['~'][9] = 0x00; fontData['~'][10] = 0x00; fontData['~'][11] = 0x00;
    
    // Space
    for (int i = 0; i < 12; i++) {
        fontData[' '][i] = 0x00;
    }
}
