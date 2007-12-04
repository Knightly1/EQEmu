


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
RULE_INT ( Character, AutosaveIntervalS, 300 )	//0=disabled
RULE_INT ( Character, HPRegenMultiplier, 100)
RULE_INT ( Character, ManaRegenMultiplier, 100)
RULE_INT ( Character, EnduranceRegenMultiplier, 100)
RULE_INT ( Character, ConsumptionMultiplier, 100) //item's hunger restored = this value * item's food level, 100 = normal, 50 = people eat 2x as fast, 200 = people eat 2x as slow
RULE_CATEGORY_END()

RULE_CATEGORY( Guild )
RULE_INT ( Guild, MaxMembers, 2048 )
RULE_CATEGORY_END()

RULE_CATEGORY( Skills )
RULE_INT ( Skills, MaxTrainTradeskills, 21 )
RULE_CATEGORY_END()

RULE_CATEGORY( Pets )
RULE_REAL( Pets, AttackCommandRange, 150 )
RULE_CATEGORY_END()

RULE_CATEGORY( GM )
RULE_INT ( GM, MinStatusToZoneAnywhere, 250 )
RULE_CATEGORY_END()

RULE_CATEGORY( World )
RULE_INT ( World, ZoneAutobootTimeoutMS, 60000 )
RULE_INT ( World, ClientKeepaliveTimeoutMS, 65000 )
RULE_CATEGORY_END()

RULE_CATEGORY( Zone )
RULE_INT ( Zone,  NPCGlobalPositionUpdateInterval, 60000 ) //ms between intervals of sending a position update to the entire zone.
RULE_INT ( Zone,  ClientLinkdeadMS, 180000) //the time a client remains link dead on the server after a sudden disconnection
RULE_INT ( Zone,  GraveyardTimeMS, 1200000) //ms time until a player corpse is moved to a zone's graveyard, if one is specified for the zone
RULE_BOOL ( Zone, EnableShadowrest, 0 ) // enables or disables the shadowrest zone feature for player corpses. Default is turned off.
RULE_CATEGORY_END()

RULE_CATEGORY( Map )
//enable these to help prevent mob hopping when they are pathing
RULE_BOOL ( Map, FixPathingZWhenLoading, true )		//increases zone boot times a bit to reduce hopping.
RULE_BOOL ( Map, FixPathingZAtWaypoints, false )	//alternative to `WhenLoading`, accomplishes the same thing but does it at each waypoint instead of once at boot time.
RULE_BOOL ( Map, FixPathingZWhenMoving, false )		//very CPU intensive, but helps hopping with widely spaced waypoints.
RULE_BOOL ( Map, FixPathingZOnSendTo, false )		//try to repair Z coords in the SendTo routine as well.
RULE_REAL ( Map, FixPathingZMaxDeltaMoving, 20 )	//at runtime while pathing: max change in Z to allow the BestZ code to apply.
RULE_REAL ( Map, FixPathingZMaxDeltaWaypoint, 20 )	//at runtime at each waypoint: max change in Z to allow the BestZ code to apply.
RULE_REAL ( Map, FixPathingZMaxDeltaSendTo, 20 )	//at runtime in SendTo: max change in Z to allow the BestZ code to apply.
RULE_REAL ( Map, FixPathingZMaxDeltaLoading, 45 )	//while loading each waypoint: max change in Z to allow the BestZ code to apply.
RULE_CATEGORY_END()

RULE_CATEGORY( Spells )
RULE_INT (Spells, SpellAggroModifier, 100)
RULE_INT (Spells, BardSpellAggroMod, 3)
RULE_INT (Spells, PetSpellAggroMod, 10)
RULE_INT (Spells, AutoResistDiff, 15)
RULE_REAL (Spells, ResistChance, 2.0) //chance to resist given no resists and same level
RULE_REAL (Spells, ResistMod, 0.40) //multiplier, chance to resist = this * ResistAmount
RULE_REAL (Spells, PartialHitChance, 0.7) //The chance when a spell is resisted that it will partial hit.
RULE_CATEGORY_END()

RULE_CATEGORY( Combat )
RULE_REAL ( Combat, BaseCritChance, 0.0 ) //The base crit chance for non warriors, NOTE: This will apply to NPCs as well
RULE_REAL ( Combat, WarBerBaseCritChance, 0.03 ) //The base crit chance for warriors and berserkers, only applies to clients
RULE_REAL ( Combat, BerserkBaseCritChance, 0.06 ) //The bonus base crit chance you get when you're berserk
RULE_INT ( Combat, NPCBashKickLevel, 6 ) //The level that npcs can KICK/BASH
RULE_REAL ( Combat, ClientBaseCritChance, 0.0 ) //The base crit chance for all clients, this will stack with warrior's/zerker's crit chance.
RULE_CATEGORY_END()

RULE_CATEGORY( NPC )
RULE_INT ( NPC, MinorNPCCorpseDecayTimeMS, 450000 ) //level<55
RULE_INT ( NPC, MajorNPCCorpseDecayTimeMS, 1500000 ) //level>=55
RULE_BOOL (NPC, UseItemBonusesForNonPets, true)
RULE_CATEGORY_END()

#undef RULE_CATEGORY
#undef RULE_INT
#undef RULE_REAL
#undef RULE_BOOL
#undef RULE_CATEGORY_END






