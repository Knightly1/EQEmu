#include "client.h"
#include "StringIDs.h"

void Client::SendAAStats() {
	APPLAYER* outapp = new APPLAYER(OP_SendAAStats, sizeof(AltAdvStats_Struct));
	AltAdvStats_Struct *aps = (AltAdvStats_Struct *)outapp->pBuffer;
	aps->experience = m_pp.expAA;
	aps->experience = (int32)(((float)330.0f * (float)m_pp.expAA) / (float)max_AAXP);
	aps->unspent = m_pp.aapoints;
	aps->percentage = m_pp.perAA;
	QueuePacket(outapp);
	safe_delete(outapp);
}
void Client::BuyAA(AA_Action* action){
	SendAA_Struct* aa2 = zone->FindAA(action->ability);
	if(!aa2){
		for(int i=1;i<5;i++){
			if(aa2 = zone->FindAA(action->ability-i))
				i=4;
		}
	}
	int32 cur_level = GetAA(aa2->id);
	if(aa2 && m_pp.aapoints >= aa2->cost && cur_level < aa2->max_level){
		SetAA(aa2->id,cur_level+1);
		m_pp.aapoints -= aa2->cost;
		database.SetPlayerAlternateAdv(account_id, m_pp.name, &aa);
		SendAA(aa2->id);
		SendAATable();
		char val1[20]={0};
		char val2[20]={0};
		char val3[20]={0};
		char points[20]={0};
		char point[20]={0};
		const char* points2=ConvertArray(AA_POINTS,points);
		const char* point2=ConvertArray(AA_POINT,point);
		if(cur_level<1){
			if(aa2->cost>1)
				Message_StringID(15,AA_GAIN_ABILITY,ConvertArray(aa2->title_sid,val1),ConvertArray(aa2->cost,val2),points2);
			else
				Message_StringID(15,AA_GAIN_ABILITY,ConvertArray(aa2->title_sid,val1),ConvertArray(aa2->cost,val2),point2);
		}
		else{
			if(aa2->cost>1)
				Message_StringID(15,AA_IMPROVE,ConvertArray(aa2->title_sid,val1),ConvertArray(cur_level,val2),ConvertArray(aa2->cost,val3),points2);
			else
				Message_StringID(15,AA_IMPROVE,ConvertArray(aa2->title_sid,val1),ConvertArray(cur_level,val2),ConvertArray(aa2->cost,val3),point2);
		}
		SendAAStats();
	}
}
void Client::SendAATimer(UseAA_Struct *uaa){
	APPLAYER* outapp = new APPLAYER(OP_AAAction,sizeof(UseAA_Struct));
	UseAA_Struct* uaaout = (UseAA_Struct*)outapp->pBuffer;
	uaaout->ability=uaa->ability;
	uaaout->begin=uaa->begin;
	uaaout->end=uaa->end;
	QueuePacket(outapp);
	safe_delete(outapp);
}
void Client::ActivateAA(int activate){
	int32 timermod=0;
	switch(activate){
		case 50:
			MemorizeSpell(8,2750,3);
			CastToMob()->CastSpell(2750,this->GetID());
			timermod=7200;
			break;
		case 51:
			MemorizeSpell(8,2751,3);
			CastToMob()->CastSpell(2751,target->GetID());
			timermod=112;
			break;
		case 80:
			MemorizeSpell(8,2765,3);
			CastToMob()->CastSpell(2765,this->GetID());
			timermod=7;
			break;
	}
	time_t timestamp=time(NULL);
	
	APPLAYER* outapp = new APPLAYER(OP_AAAction,sizeof(UseAA_Struct));
	UseAA_Struct* uaa = (UseAA_Struct*)outapp->pBuffer;
	uaa->ability=activate;
	uaa->begin=timestamp;
	uaa->end=timestamp;
	database.UpdateAATimers(this->CharacterID(),timestamp+timermod,timestamp,activate);
	QueuePacket(outapp);
	safe_delete(outapp);
}
void Client::SendAATable() {
    APPLAYER* outapp = new APPLAYER(OP_RespondAA, sizeof(PlayerAA_Struct));
    PlayerAA_Struct* aa2=(PlayerAA_Struct*)outapp->pBuffer;
	for(int i=0;i<MAX_AAS;i++){
		if(aa.aa_list[i].aa_value>1)
			aa2->aa_list[i].aa_skill=aa.aa_list[i].aa_skill+aa.aa_list[i].aa_value-1;
		else
			aa2->aa_list[i].aa_skill=aa.aa_list[i].aa_skill;
		aa2->aa_list[i].aa_value=aa.aa_list[i].aa_value;
	}
	outapp->Deflate();
    QueuePacket(outapp);
    safe_delete(outapp);
}
void Client::SendAA(int32 id, int seq,bool update){
	int value=0;
	SendAA_Struct* saa2 = NULL;
	if(id==0)
		saa2=zone->GetAAList()->aa[seq];
	else
		saa2=zone->FindAA(id);
	int size=sizeof(SendAA_Struct)+sizeof(AA_Ability)*saa2->total_abilities;
	uchar* buffer = new uchar[size];
	SendAA_Struct* saa=(SendAA_Struct*)buffer;
	memcpy(saa,saa2,size);
	if(saa->spellid==0)
		saa->spellid=0xFFFFFFFF;
	if((value=GetAA(saa->id))){
		saa->id+=value;
		saa->last_id=saa->id-1;
		saa->next_id=saa->id+1;
		value++;
		saa->current_level=value;
		saa->cost2=value;
		if(saa->type==1) //general ability
			saa->abilities[0].increase_amt*=value;
	}
	APPLAYER* outapp = new APPLAYER(OP_SendAATable);
	outapp->size=size;
	outapp->pBuffer=(uchar*)saa;
	QueuePacket(outapp);
	safe_delete(outapp);
}
void Client::SendAAList(){
	for(int i=0;i<zone->GetTotalAAs();i++){
		SendAA(0,i);
	}
}
int32 Client::GetAA(int32 aa_id){
	if(!aa.aa_list)
		return 0;
	for(int i=0;i<MAX_AAS;i++){
		if(aa.aa_list[i].aa_skill==aa_id)
			return aa.aa_list[i].aa_value;
		else if(aa.aa_list[i].aa_skill==0)
			i=MAX_AAS;
	}
	return 0;
}
bool Client::SetAA(int32 aa_id, int32 new_value){
	if(!aa.aa_list)
		return false;
	for(int cur=0;cur<MAX_AAS;cur++){
		if(aa.aa_list[cur].aa_skill==aa_id){
			aa.aa_list[cur].aa_value=new_value;
			return true;
		}
		else if(aa.aa_list[cur].aa_skill==0){ //end of list
			aa.aa_list[cur].aa_skill=aa_id;
			aa.aa_list[cur].aa_value=new_value;
			return true;
		}
	}
	return false;
}
SendAA_Struct* Zone::FindAA(int32 id){
	SendAA_Struct* ret=NULL;
	for(int i=0;i<totalAAs;i++){
		if(aas->aa[i]->id==id){
			ret=aas->aa[i];
			i=totalAAs;
		}
	}
	return ret;
}
void Zone::LoadAAs(){
	int32 size=database.GetSizeAA();
	if(size>=sizeof(SendAA_Struct)){
		aa_buffer = new uchar[size];
		aas=(AA_List*)aa_buffer;
		database.LoadAAs(aas);
		totalAAs=database.CountAAs();
	}
	else{
		LogFile->write(EQEMuLog::Error, "Failed to load AAs!");
		aas=NULL;
	}
}