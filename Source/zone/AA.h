
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
	aaInnateStrength = 1,			//works
	aaInnateStamina = 2,			//works
	aaInnateAgility = 3,			//works
	aaInnateDexterity = 4,			//works
	aaInnateIntelligence = 5,		//works
	aaInnateWisdom = 6,			//works
	aaInnateCharisma = 7,			//works
	aaInnateFireProtection = 8,		//works
	aaInnateColdProtection = 9,		//works
	aaInnateMagicProtection = 10,		//works
	aaInnatePoisonProtection = 11,		//works
	aaInnateDiseaseProtection = 12,		//works
	aaInnateRunSpeed = 13,			//works
	aaInnateRegeneration = 14,		//works
	aaInnateMetabolism = 15,		
	aaInnateLungCapacity = 16,		
	aaFirstAid = 17,			//untested
	aaHealingAdept = 18,			//untested
	aaHealingGift = 19,			//untested
	aaSpellCastingMastery = 20,		//untested
	aaSpellCastingReinforcement = 21,	//untested
	aaMentalClarity = 22,			
	aaSpellCastingFury = 23,		//untested
	aaChanellingFocus = 24,			
	aaSpellCastingSubtlety = 25,		//untested
	aaSpellCastingExpertise = 26,		//untested
	aaSpellCastingDeftness = 27,		//untested
	aaNaturalDurability = 28,		//works
	aaNaturalHealing = 29,			//untested
	aaCombatFury = 30,			//untested
	aaFearResistance = 31,			//untested
	aaFinishingBlow = 32,			//untested
	aaCombatStability = 33,
	aaCombatAgility = 34,
	aaMassGroupBuff = 35,			//untested
	aaDivineResurrection = 36,		//DB
	aaInnateInvisToUndead = 37,		//DB
	aaCelestialRegeneration = 38,		//untested
	aaBestowDivineAura = 39,		//DB
	aaTurnUndead = 40,			//DB
	aaPurifySoul = 41,			//DB
	aaQuickEvacuation = 42,			//untested
	aaExodus = 43,				//untested
	aaQuickDamage = 44,			//untested
	aaEnhancedRoot = 45,			
	aaDireCharm = 46,			//untested
	aaCannibalization = 47,			//DB
	aaQuickBuff = 48,			//untested
	aaAlchemyMastery = 49,			
	aaRabidBear = 50,			//DB
	aaManaBurn = 51,			//DB
	aaImprovedFamiliar = 52,		//untested, implemented?
	aaNexusGate = 53,			//DB
	aaUnknown54 = 54,			
	aaPermanentIllusion = 55,		
	aaJewelCraftMastery = 56,		
	aaGatherMana = 57,			//DB
	aaMendCompanion = 58,			//DB
	aaQuickSummoning = 59,			//untested
	aaFrenziedBurnout = 60,			//DB
	aaElementalFormFire = 61,		//DB
	aaElementalFormWater = 62,		//DB
	aaElementalFormEarth = 63,		//DB
	aaElementalFormAir = 64,		//DB
	aaImprovedReclaimEnergy = 65,		//untested
	aaTurnSummoned = 66,			//DB
	aaElementalPact = 67,			//DB
	aaLifeBurn = 68,			//DB
	aaDeadMesmerization = 69,		//DB
	aaFearstorm = 70,			//DB
	aaFleshToBone = 71,			//DB
	aaCallToCorpse = 72,			//DB
	aaDivineStun = 73,			//DB
	aaImprovedLayOfHands = 74,		
	aaSlayUndead = 75,			
	aaActOfValor = 76,			//DB
	aaHolySteed = 77,			//DB
	aaFearless = 78,			
	aa2HandBash = 79,			
	aaInnateCamouflage = 80,		//DB
	aaAmbidexterity = 81,			//untested
	aaArcheryMastery = 82,			//untested
	aaFletchingMastery = 83,		
	aaEndlessQuiver = 84,			//untested
	aaUnholySteed = 85,			//DB
	aaImprovedHarmTouch = 86,		//untested
	aaLeechTouch = 87,			//DB
	aaDeathPeace = 88,			
	aaSoulAbrasion = 89,			//untested
	aaInstrumentMastery = 90,		//untested
	aaUnknown91 = 91,			
	aaUnknown92 = 92,			
	aaUnknown93 = 93,			
	aaJamFest = 94,				
	aaUnknown95 = 95,
	aaSonicCall = 96,			
	aaCriticalMend = 97,			//untested
	aaPurifyBody = 98,			//DB
	aaChainCombo = 99,			
	aaRapidFeign = 100,			//untested
	aaReturnKick = 101,			
	aaEscape = 102,				//DB
	aaPoisonMastery = 103,			
	aaDoubleRiposte = 104,			//untested
	aaUnknown105 = 105,			
	aaQuickThrow = 106,			
	aaPurgePoison = 107,			//DB
	aaFlurry = 108,				//untested
	aaRampage = 109,			//untested
	aaAreaTaunt = 110,			//untested
	aaWarcry = 111,				//DB
	aaBandageWound = 112,			//untested
	aaSpellCastingReinforcementMastery = 113,	//untested
	aaSpellCastingFuryMastery = 114,	//untested
	aaExtendedNotes = 115,			//untested
//--- stopping point
	aaDragonPunch = 116,			
	aaStrongRoot = 117,			//DB
	aaSingingMastery = 118,			
	aaBodyAndMindRejuvenation = 119,	
	aaPhysicalEnhancement = 120,		//untested
	aaAdvTrapNegotiation = 121,		
	aaAcrobatics = 122,				//untested
	aaScribbleNotes = 123,			
	aaChaoticStab = 124,			
	aaPetDiscipline = 125,			
	aaHobbleofSpirits = 126,		//DB
	aaFrenzyofSpirit = 127,			//DB
	aaParagonofSpirit = 128,		//DB
	aaAdvancedInnateStrength = 129,
	aaAdvancedInnateStamina = 130,
	aaAdvancedInnateAgility = 131,
	aaAdvancedInnateDexterity = 132,
	aaAdvancedInnateIntelligence = 133,
	aaAdvancedInnateWisdom = 134,
	aaAdvancedInnateCharisma = 135,
	aaWardingofSolusek = 136,
	aaBlessingofEci = 137,
	aaMarrsProtection = 138,
	aaShroudofTheFaceless = 139,
	aaBertoxxulousGift = 140,
	aaNewTanaanCraftingMastery = 141,
	aaPlanarPower = 142,			//untested
	aaPlanarDurability = 143,
	aaInnateEnlightenment = 144,
	aaAdvancedSpellCastingMastery = 145,
	aaAdvancedHealingAdept = 146,		//untested
	aaAdvancedHealingGift = 147,		//untested
	aaCoupdeGrace = 148,
	aaFuryoftheAges = 149,
	aaMasteryofthePast = 150,
	aaLightningReflexes = 151,
	aaInnateDefense = 152,
	aaRadiantCure = 153,			//DB
	aaHastenedDivinity = 154,			//DB
	aaHastenedTurning = 155,			//DB
	aaHastenedPurificationofSoul = 156,	//DB
	aaHastenedGathering = 157,			//DB
	aaHastenedRabidity = 158,			//DB
	aaHastenedExodus = 159,				//DB
	aaHastenedRoot = 160,
	aaHastenedMending = 161,			//DB
	aaHastenedBanishment = 162,			
	aaHastenedInstigation = 163,		//DB, maybe
	aaFuriousRampage = 164,
	aaHastenedPurificationoftheBody = 165,	//DB
	aaHastyExit = 166,
	aaHastenedPurification = 167,		//DB
	aaFlashofSteel = 168,
	aaDivineArbitration = 169,		//DB
	aaWrathoftheWild = 170,			//DB
	aaVirulentParalysis = 171,		//DB
	aaHarvestofDruzzil = 172,		//DB
	aaEldritchRune = 173,			//DB
	aaServantofRo = 174,			//DB
	aaWaketheDead = 175,			//DB
	aaSuspendedMinion = 176,		//untested
	aaSpiritCall = 177,			//DB
	aaCelestialRenewal = 178,		
	aaAllegiantFamiliar = 179,		
	aaHandofPiety = 180,			//DB
	aaMithanielsBinding = 181,		//untested
	aaMendingoftheTranquil = 182,		
	aaRagingFlurry = 183,			
	aaGuardianoftheForest = 184,		
	aaSpiritoftheWood = 185,		//DB
	aaBestialFrenzy = 186,			//untested
	aaHarmoniousAttack = 187,		//untested
	aaKnightsAdvantage = 188,		
	aaFerocity = 189,			
	aaViscidRoots = 190,			
	aaSionachiesCrescendo = 191,		//untested
	aaAyonaesTutelage = 192,		
	aaFeignedMinion = 193,			
	aaUnfailingDivinity = 194,		
	aaAnimationEmpathy = 195,		
	aaRushtoJudgement = 196,		
	aaLivingShield = 197,			
	aaConsumptionoftheSoul = 198,		//untested
	aaBoastfulBellow = 199,			//DB
	aaFervrentBlessing = 200,		//untested
	aaTouchoftheWicked = 201,		//untested
	aaPunishingBlade = 202,			
	aaSpeedoftheKnight = 203,		
	aaShroudofStealth = 204,		
	aaNimbleEvasion = 205,			
	aaTechniqueofMasterWu = 206,		
	aaHostoftheElements = 207,		//DB
	aaCallofXuzl = 208,			//DB
	aaHastenedStealth = 209,		
	aaIngenuity = 210,			
	aaFleetofFoot = 211,			
	aaFadingMemories = 212,			
	aaTacticalMastery = 213,		
	aaTheftofLife = 214,			
	aaFuryofMagic = 215,
	aaFuryofMagicMastery2 = 216,		//whats the difference?
	aaProjectIllusion = 217,		
	aaHeadshot = 218,			
	aaEntrap = 219,				//DB
	aaUnholyTouch = 220,			//untested
	aaTotalDomination = 221,		
	aaStalwartEndurance = 222,		
	aaQuickSummoning2 = 223,		//whats the difference?
	aaMentalClarity2 = 224,			//whats the difference?
	aaInnateRegeneration2 = 225,	//whats the difference?
	aaManaBurn2 = 226,				//whats the difference?
	aaExtendedNotes2 = 227,			//whats the difference? untested
	aaSionachiesCrescendo2 = 228,	//whats the difference? untested
	aaImprovedReclaimEnergy2 = 229,	//whats the difference? untetsed
	aaSwiftJourney = 230,			
	aaConvalescence = 231,
	aaLastingBreath = 232,
	aaPackrat = 233,
	aaHeightenedEndurance = 234,
	aaWeaponAffinity = 235,				//implemented
	aaSecondaryForte = 236,
	aaPersistantCasting = 237,
	aaTuneofPursuance = 238,
	aaImprovedInstrumentMastery = 239,
	aaImprovedSingingMastery = 240,
	aaExultantBellowing = 241,
	aaEchoofTaelosia = 242,
	aaInternalMetronome = 243,
	aaPiousSupplication = 244,
	aaBeastialAlignment = 245,		//untested
	aaWrathofXuzl = 246,
	aaFeralSwipe = 247,			//DB?
	aaWardersFury = 248,
	aaWardersAlacrity = 249,
	aaPetAffinity = 250,
	aaMasteryofthePast2 = 251,		//whats the difference?
	aaSpellCastingSubtlety2 = 252,		//whats the difference?
	aaTouchoftheDivine = 253,
	aaDivineAvatar = 254,			//DB
	aaExquisiteBenediction = 255,		//DB
	aaQuickenedCuring = 256,
	aaNaturesBoon = 257,			//DB
	aaAdvancedTracking = 258,
	aaCriticalAffliction = 259,
	aaFuryofMagicMastery = 260,	//whats the difference?
	aaDoppelganger = 261,
	aaEnchancedForgetfulness = 262,
	aaMesmerizationMastery = 263,
	aaQuickMassGroupBuff = 264,
	aaSharedHealth = 265,
	aaElementalFury = 266,
	aaElementalAlacrity = 267,
	aaElementalAgility = 268,
	aaElementalDurability = 269,
	aaSinisterStrikes = 270,
	aaStrikethrough = 271,
	aaStonewall = 272,
	aaRapidStrikes = 273,
	aaKickMastery = 274,
	aaHightenedAwareness = 275,
	aaDestructiveForce = 276,		//DB
	aaSwarmofDecay = 277,			//DB
	aaDeathsFury = 278,
	aaQuickeningofDeath = 279,
	aaAdvancedTheftofLife = 280,
	aaTripleBackstab = 281,
	aaHastenedPiety = 282,
	aaImmobilizingBash = 283,
	aaViciousSmash = 284,
	aaRadiantCure2 = 285,			//whats the difference?
	aaPurification = 286,			
	aaPrecisionofthePathfinder = 287,
	aaCoatofThistles = 288,
	aaFlamingArrows = 289,			//untested
	aaFrostArrows = 290,			//untested
	aaSeizedOpportunity = 291,
	aaTrapCircumvention = 292,
	aaImprovedHastyExit = 293,
	aaVirulentVenom = 294,
	aaImprovedConsumptionofSoul = 295,
	aaIntenseHatred = 296,
	aaAdvancedSpiritCall = 297,
	aaCalloftheAncients = 298,		//DB
	aaSturdiness = 299,
	aaWarlordsTenacity = 300,		//DB
	aaStrengthenedStrike = 301,
	aaExtendedShielding = 302,
	aaRosFlamingFamiliar = 303,		//DB
	aaEcisIcyFamiliar = 304,		//DB
	aaDruzzilsMysticalFamiliar = 305,	//DB
	aaAdvancedFuryofMagicMastery = 306,	
	aaWardofDestruction = 307,		//DB
	aaFrenziedDevistation = 308,		//DB
	aaUnknown = 309,
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
