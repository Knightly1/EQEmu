
//grep ^#define eq_packet_structs.h | tr '\t' ' ' | cut '-d ' -f1-2|sed 's/#define/#undef/g'

#ifndef KEEP_DEFINES
//clear macros defined in eq_packet_structs
#undef BUFF_COUNT
#undef ADVENTURE_COLLECT
#undef ADVENTURE_MASSKILL
#undef ADVENTURE_NAMED
#undef ADVENTURE_RESCUE
#undef MAX_PP_DISCIPLINES
#undef MAX_PLAYER_TRIBUTES
#undef MAX_PLAYER_BANDOLIER
#undef MAX_PLAYER_BANDOLIER_ITEMS
#undef TRIBUTE_NONE
#undef MAX_LEADERSHIP_AA_ARRAY
#undef MAX_PP_LANGUAGE
#undef MAX_PP_SPELLBOOK
#undef MAX_PP_MEMSPELL
#undef MAX_PP_SKILL
#undef MAX_PP_AA_ARRAY
#undef COINTYPE_PP
#undef COINTYPE_GP
#undef COINTYPE_SP
#undef COINTYPE_CP
#undef MAX_NUMBER_GUILDS
#undef MAX_TRIBUTE_TIERS
#endif //KEEP_DEFINES

