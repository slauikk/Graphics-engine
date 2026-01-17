#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <map>

extern std::map<char, unsigned char[12]> fontData;

void initFontData();

#endif // FONT_DATA_H
