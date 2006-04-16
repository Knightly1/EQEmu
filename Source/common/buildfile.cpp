//#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "packetfile.h"
#include "buildfile.h"
#include <map>


using namespace std;


BuildFileWriter::BuildFileWriter() {
	out = NULL;
}

BuildFileWriter::~BuildFileWriter() {
	CloseFile();
}
	
bool BuildFileWriter::OpenFile(const char *name) {
	CloseFile();
	
	out = fopen(name, "wb");
	if(out == NULL) {
		fprintf(stderr, "Error opening packet file '%s': %s\n", name, strerror(errno));
		return(false);
	}
	
	BuildFileHeader head;
	head.build_file_magic = BUILD_FILE_MAGIC;
	
	if(fwrite(&head, sizeof(head), 1, out) != 1) {
		fprintf(stderr, "Error writting header to packet file: %s\n", strerror(errno));
		return(false);
	}
	
	return(true);
}

void BuildFileWriter::CloseFile() {
	if(out != NULL) {
		fclose(out);
		out = NULL;
	}
}


bool BuildFileWriter::_WriteBlock(PF_SectionType pf_op, const void *d, uint16 len) {
	if(out == NULL)
		return(false);
	
	BuildFileSection s;
	s.type = pf_op;
	s.len = len;
	
	if(fwrite(&s, sizeof(s), 1, out) != 1) {
		fprintf(stderr, "Error writting block header: %s\n", strerror(errno));
		return(false);
	}
	
	if(fwrite(d, len, 1, out) != 1) {
		fprintf(stderr, "Error writting block body: %s\n", strerror(errno));
		return(false);
	}
	
	return(true);
}


BuildFileReader::BuildFileReader() {
	in = NULL;
}

BuildFileReader::~BuildFileReader() {
	CloseFile();
}
	
bool BuildFileReader::OpenFile(const char *name) {
	CloseFile();
	
	in = fopen(name, "rb");
	if(in == NULL) {
		fprintf(stderr, "Error opening build file '%s': %s\n", name, strerror(errno));
		return(false);
	}
	
	BuildFileHeader head;
	
	if(fread(&head, sizeof(head), 1, in) != 1) {
		fprintf(stderr, "Error reading header from build file: %s\n", strerror(errno));
		fclose(in);
		return(false);
	}
	
	if(head.build_file_magic != BUILD_FILE_MAGIC) {
		fclose(in);
		if(head.build_file_magic == PACKET_FILE_MAGIC) {
			fprintf(stderr, "Error: this is a packet file, not a build file, its needs processing!\n");
		} else {
			fprintf(stderr, "Error: this is not a packet file!\n");
		}
		return(false);
	}
	
	return(true);
}

void BuildFileReader::CloseFile() {
	if(in != NULL) {
		fclose(in);
		in = NULL;
		printf("Closed packet file.\n");
	}
}

bool BuildFileReader::ReadSection(PF_SectionType &bf_op, uint16 &packlen, unsigned char *packet) {
	if(in == NULL)
		return(false);
	if(feof(in))
		return(false);
	
	BuildFileSection s;
	
	if(fread(&s, sizeof(s), 1, in) != 1) {
		if(!feof(in))
			fprintf(stderr, "Error reading section header: %s\n", strerror(errno));
		return(false);
	}
	
	bf_op = (PF_SectionType) s.type;
	
	if(packlen < s.len) {
		fprintf(stderr, "Section buffer is too small! %d < %d, skipping\n", packlen, s.len);
		fseek(in, s.len, SEEK_CUR);
		return(false);
	}
	
	if(fread(packet, 1, s.len, in) != s.len) {
		if(feof(in))
			fprintf(stderr, "Error: EOF encountered when expecting packet data.\n");
		else
			fprintf(stderr, "Error reading section body: %s\n", strerror(errno));
		return(false);
	}
	
	packlen = s.len;
	
	return(true);
}







