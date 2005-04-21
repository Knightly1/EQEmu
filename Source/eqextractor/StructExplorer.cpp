/*

EQ Extractor, by Father Nitwit 2005

*/
#include "StructExplorer.h"


void StructExplorer::PrintBlockReal(const char *field_name, const char *data, uint32 length) {
	uint32 quads = length / 4;
	uint32 rem = length % 4;
	uint32 *dataq = (uint32 *) data;
	uint32 pos = 0;
	char buf[64];
	bool was_zeros = false;
	bool was_ones = false;
	while(quads > 0) {
		sprintf(buf, "%s[%lu]", field_name, pos);
		if(*dataq == 0) {
			if(was_ones)
				printf("%s: ... ones\n", buf);
			was_ones = false;
			
			if(was_zeros) {
				//do nothing...
			} else {
				printf("%s: zeros\n", buf);
				was_zeros = true;
			}
		} else if(*dataq == 0xFFFFFFFF) {
			if(was_zeros)
				printf("%s: ... zeros\n", buf);
			was_zeros = false;
			
			if(was_ones) {
				//do nothing...
			} else {
				printf("%s: all ones\n", buf);
				was_ones = true;
			}
		} else {
			if(was_zeros)
				printf("%s: ... zeros\n", buf);
			if(was_ones)
				printf("%s: ... ones\n", buf);
			was_zeros = false;
			was_ones = false;
			PrintQuadrentReal(buf, *dataq);
		}
		quads--;
		dataq++;
		pos += 4;
	}
	
	
	switch(rem) {
	case 1:
		printf("%s[%lu]: (%u)\n", field_name, pos, *((uint8 *)dataq));
		pos += 1;
		break;
	case 2:
		printf("%s[%lu]: (%u %u)(%u)(%d)\n", field_name, pos, *((uint8 *)dataq), *(((uint8 *)dataq)+1), *((uint16 *)dataq), *((sint16 *)dataq));
		pos += 2;
		break;
	case 3:
		printf("%s[%lu]: (%u %u %u)(%u _)(%d _)\n", field_name, pos, *((uint8 *)dataq), *(((uint8 *)dataq)+1), *(((uint8 *)dataq)+2), *((uint16 *)dataq), *((sint16 *)dataq));
		pos += 3;
		break;
	default:
		break;
	}
}

//most of the common things 4 bytes can represent
typedef union {
	struct {	//bytes
		uint8	b1;
		uint8	b2;
		uint8	b3;
		uint8	b4;
	} bytes;
	struct {	//words
		uint16	w1;
		uint16	w2;
	} words;
	struct {	//words
		sint16	w1;
		sint16	w2;
	} signed_words;
	uint32 dword;
	sint32 signed_dword;
	float _float;
} ByteReps;
void StructExplorer::PrintQuadrentReal(const char *field_name, uint32 data) {
	ByteReps *fbr = (ByteReps *) &data;
	if(fbr->dword == 0) {
		printf("%s: zeros\n", field_name);
		return;
	}
	
	printf("%s: (%u %u %u %u) (%u %u) (%d %d) (0x%.4x 0x%.4x)\n",
	field_name,
	fbr->bytes.b1, fbr->bytes.b2, fbr->bytes.b3, fbr->bytes.b3,
	fbr->words.w1, fbr->words.w2, fbr->signed_words.w1, fbr->signed_words.w2,
	fbr->words.w1, fbr->words.w2
	);
	
	printf("%s: (%lu) (%ld) (0x%.8lx) (%.4f)\n",
	field_name,
	fbr->dword, fbr->signed_dword, fbr->dword, fbr->_float
	);
}

void StructExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
}













