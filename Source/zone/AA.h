
#ifndef AA_H
#define AA_H

#include "../common/eq_packet_structs.h"

#define MANA_BURN 664


#include <map>
using namespace std;


#define MAX_SWARM_PETS 12	//this can change as long as you make more coords (swarm_pet_x/swarm_pet_y)

//this might be missing some, and some might not be used...
typedef enum {	//AA Targeting Constants
	aaTargetUser = 1,
	aaTargetCurrent = 2,		//use current target
	aaTargetGroup = 3,			//target group of user
	aaTargetCurrentGroup = 4,	//target group of current target
	aaTargetPet = 5				//target the user's pet
} aaTargetType;


typedef enum {
	aaActionNone 				= 0,
	aaActionAETaunt 			= 1,
	aaActionMassBuff 			= 2,
	aaActionFlamingArrows 		= 3,
	aaActionFrostArrows			= 4,
	aaActionRampage				= 5,
	aaActionSharedHealth		= 6,
	aaActionCelestialRegen		= 7,
	aaActionDireCharm			= 8,
	aaActionImprovedFamiliar	= 9,
	aaActionActOfValor			= 10,
	aaActionSuspendedMinion		= 11,
	aaActionEscape				= 12,
	aaActionBeastialAlignment	= 13
} aaNonspellAction;

//use these for AAs which dont cast spells, yet need effects
//if this list grows beyond 32, more work is needed in *AAEffect
typedef enum {	//AA Effect IDs
	aaEffectMassGroupBuff = 1,
	aaEffectRampage,
	aaEffectSharedHealth,
	aaEffectFlamingArrows,
	aaEffectFrostArrows
} aaEffectType;

typedef enum {	//AA IDs
	aaUnknown0 = 0,
	aaInnateStrength = 2,			//works
	aaInnateStamina = 7,			//works
	aaInnateAgility = 12,			//works
	aaInnateDexterity = 17,			//works
	aaInnateIntelligence = 22,		//works
	aaInnateWisdom = 27,			//works
	aaInnateCharisma = 32,			//works
	aaInnateFireProtection = 37,		//works
	aaInnateColdProtection = 42,		//works
	aaInnateMagicProtection = 47,		//works
	aaInnatePoisonProtection = 52,		//works
	aaInnateDiseaseProtection = 57,		//works
	aaInnateRunSpeed = 62,			//works
	aaInnateRegeneration = 65,		//works
	aaInnateMetabolism = 68,		
	aaInnateLungCapacity = 71,		
	aaFirstAid = 74,			//untested
	aaHealingAdept = 77,			//untested
	aaHealingGift = 80,			//untested
	aaSpellCastingMastery = 83,		//untested
	aaSpellCastingReinforcement = 86,	//untested
	aaMentalClarity = 89,			
	aaSpellCastingFury = 92,		//untested
	aaChanellingFocus = 95,			
	aaSpellCastingSubtlety = 98,		//untested
	aaSpellCastingExpertise = 101,		//untested
	aaSpellCastingDeftness = 104,		//untested
	aaNaturalDurability = 107,		//works
	aaNaturalHealing = 110,			//untested
	aaCombatFury = 113,			//untested
	aaFearResistance = 116,			//untested
	aaFinishingBlow = 119,			//untested
	aaCombatStability = 122,
	aaCombatAgility = 125,
	aaMassGroupBuff = 128,			//untested
	aaDivineResurrection = 129,		//DB
	aaInnateInvisToUndead = 130,		//DB
	aaCelestialRegeneration = 131,		//untested
	aaBestowDivineAura = 132,		//DB
	aaTurnUndead = 133,			//DB
	aaPurifySoul = 136,			//DB
	aaQuickEvacuation = 137,			//untested
	aaExodus = 140,				//untested
	aaQuickDamage = 141,			//untested
	aaEnhancedRoot = 144,			
	aaDireCharm = 145,			//untested
	aaCannibalization = 146,			//DB
	aaQuickBuff = 147,			//untested
	aaAlchemyMastery = 150,			
	aaRabidBear = 153,			//DB
	aaManaBurn = 154,			//DB
	aaImprovedFamiliar = 155,		//untested, implemented?
	aaNexusGate = 156,			//DB
	aaUnknown54 = 157,			
	aaPermanentIllusion = 158,		
	aaJewelCraftMastery = 159,		
	aaGatherMana = 162,			//DB
	aaMendCompanion = 163,			//DB
	aaQuickSummoning = 164,			//untested
	aaFrenziedBurnout = 167,			//DB
	aaElementalFormFire = 168,		//DB
	aaElementalFormWater = 171,		//DB
	aaElementalFormEarth = 174,		//DB
	aaElementalFormAir = 177,		//DB
	aaImprovedReclaimEnergy = 180,		//untested
	aaTurnSummoned = 181,			//DB
	aaElementalPact = 182,			//DB
	aaLifeBurn = 183,			//DB
	aaDeadMesmerization = 184,		//DB
	aaFearstorm = 185,			//DB
	aaFleshToBone = 186,			//DB
	aaCallToCorpse = 187,			//DB
	aaDivineStun = 188,			//DB
	aaImprovedLayOfHands = 189,		
	aaSlayUndead = 190,			
	aaActOfValor = 193,			//DB
	aaHolySteed = 194,			//DB
	aaFearless = 195,			
	aa2HandBash = 196,			
	aaInnateCamouflage = 197,		//DB
	aaAmbidexterity = 198,			//untested
	aaArcheryMastery = 199,			//untested
	aaFletchingMastery = 202,		
	aaEndlessQuiver = 205,			//untested
	aaUnholySteed = 206,			//DB
	aaImprovedHarmTouch = 207,		//untested
	aaLeechTouch = 208,			//DB
	aaDeathPeace = 209,			
	aaSoulAbrasion = 210,			//untested
	aaInstrumentMastery = 213,		//untested
	aaUnknown91 = 216,			
	aaUnknown92 = 219,			
	aaUnknown93 = 222,			
	aaJamFest = 225,				
	aaUnknown95 = 228,
	aaSonicCall = 229,			
	aaCriticalMend = 230,			//untested
	aaPurifyBody = 233,			//DB
	aaChainCombo = 234,			
	aaRapidFeign = 237,			//untested
	aaReturnKick = 240,			
	aaEscape = 243,				//DB
	aaPoisonMastery = 244,			
	aaDoubleRiposte = 247,			//untested
	aaQuickHide = 250,			
	aaQuickThrow = 251,			
	aaPurgePoison = 254,			//DB
	aaFlurry = 255,				//untested
	aaRampage = 258,			//untested
	aaAreaTaunt = 259,			//untested
	aaWarcry = 260,				//DB
	aaBandageWound = 263,			//untested
	aaSpellCastingReinforcementMastery = 266,	//untested
	aaSpellCastingFuryMastery = 267,	//untested
	aaExtendedNotes = 270,			//untested
//--- stopping point
	aaDragonPunch = 273,			
	aaStrongRoot = 274,			//DB
	aaSingingMastery = 275,			
	aaBodyAndMindRejuvenation = 278,	
	aaPhysicalEnhancement = 279,		//untested
	aaAdvTrapNegotiation = 280,		
	aaAcrobatics = 283,				//untested
	aaScribbleNotes = 286,			
	aaChaoticStab = 287,			
	aaPetDiscipline = 288,			
	aaHobbleofSpirits = 289,		//DB
	aaFrenzyofSpirit = 290,			//DB
	aaParagonofSpirit = 291,		//DB
	aaAdvancedInnateStrength = 292,
	aaAdvancedInnateStamina = 302,
	aaAdvancedInnateAgility = 312,
	aaAdvancedInnateDexterity = 322,
	aaAdvancedInnateIntelligence = 332,
	aaAdvancedInnateWisdom = 342,
	aaAdvancedInnateCharisma = 352,
	aaWardingofSolusek = 362,
	aaBlessingofEci = 372,
	aaMarrsProtection = 382,
	aaShroudofTheFaceless = 392,
	aaBertoxxulousGift = 402,
	aaNewTanaanCraftingMastery = 412,
	aaPlanarPower = 418,			//untested
	aaPlanarDurability = 423,
	aaInnateEnlightenment = 426,
	aaAdvancedSpellCastingMastery = 431,
	aaAdvancedHealingAdept = 434,		//untested
	aaAdvancedHealingGift = 437,		//untested
	aaCoupdeGrace = 440,
	aaFuryoftheAges = 443,
	aaMasteryofthePast = 446,
	aaLightningReflexes = 449,
	aaInnateDefense = 454,
	aaRadiantCure = 459,			//DB
	aaHastenedDivinity = 462,			//DB
	aaHastenedTurning = 465,			//DB
	aaHastenedPurificationofSoul = 468,	//DB
	aaHastenedGathering = 471,			//DB
	aaHastenedRabidity = 474,			//DB
	aaHastenedExodus = 477,				//DB
	aaHastenedRoot = 480,
	aaHastenedMending = 483,			//DB
	aaHastenedBanishment = 486,			
	aaHastenedInstigation = 489,		//DB, maybe
	aaFuriousRampage = 492,
	aaHastenedPurificationoftheBody = 495,	//DB
	aaHastyExit = 498,
	aaHastenedPurification = 501,		//DB
	aaFlashofSteel = 504,
	aaDivineArbitration = 507,		//DB
	aaWrathoftheWild = 510,			//DB
	aaVirulentParalysis = 513,		//DB
	aaHarvestofDruzzil = 516,		//DB
	aaEldritchRune = 517,			//DB
	aaServantofRo = 520,			//DB
	aaWaketheDead = 523,			//DB
	aaSuspendedMinion = 526,		//untested
	aaSpiritCall = 528,			//DB
	aaCelestialRenewal = 531,		
	aaAllegiantFamiliar = 533,		
	aaHandofPiety = 534,			//DB
	aaMithanielsBinding = 537,		//untested
	aaMendingoftheTranquil = 539,		
	aaRagingFlurry = 542,			
	aaGuardianoftheForest = 545,		
	aaSpiritoftheWood = 548,		//DB
	aaBestialFrenzy = 551,			//untested
	aaHarmoniousAttack = 556,		//untested
	aaKnightsAdvantage = 561,		
	aaFerocity = 564,			
	aaViscidRoots = 567,			
	aaSionachiesCrescendo = 568,		//untested
	aaAyonaesTutelage = 571,		
	aaFeignedMinion = 574,			
	aaUnfailingDivinity = 577,		
	aaAnimationEmpathy = 580,		
	aaRushtoJudgement = 583,		
	aaLivingShield = 586,			
	aaConsumptionoftheSoul = 589,		//untested
	aaBoastfulBellow = 592,			//DB
	aaFervrentBlessing = 593,		//untested
	aaTouchoftheWicked = 596,		//untested
	aaPunishingBlade = 599,			
	aaSpeedoftheKnight = 602,		
	aaShroudofStealth = 605,		
	aaNimbleEvasion = 606,			
	aaTechniqueofMasterWu = 611,		
	aaHostoftheElements = 616,		//DB
	aaCallofXuzl = 619,			//DB
	aaHastenedStealth = 622,		
	aaIngenuity = 625,			
	aaFleetofFoot = 628,			
	aaFadingMemories = 630,			
	aaTacticalMastery = 631,		
	aaTheftofLife = 634,			
	aaFuryofMagic = 637,
	aaFuryofMagicMastery2 = 640,		//whats the difference?
	aaProjectIllusion = 643,		
	aaHeadshot = 644,			
	aaEntrap = 645,				//DB
	aaUnholyTouch = 646,			//untested
	aaTotalDomination = 649,		
	aaStalwartEndurance = 652,		
	aaQuickSummoning2 = 655,		//whats the difference?
	aaMentalClarity2 = 658,			//whats the difference?
	aaInnateRegeneration2 = 661,	//whats the difference?
	aaManaBurn2 = 664,				//whats the difference?
	aaExtendedNotes2 = 665,			//whats the difference? untested
	aaSionachiesCrescendo2 = 668,	//whats the difference? untested
	aaImprovedReclaimEnergy2 = 671,	//whats the difference? untetsed
	aaSwiftJourney = 672,			
	aaConvalescence = 674,
	aaLastingBreath = 676,
	aaPackrat = 678,
	aaHeightenedEndurance = 683,
	aaWeaponAffinity = 686,				//implemented
	aaSecondaryForte = 691,
	aaPersistantCasting = 692,
	aaTuneofPursuance = 695,
	aaImprovedInstrumentMastery = 700,
	aaImprovedSingingMastery =701,
	aaExultantBellowing = 702,
	aaEchoofTaelosia = 707,
	aaInternalMetronome = 710,
	aaPiousSupplication = 715,
	aaBeastialAlignment = 718,		//untested
	aaWrathofXuzl = 721,
	aaFeralSwipe = 723,			//DB?
	aaWardersFury = 724,
	aaWardersAlacrity = 729,
	aaPetAffinity = 734,
	aaMasteryofthePast2 = 735,		//whats the difference?
	aaSpellCastingSubtlety2 = 738,		//whats the difference?
	aaTouchoftheDivine = 741,
	aaDivineAvatar = 746,			//DB
	aaExquisiteBenediction = 749,		//DB
	aaQuickenedCuring = 754,
	aaNaturesBoon = 757,			//DB
	aaAdvancedTracking = 762,
	aaCriticalAffliction = 767,
	aaFuryofMagicMastery = 770,	//whats the difference?
	aaDoppelganger = 773,
	aaEnchancedForgetfulness = 776,
	aaMesmerizationMastery = 781,
	aaQuickMassGroupBuff = 782,
	aaSharedHealth = 785,
	aaElementalFury = 790,
	aaElementalAlacrity = 795,
	aaElementalAgility = 800,
	aaElementalDurability = 803,
	aaSinisterStrikes = 806,
	aaStrikethrough = 807,
	aaStonewall = 810,
	aaRapidStrikes = 815,
	aaKickMastery = 820,
	aaHightenedAwareness = 823,
	aaDestructiveForce = 828,		//DB
	aaSwarmofDecay = 831,			//DB
	aaDeathsFury = 834,
	aaQuickeningofDeath = 839,
	aaAdvancedTheftofLife = 844,
	aaTripleBackstab = 846,
	aaHastenedPiety = 849,
	aaImmobilizingBash = 852,
	aaViciousSmash = 855,
	aaRadiantCure2 = 860,			//whats the difference?
	aaPurification = 863,			
	aaPrecisionofthePathfinder = 864,
	aaCoatofThistles = 867,
	aaFlamingArrows = 872,			//untested
	aaFrostArrows = 875,			//untested
	aaSeizedOpportunity = 878,
	aaTrapCircumvention = 881,
	aaImprovedHastyExit = 886,
	aaVirulentVenom = 888,
	aaImprovedConsumptionofSoul = 893,
	aaIntenseHatred = 895,
	aaAdvancedSpiritCall = 900,
	aaCalloftheAncients = 902,		//DB
	aaSturdiness = 907,
	aaWarlordsTenacity = 912,		//DB
	aaStrengthenedStrike = 915,
	aaExtendedShielding = 918,
	aaRosFlamingFamiliar = 921,		//DB
	aaEcisIcyFamiliar = 922,		//DB
	aaDruzzilsMysticalFamiliar = 923,	//DB
	aaAdvancedFuryofMagicMastery = 924,	
	aaWardofDestruction = 926,		//DB
	aaFrenziedDevistation = 931,		//DB
	aaUnknown = 932,
	aaHighestID		//this should always be last, and should always
					//follow the highest AA ID
} aaID;


//Structure representing the database's AA actions
struct AA_DBAction {
	uint16 reuse_time;			//in seconds
	int16 spell_id;				//spell to cast, SPELL_UNKNOWN=no spell
	aaTargetType target;		//from aaTargetType
	aaNonspellAction action;	//non-spell action to take
	uint16 mana_cost;			//mana the NON-SPELL action costs
	uint16 duration;			//duration of NON-SPELL effect, 0=N/A
	aaID redux_aa;				//AA which reduces reuse time
	sint32 redux_rate;			//%/point in redux_aa reduction in reuse time
};

//Structure representing the database's swarm pet configs
struct AA_SwarmPet {
	uint8 count;		//number to summon
	uint32 npc_id;		//id from npc_types to represent it.
	int16 duration;		//how long they last, in seconds
};

//assumes that no activatable AA has more than 5 ranks
#define MAX_AA_ACTION_RANKS 5
extern AA_DBAction AA_Actions[aaHighestID][MAX_AA_ACTION_RANKS];	//[aaid][rank]
extern map<int16, AA_SwarmPet> AA_SwarmPets;	//key=spell_id

#define AA_Choose3(val, v1, v2, v3) (val==1?v1:(val==2?v2:v3))

struct AltAdvStats_Struct {
/*000*/  uint32 experience;
/*004*/  uint16 unspent;
/*006*/  uint16	unknown006;
/*008*/  int8	percentage;
/*009*/  int8	unknown009[3];
};

struct UseAA_Struct {
	int32 begin;
	int32 ability;
	int32 end;
};

struct AA_Ability {
/*00*/	int32 skill_id;
/*04*/	int32 increase_amt;
/*08*/	int32 unknown08;
/*12*/	int32 last_level;
};

struct SendAA_Struct {
/*0000*/	int32 id;
/*0004*/	int32 hotkey_sid;
/*0008*/	int32 hotkey_sid2;
/*0012*/	int32 title_sid;
/*0016*/	int32 desc_sid;
/*0020*/	int32 class_type;
/*0024*/	int32 cost;
/*0028*/	int32 seq;
/*0032*/	int32 current_level; //1s
/*0036*/	int32 prereq_skill;
/*0040*/	int32 prereq_minpoints; //min points in the prereq
/*0044*/	int32 type;
/*0048*/	int32 spellid;
/*0052*/	int32 spell_type;
/*0056*/	int32 spell_refresh;
/*0060*/	int16 classes;
/*0062*/	int16 berserker; //seems to be 1 if its a berserker ability
/*0064*/	int32 max_level;
/*0068*/	int32 last_id;
/*0072*/	int32 next_id;
/*0076*/	int32 cost2;
/*0080*/	int32 unknown80[2]; //0s
/*0084*/	int32 total_abilities;
/*0088*/	AA_Ability abilities[0];
};
extern map<int32,SendAA_Struct*>aas_send;

struct AA_List {
	SendAA_Struct* aa[0];
};

enum {	//values of AA_Action.action
	aaActionActivate = 0,
	aaActionSetEXP = 1,
	aaActionDisableEXP = 2,
	aaActionBuy = 3
};

struct AA_Action {
/*00*/	int32	action;
/*04*/	int32	ability;
/*08*/	int32	unknown08;
/*12*/	int32	exp_value;
};

//  New Alternate Advancement table.  holds all the skill levels for the AA skills.
//  Length: 309 Bytes
//  OpCode: 1422
struct AA_Skills {
/*00*/	int32	aa_skill; 
/*04*/	int32	aa_value;
};

struct PlayerAA_Struct {
	AA_Skills aa_list[MAX_PP_AA_ARRAY];
};

struct AATable_Struct {
	AA_Skills aa_list[MAX_PP_AA_ARRAY];
};

#endif
