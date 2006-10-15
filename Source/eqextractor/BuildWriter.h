/*

EQ Extractor, by Father Nitwit 2005


*/
//no macro guard on purpose

class PatchBuildFileWriter 
  : public ::PatchBuildFileWriterInterface {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	virtual void WritePacket(EmuOpcode emu_op, uint32 packlen, unsigned char *packet);
};




