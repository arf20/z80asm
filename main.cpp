#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "kk_ihex_write.h"

// aux

bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos) return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

bool removePreSpace(std::string& str) {
	int i = 0;
	while (str[i] == ' ' || str[i] == '\t') i++;
	str = str.substr(i, str.length() - i);
	return true;
}

int findSpace(std::string& str) {
	int i = 0;
	while ((str[i] != ' ' && str[i] != '\t') && i < str.length()) i++;
	if (i == str.length()) return -1;
	return i;
}

constexpr char hexmap[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

std::string hexStr(uint8_t *data, int len)
{
	std::string s(len * 2, '\0');
	for (int i = 0; i < len; i++) {
		s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
		s[2 * i + 1] = hexmap[data[i] & 0x0F];
	}
	return s;
}

std::string hexStr(uint16_t *data, int len)
{
	std::string s(len * 2, '\0');
	for (int i = 0; i < len; i++) {
		s[2 * i] = hexmap[(data[i] & 0xF0) >> 4];
		s[2 * i + 1] = hexmap[data[i] & 0x0F];
	}
	return s;
}

bool isPrintable(unsigned char chr) {
	int chrVal = (int)chr;
	if (isprint(chrVal) != 0) return true; else return false;
}

void listByteA(uint8_t *arr, int size, uint16_t off = 0x0000) {
	std::cout << "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f" << std::endl;

	uint8_t vOffl = off & 0xff;
	uint8_t vOffh = (off >> 8) & 0xff;

	int i = 0;
	while (i < size) {
		//cout << hexStr(&vOffl, 1) << hexStr(&vOffh, 1) << " ";
		std::cout << hexStr(&vOffh, 1) << hexStr(&vOffl, 1) << " ";

		char txt[16] = { '\0' };

		for (int j = 0; j < 16; j++) {
			if (i >= size) { break; }
			std::cout << hexStr(&arr[i], 1) << " ";
			if (isPrintable((char)arr[i]) == true) txt[j] = (char)arr[i]; else txt[j] == (char)0;
			i++;
		}

		for (int j = 0; j < 16; j++) {
			if (txt[j] != 0)
				std::cout << txt[j];
			else
				std::cout << ".";
		}
		std::cout << std::endl;

		if (vOffl == 0xF0) {
			vOffh += 0x01;
			vOffl = 0;
		}
		else {
			vOffl += 0x10;
		}

	}

	std::cout << std::endl;
}


// main

struct instruction {
	std::string opcode;
	std::vector<std::string> params;
	int line = 0;
	int size = 0;
	std::string binary;
};

struct symbol {
	std::string label;
	uint16_t address = 0;
	uint16_t size = 0;
	bool type = false;	// 0 = data, 1 = code
	int line = 0;
	std::vector<std::string> defStrs;
	std::vector<instruction> def;
	//std::string binaryBlock;
};

bool isLabelorAddr(std::string& str, std::vector<symbol> *symbolTable) {
	if (str != "") {
		for (symbol& s : *symbolTable) {
			if (str == s.label) return true;
		}

		if (str[0] == '$') {

		}
	}
}

uint16_t getLabelAddress(std::string& str, std::vector<symbol> *symbolTable) {
	for (symbol& s : *symbolTable) {
		if (str == s.label) {
			return s.address;
		}
	}
}

bool is16DecimalNum(std::string& str) {
	char* p;
	unsigned int n = std::strtol(str.c_str(), &p, 10);
	if (*p) {
		//std::cout << "Numeric error: NaN, Not a number. Specify a valid 16 bit unsigned integer" << std::endl;
		return false;
	}
	else if (n >= 0 && n <= 0xffff) return true;
	else {
		if (n < 0)
			std::cout << "Numeric error: Negative number. Negative numbers are forbidden. Specify a valid 16 bit unsigned integer" << std::endl;
		else if (n > 0xffff)
			std::cout << "Numeric error: Overflow. Numbers grater than 65535 are not representable in 16 bits. Specify a valid 16 bit unsigned integer" << std::endl;
		return false;
	}
}

bool is8DecimalNum(std::string& str) {
	char* p;
	unsigned int n = std::strtol(str.c_str(), &p, 10);
	if (*p) return false;
	else if (n >= 0 && n <= 0xff) return true;
	else {
		if (n < 0)
			std::cout << "Numeric error: Negative number. Negative numbers are forbidden. Specify a valid 16 bit unsigned integer" << std::endl;
		else if (n > 0xffff)
			std::cout << "Numeric error: Overflow. Numbers grater than 255 are not representable in 16 bits. Specify a valid 8 bit unsigned integer" << std::endl;
		return false;
	}
}

bool is3DecimalNum(std::string& str) {
	char* p;
	unsigned int n = std::strtol(str.c_str(), &p, 10);
	if (*p) return false;
	else if (n >= 0 && n <= 7) return true;
	else return false;
}

bool is16Hex(std::string& str) {
	if (str[str.length()] != 'h') return false;
	str = str.substr(0, str.length() - 1);
	char* p;
	unsigned int n = std::strtol(str.c_str(), &p, 16);
	if (*p) return false;
	else if (n > 0 && n < 0xffff) return true;
	else return false;
}

bool is8Hex(std::string& str) {
	if (str[str.length()] != 'h') return false;
	str = str.substr(0, str.length() - 1);
	char* p;
	unsigned int n = std::strtol(str.c_str(), &p, 16);
	if (*p) return false;
	else if (n > 0 && n < 0xff) return true;
	else return false;
}

bool isCond(std::string& str) {
	return (str == "NZ" || str == "Z" || str == "NC" || str == "C" || str == "PO" || str == "PE" || str == "P" || str == "M");
}

uint8_t toCond(std::string& str) {
	if (str == "NZ")
		return 0b000;
	else if (str == "Z")
		return 0b001;
	else if (str == "NC")
		return 0b010;
	else if (str == "C")
		return 0b011;
	else if (str == "PO")
		return 0b100;
	else if (str == "PE")
		return 0b101;
	else if (str == "P")
		return 0b110;
	else if (str == "M")
		return 0b111;
}

bool isRegister(std::string& str) {
	return (str == "A" || str == "B" || str == "C" || str == "D" || str == "E" || str == "H" || str == "L");
}

uint8_t toRegister(std::string& str) {
	if (str == "A")
		return 0b111;
	else if (str == "B")
		return 0b000;
	else if (str == "C")
		return 0b001;
	else if (str == "D")
		return 0b010;
	else if (str == "E")
		return 0b011;
	else if (str == "H")
		return 0b100;
	else if (str == "L")
		return 0b101;
}

bool isRegisterPair(std::string& str) {
	return (str == "BC" || str == "DE" || str == "HL" || str == "SP");
}

uint8_t toRegisterPair(std::string& str) {
	if (str == "BC")
		return 0b00;
	else if (str == "DE")
		return 0b01;
	else if (str == "HL")
		return 0b10;
	else if (str == "SP")
		return 0b11;
}

uint8_t toRstAddr(std::string& str) {
	if (str == "00h") return 0b000;
	else if (str == "08h") return 0b001;
	else if (str == "10h") return 0b010;
	else if (str == "18h") return 0b011;
	else if (str == "20h") return 0b100;
	else if (str == "28h") return 0b101;
	else if (str == "30h") return 0b110;
	else if (str == "38h") return 0b111;
}

void invalidParams(instruction *i) {
	std::cout << "Syntax error: Invalid parameters: " << i->opcode << " ";

	if ((i->params[0] != "") || (i->params[1] != "")) {
		int t = 0;
		for (std::string& p : i->params) {
			if (t != 0) std::cout << ", ";
			std::cout << p;
			t++;
		}
	}

	std::cout << std::endl;
}



std::string intelhex = "";

void ihex_flush_buffer(struct ihex_state *ihex, char *buffer, char *eptr) {
	*eptr = '\0';
	intelhex += std::string(buffer);
}

void usage() {
	std::cout << "Usage: z80asm <assembly file> [-o <output name>]" << std::endl;
}

int main(int argc, char **argv) {
	std::string inputPath = "", outputPath = "";

	if (argc == 1) {
		std::cout << "Too few arguments" << std::endl << std::endl;
		usage();
		return 1;
	}
	else if (argc > 1) {
		for (int i = 1; i < argc; i++)
			if (argv[i][0] != '-') { inputPath = std::string(argv[i]); break; }

		for (int i = 1; i < argc; i++)
			if ((std::string(argv[i]) == "-o") && (i + 1 < argc)) outputPath = std::string(argv[i + 1]);
	}



	// ======= LOAD ASSEMBLY =======
	std::ifstream inputFile = std::ifstream(inputPath, std::ios::binary);
	if (!inputFile) return 1;

	std::string line = "";
	std::vector<std::string> lines;

	while (inputFile) {
		std::getline(inputFile, line);
		lines.push_back(line);
	}

	lines.pop_back();

	std::cout << "Assembly: " << std::endl << std::endl;
	for (std::string& l : lines)
		std::cout << "\t" << l << std::endl;

	// ======= FIRST PASS =======
	std::vector<symbol> symbolTable;
	uint16_t org = 0;

	for (int i = 0; i < lines.size(); i++) {
		std::string l = lines[i];

		replace(l, "\r", "");
		replace(l, "\n", "");

		removePreSpace(l);

		int idxCol = l.find(":");
		if (idxCol != std::string::npos) {				// Label
			struct symbol s;

			std::string label = "";
			label = l.substr(0, idxCol);

			s.label = label;
			s.line = i;

			std::string instruction = "";
			instruction = l.substr(idxCol + 1, l.length() - (idxCol + 1));
			removePreSpace(instruction);

			/*std::string params = "";
			int space = findSpace(instruction);
			params = instruction.substr(space, instruction.length() - space);
			removePreSpace(params);

			instruction = instruction.substr(0, space);*/

			int j = i + 1;
			while (j < lines.size()) {
				std::string sl = lines[j];
				replace(sl, "\r", "");
				replace(sl, "\n", "");
				removePreSpace(sl);

				int idxCol = sl.find(":");
				if (idxCol != std::string::npos) break;				// Next label
				j++;
			}

			std::vector<std::string> def;
			if (instruction != "") def.push_back(instruction);
			for (int k = i + 1; k < j; k++) {
				std::string sl = lines[k];
				replace(sl, "\r", "");
				replace(sl, "\n", "");
				removePreSpace(sl);

				if (sl != "") def.push_back(sl);
			}

			s.defStrs = def;

			symbolTable.push_back(s);
		}
		else {											// No label
			int idxOrg = l.find("org");
			if (idxOrg != std::string::npos) {
				std::string params = "";
				int space = findSpace(l);
				params = l.substr(space, l.length() - space);
				removePreSpace(params);

				if (params[params.size() - 1] == 'h') {
					org = (uint16_t)strtol(params.substr(0, params.size() - 1).c_str(), 0, 16);
				}
			}
		}
	}

	int instLine = 0;

	for (struct symbol& s : symbolTable) {
		for (std::string& l : s.defStrs) {
			struct instruction inst;

			int space = findSpace(l);

			std::string paramsStr;

			if (space != -1) {
				inst.opcode = l.substr(0, space);
				paramsStr = l.substr(space + 1, l.size() - (space + 1));
				replace(paramsStr, " ", "");
			}
			else
				inst.opcode = l;

			if (paramsStr != "") {
				int idxTokenSep = paramsStr.find(",");
				while (idxTokenSep != std::string::npos) {
					inst.params.push_back(paramsStr.substr(0, idxTokenSep));
					paramsStr = paramsStr.substr(idxTokenSep + 1, paramsStr.length() - (idxTokenSep + 1));
					idxTokenSep = paramsStr.find(",");
				}

				if (paramsStr != "")
					inst.params.push_back(paramsStr);
			}

			inst.line = instLine;

			s.def.push_back(inst);

			instLine++;
		}
	}

	for (struct symbol& s : symbolTable) {
		for (instruction& l : s.def) {
			l.size = 0;

			// =============== DATA ===============
			if (l.opcode == "db") {
				s.type = false;
				l.size += 1;
				continue;
			}

			else if (l.opcode == "dw") {
				s.type = false;
				l.size += 2;
				continue;
			}

			// parameters should be always size of 2 but in data
			if (l.params.size() == 0) {
				l.params.push_back("");
				l.params.push_back("");
			}

			if (l.params.size() == 1) {
				l.params.push_back("");
			}

			// =============== MEMORY/REGISTERS ===============
			if (l.opcode == "ld") {
				s.type == true;

				if (		(l.params[0] == "(BC)" &&				l.params[1] == "A") ||					// ld (BC), A
							(l.params[0] == "(DE)" &&				l.params[1] == "A")	||					// ld (DE), A
							(l.params[0] == "(HL)" &&				isRegister(l.params[1])) ||				// ld (HL), r
							(l.params[0] == "A" &&					l.params[1] == "(BC)") ||				// ld A, (BC)
							(l.params[0] == "A" &&					l.params[1] == "(DE)") ||				// ld A, (DE)
							(l.params[0] == "SP" &&					l.params[1] == "HL") ||					// ld SP, HL
							(isRegister(l.params[0]) &&				l.params[1] == "(HL)")					// ld r, HL
					)	l.size += 1;
				else if (	(isRegister(l.params[0]) &&				is8DecimalNum(l.params[1])) ||			// ld r, n
							(isRegister(l.params[0]) &&				isRegister(l.params[1])) ||				// ld r, r'
							(l.params[0] == "(HL)" &&				is8DecimalNum(l.params[1])) ||			// ld (HL), n
							(l.params[0] == "A" &&					l.params[1] == "I") ||					// ld A, I
							(l.params[0] == "I" &&					l.params[1] == "A") ||					// ld I, A
							(l.params[0] == "A" &&					l.params[1] == "R") ||					// ld A, R
							(l.params[0] == "R" &&					l.params[1] == "A")						// ld R, A
							
					)	l.size += 2;
				else if (	(isRegisterPair(l.params[0]) &&			is16DecimalNum(l.params[1])) ||			// ld rr, nn
							(l.params[0] == "A" &&					isLabelorAddr(l.params[1], &symbolTable)) ||	// ld A, (nn)
							(isLabelorAddr(l.params[0], &symbolTable) &&	l.params[1] == "A")						// ld (nn), A
					)	l.size += 3;
				else if (	(isRegisterPair(l.params[0]) &&			isLabelorAddr(l.params[1], &symbolTable)) ||	// ld rr, (nn)
							(isLabelorAddr(l.params[0], &symbolTable) &&	isRegisterPair(l.params[1]))			// ld (nn), rr
					)	l.size += 4;
				else invalidParams(&l);
			}

			else if (l.opcode == "ldd") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// ldd
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "lddr") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// lddr
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "ldi") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// ldi
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "ldir") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// ldir
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "push") {
				s.type = true;

				if (		(isRegisterPair(l.params[0]))													// push rr
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "pop") {
				s.type = true;

				if (		(isRegisterPair(l.params[0]))													// pop rr
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "ex") {
				s.type = true;

				if (		(l.params[0] == "AF" &&					l.params[1] == "AF'") ||				// ex AF, AF'
							(l.params[0] == "DE" &&					l.params[1] == "HL") ||					// ex DE, HL
							(l.params[0] == "(SP)" &&				l.params[1] == "HL")					// ex (SP), HL
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "exx") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// exx
					)	l.size += 1;
				else invalidParams(&l);
			}

			// =============== DEVICE I/O ===============

			else if (l.opcode == "in") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "(C)") ||				// in r, (C)
							(l.params[0] == "A" &&					is8Hex(l.params[1]))					// in A, (n)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "ind") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// ind
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "indr") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// indr
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "ini") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// ini
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "inir") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// inir
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "out") {
				s.type = true;

				if (		(l.params[0] == "(C)" &&				isRegister(l.params[1])) ||				// out (C), r
							(is8Hex(l.params[1]) &&					l.params[0] == "A")						// out (n), r
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "outd") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// outd
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "outi") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// outi
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "otir") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// otir
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "otdr") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// otdr
					)	l.size += 2;
				else invalidParams(&l);
			}

			// =============== ARITHMETIC ===============

			else if (l.opcode == "inc") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// inc r
							(isRegisterPair(l.params[0]) &&			l.params[1] == "") ||					// inc rr
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// inc (HL)
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "dec") {
				s.type = true;

				if ((isRegister(l.params[0]) &&						l.params[1] == "") ||					// dec r
					(isRegisterPair(l.params[0]) &&					l.params[1] == "") ||					// dec rr
					(l.params[0] == "(HL)" &&						l.params[1] == "")						// dec (HL)
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "add") {
				s.type = true;

				if (		(l.params[0] == "HL" &&					isRegisterPair(l.params[1])) ||			// add HL, rr
							(l.params[0] == "A" &&					l.params[1] == "(HL)") ||				// add A, (HL)
							(l.params[0] == "A" &&					isRegister(l.params[1]))				// add A, r
					)	l.size += 1;
				else if (	(l.params[0] == "A" &&					is8DecimalNum(l.params[1]))				// add A, n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "adc") {
				s.type = true;

				if (		(l.params[0] == "A" &&					isRegister(l.params[1])) ||				// adc A, r
							(l.params[0] == "A" &&					l.params[1] == "(HL)")					// adc A, (HL)
					)	l.size += 1;
				else if (
							(l.params[0] == "A" &&					is8DecimalNum(l.params[1])) ||			// adc A, n
							(l.params[0] == "HL" &&					isRegisterPair(l.params[1]))			// adc HL, rr
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "sub") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// sub r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// sub (HL)
					)	l.size += 1;
				else if (	(is8DecimalNum(l.params[0]) &&			l.params[1] == "")						// sub n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "sbc") {
				s.type = true;

				if (		(l.params[0] == "A" &&					isRegister(l.params[1])) ||				// sbc A, r
							(l.params[0] == "A" &&					l.params[1] == "(HL)")					// sbc A, (HL)
					)	l.size += 1;
				else if (	(l.params[0] == "A" &&					is8DecimalNum(l.params[1])) ||			// sbc A, n
							(l.params[0] == "HL" &&					isRegisterPair(l.params[1]))			// sbc HL, rr
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "daa") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// daa
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "neg") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// neg
					)	l.size += 2;
				else invalidParams(&l);
			}

			// =============== LOGIC ===============

			else if (l.opcode == "and") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// and r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// and (HL)
					)	l.size += 1;
				else if (	(is8DecimalNum(l.params[0]) &&			l.params[1] == "")						// and n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "or") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// or r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// or (HL)
					)	l.size += 1;
				else if (	(is8DecimalNum(l.params[0]) &&			l.params[1] == "")						// or n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "xor") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// xor r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// xor (HL)
					)	l.size += 1;
				else if (	(is8DecimalNum(l.params[0]) &&			l.params[1] == "")						// xor n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "cpl") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// cpl [A = ~A]
					)	l.size += 1;
				else invalidParams(&l);
			}

			// =============== FLAGS ===============

			else if (l.opcode == "scf") {
				if (		(l.params[0] == "" &&					l.params[1] == "")						// scf
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "ccf") {
				if (		(l.params[0] == "" &&					l.params[1] == "")						// ccf
					)	l.size += 1;
				else invalidParams(&l);
			}

			// =============== BIT SET/RESET/TEST ===============

			else if (l.opcode == "set") {
				if (		(is3DecimalNum(l.params[0]) &&			isRegister(l.params[1])) ||				// set n, r
							(is3DecimalNum(l.params[0]) &&			l.params[1] == "(HL)")					// set n, (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "res") {
				if (		(is3DecimalNum(l.params[0]) &&			isRegister(l.params[1])) ||				// res n, r
							(is3DecimalNum(l.params[0]) &&			l.params[1] == "(HL)")					// res n, (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "bit") {
				if (		(is3DecimalNum(l.params[0]) &&			isRegister(l.params[1]))				// bit n, r
					)	l.size += 2;
				else invalidParams(&l);
			}

			// =============== ROTATE/SHIFT ===============

			// rotate
			else if (l.opcode == "rl") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// rl r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// rl (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "rla") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// rla
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "rlca") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// rlca
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "rlc") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// rlc r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// rlc (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "rld") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// rld
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "rr") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// rr r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// rr (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "rra") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// rra
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "rrc") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// rrc r
							(l.params[0] == "(HL)")															// rrc (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "rrca") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// rrca
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "rrd") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// rrd
					)	l.size += 2;
				else invalidParams(&l);
			}

			// shift
			else if (l.opcode == "sla") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// sla r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// sla (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "sra") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// sra r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// sra (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "srl") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// srl r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// srl (HL)
					)	l.size += 2;
				else invalidParams(&l);
			}

			// =============== CONTROL ===============

			else if (l.opcode == "cp") {
				s.type = true;

				if (		(isRegister(l.params[0]) &&				l.params[1] == "") ||					// cp r
							(l.params[0] == "(HL)" &&				l.params[1] == "")						// cp (HL)
					)	l.size += 1;
				if (		(is8DecimalNum(l.params[0]) &&			l.params[1] == "")						// cp n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "cpd") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// cpd
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "cpdr") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// cpdr
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "cpi") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// cpi
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "cpir") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// cpir
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "jp") {
				s.type = true;

				if (		(l.params[0] == "(HL)" &&				l.params[1] == "")						// jmp (HL)
					)	l.size += 1;
				else if (	(isCond(l.params[0]) &&					isLabelorAddr(l.params[1], &symbolTable)) ||	// jmp cc, (nn)
							(isLabelorAddr(l.params[0], &symbolTable) &&	l.params[1] == "")						// jmp (nn)
					)	l.size += 3;
				else invalidParams(&l);
			}

			else if (l.opcode == "jr") {
				s.type = true;

				if (		(isCond(l.params[0]) &&					is8Hex(l.params[1])) ||					// jr cc, n
							(is8Hex(l.params[0]) &&					l.params[1] == "")						// jr n
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "call") {
				s.type = true;

				if (		(isCond(l.params[0]) &&					isLabelorAddr(l.params[1], &symbolTable)) ||	// call cc, (nn)
							(isLabelorAddr(l.params[0], &symbolTable) &&	l.params[1] == "")						// call (nn)
					)	l.size += 3;
				else invalidParams(&l);
			}

			else if (l.opcode == "ret") {
				s.type = true;

				if (		(isCond(l.params[0]) &&					l.params[1] == "") ||					// ret cc
							(l.params[0] == "" &&					l.params[1] == "")						// ret
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "reti") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// reti
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "retn") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// retn
					)	l.size += 2;
				else invalidParams(&l);
			}

			else if (l.opcode == "halt") {
				s.type = true;
				
				if (		(l.params[0] == "" &&					l.params[1] == "")						// halt
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "nop") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// nop
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "rst") {
				s.type = true;

				if (		(is8Hex(l.params[0]) &&					l.params[1] == "")						// rst (n)
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "djnz") {
				s.type = true;

				if (		(is8DecimalNum(l.params[0]) &&			l.params[1] == "")						// djnz n
					)	l.size += 2;
				else invalidParams(&l);
			}



			// =============== INTERRUPTS ===============

			else if (l.opcode == "di") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// di
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "ei") {
				s.type = true;

				if (		(l.params[0] == "" &&					l.params[1] == "")						// ei
					)	l.size += 1;
				else invalidParams(&l);
			}

			else if (l.opcode == "im") {
				s.type = true;

				if (		(l.params[0] == "0" &&					l.params[1] == "") ||					// im 0
							(l.params[0] == "1" &&					l.params[1] == "") ||					// im 1
							(l.params[0] == "2" &&					l.params[1] == "")						// im 2
					)	l.size += 2;
				else invalidParams(&l);
			}

			else {
				//int line = s.line;
				std::cout << "Syntax error: " << l.opcode << " is not a valid instruction opcode" << std::endl;
			}
		}
	}

	for (struct symbol& s : symbolTable) {
		s.size = 0;

		for (struct instruction& i : s.def) {
			s.size += i.size;
		}
	}

	uint16_t nextLabelPtr = org;
	for (struct symbol& s : symbolTable) {
		s.address = nextLabelPtr;
		nextLabelPtr += s.size;
	}

	std::cout << std::endl << "Symbol table:" << std::endl;
	std::cout << "\tLABEL\tADDR\tSIZE\tTYPE\tLINE\tLINES" << std::endl;
	for (struct symbol& s : symbolTable) {
		std::cout << "\t" << s.label
			<< "\t";
		printf("%04x", s.address);
			std::cout << "\t" << std::dec << s.size
			<< "\t" << s.type
			<< "\t" << s.line + 1
			<< "\t" << s.defStrs.size() << std::endl;
	}

	// ======= SECOND PASS =======
	for (struct symbol& s : symbolTable) {
		for (instruction& l : s.def) {
			// =============== DATA ===============

			if (l.opcode == "dw") {
				if (is16DecimalNum(l.params[0])) {
					uint16_t word = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)(word & 0xff);
					l.binary += (char)((word >> 8) & 0xff);
				}
			}
			else if (l.opcode == "dw") {
				if (is16DecimalNum(l.params[0])) {
					uint16_t word = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)(word & 0xff);
					l.binary += (char)((word >> 8) & 0xff);
				}
			}

			// =============== MEMORY/REGISTERS ===============
			if (l.opcode == "ld") {
				if			(isRegisterPair(l.params[0]) &&			isLabelorAddr(l.params[1], &symbolTable)) {	// ld rr, (nn)
					l.binary += (char)0b11101101;
					l.binary += (char)(0b01001011 | (toRegisterPair(l.params[0]) << 4));
					uint16_t labelAddr = getLabelAddress(l.params[1], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if			(isRegisterPair(l.params[0]) &&			is16DecimalNum(l.params[1])) {		// ld rr, nn
					l.binary += (char)(0b00000001 | (toRegisterPair(l.params[0]) << 4));
					uint16_t nn = std::strtol(l.params[1].c_str(), 0, 10);
					l.binary += nn & 0xff;
					l.binary += (nn >> 8) & 0xff;
				}
				else if			(isRegister(l.params[0]) &&				is8DecimalNum(l.params[1])) {		// ld r, n
					l.binary += (char)(0b00000110 | (toRegister(l.params[0]) << 3));
					uint8_t n = std::strtol(l.params[1].c_str(), 0, 10);
					l.binary += (char)n;
				}
				else if			(isRegister(l.params[0]) &&				isRegister(l.params[1])) {			// ld r, r'
					l.binary += (char)((0b01000000 | (toRegister(l.params[0]) << 3)) | (toRegister(l.params[1])));
				}
				else if			(l.params[0] == "(BC)" &&				l.params[1] == "A") {				// ld (BC), A
					l.binary += (char)(0b00000010);
				}
				else if		(l.params[0] == "(DE)" &&				l.params[1] == "A") {					// ld (DE), A
					l.binary += (char)(0b00010010);
				}
				else if			(l.params[0] == "(HL)" &&				is8DecimalNum(l.params[1])) {		// ld (HL), n
					l.binary += (char)(0b00110110);
					uint8_t n = std::strtol(l.params[1].c_str(), 0, 10);
					l.binary += n;
				}
				else if		(l.params[0] == "(HL)" &&				isRegister(l.params[1])) {				// ld (HL), r
					l.binary += (char)(0b01110000 | toRegister(l.params[1]));
				}
				else if		(l.params[0] == "A" &&					isLabelorAddr(l.params[1], &symbolTable)) {	// ld A, (nn)
					l.binary += (char)(0b00111010);
					uint16_t labelAddr = getLabelAddress(l.params[1], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if		(isLabelorAddr(l.params[0], &symbolTable) &&	l.params[1] == "A") {					// ld (nn), A
					l.binary += (char)(0b00110010);
					uint16_t labelAddr = getLabelAddress(l.params[0], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if (isLabelorAddr(l.params[0], &symbolTable) && isRegisterPair(l.params[1])) {				// ld (nn), rr
					l.binary += (char)0b11101101;
					l.binary += (char)(0b01000011 | (toRegisterPair(l.params[1]) << 4));
					uint16_t labelAddr = getLabelAddress(l.params[0], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if		(l.params[0] == "A" &&					l.params[1] == "(BC)") {				// ld A, (BC)
					l.binary += (char)(0b00001010);
				}
				else if		(l.params[0] == "A" &&					l.params[1] == "(DE)") {				// ld A, (DE)
					l.binary += (char)(0b00011010);
				}
				else if		(l.params[0] == "A" &&					l.params[1] == "I") {					// ld A, I
					l.binary += (char)0b11101101;
					l.binary += (char)0b01010111;
				}
				else if		(l.params[0] == "I" &&					l.params[1] == "A") {					// ld I, A
					l.binary += (char)0b11101101;
					l.binary += (char)0b01000111;
				}
				else if		(l.params[0] == "A" &&					l.params[1] == "R") {					// ld A, R
					l.binary += (char)0b11101101;
					l.binary += (char)0b01011111;
				}
				else if		(l.params[0] == "R" &&					l.params[1] == "A") {					// ld R, A
					l.binary += (char)0b11101101;
					l.binary += (char)0b01001111;
				}
				else if		(l.params[0] == "SP" &&					l.params[1] == "HL") {					// ld SP, HL
					l.binary += (char)0b11111001;
				}
				else if		(isRegister(l.params[0]) &&				l.params[1] == "(HL)") {				// ld r, (HL)
					l.binary += (char)(0b01000110 | (toRegister(l.params[0]) << 3));
				}
			}
			else if	(l.opcode == "ldd") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ldd
					l.binary += (char)0b11101101;
					l.binary += (char)0b10101000;
				}
			}
			else if (l.opcode == "lddr") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// lddr
					l.binary += (char)0b11101101;
					l.binary += (char)0b10111000;
				}
			}
			else if (l.opcode == "ldi") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ldi
					l.binary += (char)0b11101101;
					l.binary += (char)0b10100000;
				}
			}
			else if (l.opcode == "ldir") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ldir
					l.binary += (char)0b11101101;
					l.binary += (char)0b10110000;
				}
			}
			else if (l.opcode == "push") {
				if			(isRegisterPair(l.params[0]) &&			l.params[1] == "") {					// push rr
					l.binary += (char)(0b11000101 | (toRegisterPair(l.params[0]) << 4));
				}
			}
			else if (l.opcode == "pop") {
				if			(isRegisterPair(l.params[0]) &&			l.params[1] == "") {					// pop rr
					l.binary += (char)(0b11000001 | (toRegisterPair(l.params[0]) << 4));
				}
			}
			else if (l.opcode == "ex") {
				if			(l.params[0] == "AF" &&					l.params[1] == "AF'") {					// ex AF, AF'
					l.binary += (char)0b00001000;
				}
				else if		(l.params[0] == "DE" &&					l.params[1] == "HL") {					// ex DE, HL
					l.binary += (char)0b11101011;
				}
				else if		(l.params[0] == "(SP)" &&				l.params[1] == "HL") {					// ex (SP), HL
					l.binary += (char)0b11100011;
				}
			}
			else if (l.opcode == "exx") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// exx
					l.binary += (char)0b11011001;
				}
			}

			// =============== DEVICE I/O ===============

			else if (l.opcode == "in") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "(C)") {					// in r, (C)
					l.binary += (char)0b11101101;
					l.binary += (char)(0b01000000 | (toRegister(l.params[0]) << 3));
				}
				else if (l.params[0] == "A" &&						is8Hex(l.params[1])) {					// in A, (n)
					l.binary += (char)0b11011011;
					uint8_t devAddr = std::strtol(l.params[1].c_str(), 0, 16);
					l.binary += devAddr;
				}
			}
			else if (l.opcode == "ind") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ind
					l.binary += (char)0b11101101;
					l.binary += (char)0b10101010;
				}
			}
			else if (l.opcode == "indr") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// indr
					l.binary += (char)0b11101101;
					l.binary += (char)0b10111010;
				}
			}
			else if (l.opcode == "ini") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ini
					l.binary += (char)0b11101101;
					l.binary += (char)0b10100010;
				}
			}
			else if (l.opcode == "inir") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// indr
					l.binary += (char)0b11101101;
					l.binary += (char)0b10110010;
				}
			}
			else if (l.opcode == "out") {
				if			(l.params[1] == "(C)" &&				isRegister(l.params[1])) {				// out (C), r
					l.binary += (char)0b11101101;
					l.binary += (char)(0b01000001 | (toRegister(l.params[1]) << 3));
				}
				else if		(is8Hex(l.params[0]) &&					l.params[1] == "A") {					// out (n), A
					l.binary += (char)0b11010011;
					uint8_t devAddr = std::strtol(l.params[0].c_str(), 0, 16);
					l.binary += devAddr;
				}
			}
			else if (l.opcode == "outd") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// outd
					l.binary += (char)0b11101101;
					l.binary += (char)0b10101011;
				}
			}
			else if (l.opcode == "outi") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// outi
					l.binary += (char)0b11101101;
					l.binary += (char)0b10100011;
				}
			}
			else if (l.opcode == "otir") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// otir
					l.binary += (char)0b11101101;
					l.binary += (char)0b10110011;
				}
			}
			else if (l.opcode == "otdr") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// otdr
					l.binary += (char)0b11101101;
					l.binary += (char)0b10111011;
				}
			}

			// =============== ARITHMETIC ===============

			else if (l.opcode == "inc") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// inc r
					l.binary += (char)(0b00000100 | toRegister(l.params[0]) << 3);
				} else if	(isRegisterPair(l.params[0]) &&			l.params[1] == "") {					// inc rr
					l.binary += (char)(0b00000011 | toRegisterPair(l.params[0]) << 4);
				} else if	(l.params[0] == "(HL)" &&				l.params[1] == "") {					// inc (HL)
					l.binary += (char)0b00110100;
				}
			}
			else if (l.opcode == "dec") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// dec r
					l.binary += (char)(0b00000101 | toRegister(l.params[0]) << 3);
				} else if	(isRegisterPair(l.params[0]) &&			l.params[1] == "") {					// dec rr
					l.binary += (char)(0b00001011 | toRegisterPair(l.params[0]) << 4);
				} else if	(l.params[0] == "(HL)" &&				l.params[1] == "") {					// dec (HL)
					l.binary += (char)0b00110101;
				}
			}
			else if (l.opcode == "add") {
				if			(l.params[0] == "A" &&					l.params[1] == "(HL)") {				// add A, (HL)
					l.binary += (char)0b10000110;
				}
				else if		(l.params[0] == "A" &&					is8DecimalNum(l.params[1])) {			// add A, n
					l.binary += (char)0b11000110;
					uint8_t n = std::strtol(l.params[1].c_str(), 0, 10);
					l.binary += n;
				}
				else if		(l.params[0] == "A" &&					isRegister(l.params[1])) {				// add A, r
					l.binary += (char)0b10000000 | toRegister(l.params[1]);
				}
				else if		(l.params[0] == "HL" &&					isRegisterPair(l.params[1])) {			// add HL, rr
					l.binary += (char)(0b00001001 | (toRegisterPair(l.params[1]) << 4));
				}
			}
			else if (l.opcode == "adc") {
				if			(l.params[0] == "A" &&					isRegister(l.params[1])) {				// adc A, r
					l.binary += (char)0b10001000 | toRegister(l.params[1]);
				}
				else if		(l.params[0] == "A" &&					is8DecimalNum(l.params[1])) {			// adc A, n
					l.binary += (char)0b11001110;
					uint8_t n = std::strtol(l.params[1].c_str(), 0, 10);
					l.binary += n;
				}
				else if		(l.params[0] == "A" &&					l.params[1] == "(HL)") {				// adc A, (HL)
					l.binary += (char)0b10001110;
				}
				else if		(l.params[0] == "HL" &&					isRegisterPair(l.params[1])) {			// adc HL, rr
					l.binary += (char)0b11101101;
					l.binary += (char)(0b01001010 | (toRegisterPair(l.params[1]) << 4));
				}
			}
			else if (l.opcode == "sub") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// sub r
					l.binary += (char)0b10010000 | toRegister(l.params[0]);
				}
				else if		(is8DecimalNum(l.params[0]) &&			l.params[1] == "") {					// sub n
					l.binary += (char)0b11010110;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += n;
				}
				else if		(l.params[0] == "(HL)" &&				l.params[1] == "") {					// sub (HL)
					l.binary += (char)0b10010110;
				}
			}
			else if (l.opcode == "sbc") {
				if			(l.params[0] == "A" &&					isRegister(l.params[1])) {				// sbc A, r
					l.binary += (char)0b10011000 | toRegister(l.params[1]);
				}
				else if		(l.params[0] == "A" &&					is8DecimalNum(l.params[1])) {			// sbc A, n
					l.binary += (char)0b11011110;
					uint8_t n = std::strtol(l.params[1].c_str(), 0, 10);
					l.binary += n;
				}
				else if		(l.params[0] == "A" &&					l.params[1] == "(HL)") {				// sbc A, (HL)
					l.binary += (char)0b10011110;
				}
				else if		(l.params[0] == "HL" &&					isRegisterPair(l.params[1])) {			// sbc HL, rr
					l.binary += (char)0b11101101;
					l.binary += (char)(0b01000010 | (toRegisterPair(l.params[1]) << 4));
				}
			}
			else if (l.opcode == "daa") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// daa
					l.binary += (char)0b00100111;
				}
			}
			else if (l.opcode == "neg") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// neg
					l.binary += (char)0b11101101;
					l.binary += (char)0b01000100;
				}
			}

			// =============== LOGIC ===============

			else if (l.opcode == "and") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// and r
					l.binary += (char)0b10100000 | toRegister(l.params[0]);
				} else if	(isRegister(l.params[0]) &&				l.params[1] == "") {					// and n
					l.binary += (char)0b11100110;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += n;
				} else if	(l.params[0] == "(HL)" &&				l.params[1] == "") {					// and (HL)
					l.binary += (char)0b10100110;
				}
			}
			else if (l.opcode == "or") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// or r
					l.binary += (char)0b10110000 | toRegister(l.params[0]);
				} else if	(isRegister(l.params[0]) &&				l.params[1] == "") {					// or n
					l.binary += (char)0b11110110;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += n;
				} else if	(l.params[0] == "(HL)" &&				l.params[1] == "") {					// or (HL)
					l.binary += (char)0b10110110;
				}
			}
			else if (l.opcode == "xor") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// xor r
					l.binary += (char)0b10101000 | toRegister(l.params[0]);
				} else if	(isRegister(l.params[0]) &&				l.params[1] == "") {					// xor n
					l.binary += (char)0b11101110;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += n;
				} else if	(l.params[0] == "(HL)" &&				l.params[1] == "") {					// xor (HL)
					l.binary += (char)0b10101110;
				}
			}
			else if (l.opcode == "cpl") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// cpl
					l.binary += (char)0b00101111;
				}
			}

			// =============== FLAGS ===============

			else if (l.opcode == "scf") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// scf
					l.binary += (char)0b00110111;
				}
			}
			else if (l.opcode == "ccf") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ccf
					l.binary += (char)0b00111111;
				}
			}

			// =============== BIT SET/RESET/TEST ===============

			else if (l.opcode == "set") {
				if			(is3DecimalNum(l.params[0]) &&			isRegister(l.params[1])) {				// set n, r
					l.binary += (char)0b11001011;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)((0b11000000 | n << 3) | toRegister(l.params[1]));
				}
				else if		(is3DecimalNum(l.params[0]) &&			 l.params[1] == "(HL)") {				// set n, (HL)
					l.binary += (char)0b11001011;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)(0b11000110 | n << 3);
				}
			}
			else if (l.opcode == "res") {
				if			(is3DecimalNum(l.params[0]) &&			isRegister(l.params[1])) {				// res n, r
					l.binary += (char)0b11001011;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)((0b10000000 | n << 3) | toRegister(l.params[1]));
				}
				else if		(is3DecimalNum(l.params[0]) &&			 l.params[1] == "(HL)") {				// res n, (HL)
					l.binary += (char)0b11001011;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)(0b10000100 | n << 3);
				}
			}
			else if (l.opcode == "bit") {
				if			(is3DecimalNum(l.params[0]) &&			 l.params[1] == "(HL)") {				// bit n, (HL)
					l.binary += (char)0b11001011;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)(0b01000110 | n << 3);
				}
				else if		(is3DecimalNum(l.params[0]) &&			isRegister(l.params[1])) {				// bit n, r
					l.binary += (char)0b11001011;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)((0b01000000 | n << 3) | toRegister(l.params[1]));
				}
				
			}

			// =============== ROTATE/SHIFT ===============

			// rotate
			else if	(l.opcode == "rl") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// rl r
					l.binary += (char)0b11001011;
					l.binary += (char)(0b00010000 | toRegister(l.params[0]));
				}
				else if		(l.params[0] == "(HL)" &&				l.params[1] == "") {					// rl (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00010110;
				}
			}
			else if (l.opcode == "rla") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// rla
					l.binary += (char)0b00010111;
				}
			}
			else if (l.opcode == "rlca") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// rlca
					l.binary += (char)0b00000111;
				}
			}
			else if (l.opcode == "rlc") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// rlc r
					l.binary += (char)0b11001011;
					l.binary += (char)(0b00000000 | toRegister(l.params[0]));
				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// rlc (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00000110;
				}
			}
			else if (l.opcode == "rld") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// rld
					l.binary += (char)0b11101101;
					l.binary += (char)0b01101111;
				}
			}
			else if	(l.opcode == "rr") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// rr r
					l.binary += (char)0b11001011;
					l.binary += (char)(0b00011000 | toRegister(l.params[0]));
				}
				else if		(l.params[0] == "(HL)" &&				l.params[1] == "") {					// rr (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00011110;
				}
			}
			else if (l.opcode == "rra") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// rra
					l.binary += (char)0b00011111;
				}
			}
			else if (l.opcode == "rrc") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// rrc r
					l.binary += (char)0b11001011;
					l.binary += (char)0b00001000 | toRegister(l.params[0]);
				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// rrc (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00001110;
				}
			}
			else if (l.opcode == "rrca") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// rrca
					l.binary += (char)0b00001111;
				}
			}
			else if (l.opcode == "rrd") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// rrd
					l.binary += (char)0b11101101;
					l.binary += (char)0b01100111;
				}
			}

			// shift
			else if (l.opcode == "sla") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// sla r
					l.binary += (char)0b11001011;
					l.binary += (char)0b00100000 | toRegister(l.params[0]);
				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// sla (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00100110;
				}
			}
			else if (l.opcode == "sra") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// sra r
					l.binary += (char)0b11001011;
					l.binary += (char)0b00101000 | toRegister(l.params[0]);
				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// sra (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00101110;
				}
			}
			else if (l.opcode == "srl") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// srl r
					l.binary += (char)0b11001011;
					l.binary += (char)0b00111000 | toRegister(l.params[0]);
				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// srl  (HL)
					l.binary += (char)0b11001011;
					l.binary += (char)0b00111110;
				}
			}

			// =============== CONTROL ===============

			else if (l.opcode == "cp") {
				if			(isRegister(l.params[0]) &&				l.params[1] == "") {					// cp r
					l.binary += (char)0b10111000 | toRegister(l.params[0]);
				}
				else if (is8DecimalNum(l.params[0]) &&				l.params[1] == "") {					// cp n
					l.binary += (char)0b11111110;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 10);
					l.binary += (char)n;

				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// cp (HL)
					l.binary += (char)0b10111110;
				}
			}
			else if (l.opcode == "cpd") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// cpd
					l.binary += (char)0b11101101;
					l.binary += (char)0b10101001;
				}
			}
			else if (l.opcode == "cpdr") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// cpdr
					l.binary += (char)0b11101101;
					l.binary += (char)0b10111001;
				}
			}
			else if (l.opcode == "cpi") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// cpi
					l.binary += (char)0b11101101;
					l.binary += (char)0b10100001;
				}
			}
			else if (l.opcode == "cpir") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// cpir
					l.binary += (char)0b11101101;
					l.binary += (char)0b10110001;
				}
			}
			else if (l.opcode == "jp") {
				if			(isCond(l.params[0]) &&					isLabelorAddr(l.params[1], &symbolTable)) {	// jp cc, (nn)
					l.binary += (char)0b11000010 | (toCond(l.params[0]) << 3);
					uint16_t labelAddr = getLabelAddress(l.params[1], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if (isLabelorAddr(l.params[0], &symbolTable) &&		l.params[1] == "") {					// jp (nn)
					l.binary += (char)0b11000011;
					uint16_t labelAddr = getLabelAddress(l.params[0], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if (l.params[0] == "(HL)" &&					l.params[1] == "") {					// jp (HL)
					l.binary += (char)0b11101001;
				}
			}
			else if (l.opcode == "jr") {
				if			(isCond(l.params[0]) &&					is8Hex(l.params[1])) {					// jr cc, n
					l.binary += (char)0b001000000 | (toCond(l.params[0]) << 3);
					uint8_t n = std::strtol(l.params[1].c_str(), 0, 16);
					l.binary += (char)n;
				}
				else if		(is8Hex(l.params[0]) &&					l.params[1] == "") {					// jr n
					l.binary += (char)0b00011000;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 16);
					l.binary += (char)n;
				}
			}
			else if (l.opcode == "call") {
				if			(isCond(l.params[0]) &&					isLabelorAddr(l.params[1], &symbolTable)) {	// call cc, (nn)
					l.binary += (char)0b11000100 | (toCond(l.params[0]) << 3);
					uint16_t labelAddr = getLabelAddress(l.params[1], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
				else if		(isLabelorAddr(l.params[0], &symbolTable) &&	l.params[1] == "") {					// call (nn)
					l.binary += (char)0b11001101;
					uint16_t labelAddr = getLabelAddress(l.params[0], &symbolTable);
					l.binary += (char)(labelAddr & 0xff);
					l.binary += (char)((labelAddr >> 8) & 0xff);
				}
			}
			else if (l.opcode == "ret") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ret
					l.binary += (char)0b11001001;
				}
				else if		(isCond(l.params[0]) &&					l.params[1] == "") {					// ret cc
					l.binary += (char)(0b11000000 | (toCond(l.params[0]) << 3));
				}
			}
			else if (l.opcode == "reti") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// reti
					l.binary += (char)0b11101101;
					l.binary += (char)0b01001101;
				}
			}
			else if (l.opcode == "retn") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// retn
					l.binary += (char)0b11101101;
					l.binary += (char)0b01000101;
				}
			}
			else if (l.opcode == "halt") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// halt
					l.binary += (char)0b01110110;
				}
			}
			else if (l.opcode == "nop") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// nop
					l.binary += (char)0b01110110;
				}
			}
			else if (l.opcode == "rst") {
				if			(is8Hex(l.params[0]) &&					l.params[1] == "") {					// rst (n)
					l.binary += (char)(0b11000111 | (toRstAddr(l.params[0]) << 3));
				}
			}
			else if (l.opcode == "djnz") {
				if			(is8Hex(l.params[0]) &&					l.params[1] == "") {					// djnz n
					l.binary += (char)0b00010000;
					uint8_t n = std::strtol(l.params[0].c_str(), 0, 16);
					l.binary += (char)n;
				}
			}

			// =============== INTERRUPTS ===============

			else if (l.opcode == "di") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// di
					l.binary += (char)0b11110011;
				}
			}
			else if (l.opcode == "ei") {
				if			(l.params[0] == "" &&					l.params[1] == "") {					// ei
					l.binary += (char)0b11111011;
				}
			}
			else if (l.opcode == "im") {
				if			(l.params[0] == "0" &&					l.params[1] == "") {					// im 0
					l.binary += (char)0b11101101;
					l.binary += (char)0b01000110;
				}
				else if		(l.params[0] == "1" &&					l.params[1] == "") {					// im 1
					l.binary += (char)0b11101101;
					l.binary += (char)0b01010110;
				}
				else if		(l.params[0] == "2" &&					l.params[1] == "") {					// im 2
					l.binary += (char)0b11101101;
					l.binary += (char)0b01011110;
				}
			}
		}
	}

	std::string machineCode = "";
	for (struct symbol& s : symbolTable) {
		for (struct instruction& i : s.def) {
			machineCode += i.binary;
		}
	}

	uint16_t outputSize = machineCode.length();

	uint8_t *output = (uint8_t*)malloc(0xffff);
	output = (uint8_t*)machineCode.c_str();



	std::cout << std::endl << "Program listing:" << std::endl;
	std::cout << "\tADDRESS\tMACHINE LANG\tLABEL\tINSTRUC\tPARAMS" << std::endl;

	uint16_t instAddr = org;
	
	for (struct symbol& s : symbolTable) {
		int k = 0;
		for (struct instruction& i : s.def) {
			printf("\t%04x\t", instAddr);

			for (char& c : i.binary) printf("%02x ", (uint8_t)c);

			if (i.size < 3) std::cout << "\t";

			std::cout << "\t";
			if (k == 0) std::cout << s.label << "\t"; else std::cout << "\t";
			std::cout << i.opcode << "\t";

			if (s.type == 1) {
				if ((i.params[0] != "") || (i.params[1] != "")) {
					int t = 0;
					for (std::string& p : i.params) {
						if (p != "") {
							if (t != 0) std::cout << ", ";
							std::cout << p;
						}
						t++;
					}
				}
			}
			else {
				int t = 0;
				for (std::string& p : i.params) {
					if (t != 0) std::cout << ", ";
					std::cout << p;
					t++;
				}
			}

			instAddr += i.size;

			std::cout << std::endl;

			k++;
		}
	}


	std::cout << std::endl << "Binary listing:" << std::endl;
	listByteA(output, machineCode.length(), org);

	uint16_t entryPoint = 0xffff;
	for (symbol& s : symbolTable) if (s.label == "_main") entryPoint = s.address;

	if (entryPoint != 0xffff)
		printf("Entry point: %04x\n", entryPoint);
	else
		std::cout << "Error: Entry point _main symbol not defined. Define symbol _main" << std::endl;
	
	// Save hex
	struct ihex_state ihex;
	ihex_init(&ihex);
	ihex_write_at_address(&ihex, org);
	ihex_write_bytes(&ihex, output, outputSize);
	ihex_end_write(&ihex);

	std::ofstream outfile(outputPath, std::ios::binary);
	outfile << intelhex;
	outfile.close();

	return 0;
}
