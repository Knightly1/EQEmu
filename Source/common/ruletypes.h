


#ifndef RULE_CATEGORY
#define RULE_CATEGORY(name)
#endif
#ifndef RULE_INT
#define RULE_INT(cat, rule, default_value)
#endif
#ifndef RULE_REAL
#define RULE_REAL(cat, rule, default_value)
#endif
#ifndef RULE_BOOL
#define RULE_BOOL(cat, rule, default_value)
#endif
#ifndef RULE_CATEGORY_END
#define RULE_CATEGORY_END()
#endif




RULE_CATEGORY( Character )
RULE_INT ( Character, MaxLevel, 65 )
RULE_INT ( Character, DeathExpLossLevel, 6 )
RULE_INT ( Character, CorpseDecayTimeMS, 10800000 )
RULE_BOOL( Character, LeaveCorpses, false )
RULE_BOOL( Character, LeaveNakedCorpses, false )
RULE_REAL( Character, ExpMultiplier, 1.0 )
RULE_CATEGORY_END()

RULE_CATEGORY( Guild )
RULE_INT ( Guild, MaxMembers, 2048 )

RULE_CATEGORY( Skills )
RULE_INT ( Skills, MaxTrainTradeskills, 21 )

RULE_CATEGORY( Pets )
RULE_REAL( Pets, AttackCommandRange, 150 )

RULE_CATEGORY( GM )
RULE_INT ( GM, MinStatusToZoneAnywhere, 250 )

#undef RULE_CATEGORY
#undef RULE_INT
#undef RULE_REAL
#undef RULE_BOOL
#undef RULE_CATEGORY_END






