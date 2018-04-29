/*
 * textCharacters.h
 *
 *  Created on: Mar 26, 2018
 *      Author: GFV
 */

#ifndef TEXTCHARACTERS_H_
#define TEXTCHARACTERS_H_

#include <stdint.h>

struct pov_char {
	const uint8_t data[5];
};

class textCharacters {
	friend class TeensyPOV;
private:
	static const pov_char font_numbers[];
	static const pov_char font_uppercase[];
	static const pov_char font_lowercase[];
	static const pov_char font_space;
	static void getMatrix(const char, uint8_t *, bool);
};

#endif /* TEXTCHARACTERS_H_ */
