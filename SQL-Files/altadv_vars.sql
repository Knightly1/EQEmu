# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eqrc
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'altadv_vars'
#

DROP TABLE IF EXISTS altadv_vars;
CREATE TABLE altadv_vars (
  skill_id int(11) NOT NULL default '0',
  name varchar(128) default NULL,
  cost int(11) default NULL,
  max_level int(11) default NULL,
  hotkey_sid int(10) unsigned NOT NULL default '0',
  hotkey_sid2 int(10) unsigned NOT NULL default '0',
  title_sid int(10) unsigned NOT NULL default '0',
  desc_sid int(10) unsigned NOT NULL default '0',
  type tinyint(3) unsigned NOT NULL default '1',
  spellid int(10) unsigned NOT NULL default '0',
  prereq_skill int(10) unsigned NOT NULL default '0',
  prereq_minpoints int(10) unsigned NOT NULL default '0',
  spell_type int(10) unsigned NOT NULL default '0',
  spell_refresh int(10) unsigned NOT NULL default '0',
  classes int(10) unsigned NOT NULL default '65534',
  berserker int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (skill_id)
) TYPE=MyISAM;



#
# Dumping data for table 'altadv_vars'
#

INSERT INTO altadv_vars VALUES("12", "Innate Agility", "1", "5", "4294967295", "4294967295", "13504", "13505", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("17", "Innate Dexterity", "1", "5", "4294967295", "4294967295", "13506", "13507", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("22", "Innate Intelligence", "1", "5", "4294967295", "4294967295", "13508", "13509", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("27", "Innate Wisdom", "1", "5", "4294967295", "4294967295", "13510", "13511", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("32", "Innate Charisma", "1", "5", "4294967295", "4294967295", "13512", "13513", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("37", "Innate Fire Protection", "1", "5", "4294967295", "4294967295", "13514", "13515", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("42", "Innate Cold Protection", "1", "5", "4294967295", "4294967295", "13516", "13517", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("47", "Innate Magic Protection", "1", "5", "4294967295", "4294967295", "13518", "13519", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("52", "Innate Poison Protection", "1", "5", "4294967295", "4294967295", "13520", "13521", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("57", "Innate Disease Protection", "1", "5", "4294967295", "4294967295", "13522", "13523", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("62", "Innate Run Speed", "1", "3", "4294967295", "4294967295", "13524", "13525", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("68", "Innate Metabolism", "1", "3", "4294967295", "4294967295", "13528", "13529", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("71", "Innate Lung Capacity", "1", "3", "4294967295", "4294967295", "13530", "13531", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("74", "First Aid", "1", "3", "4294967295", "4294967295", "13532", "13533", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("77", "Healing Adept", "2", "3", "4294967295", "4294967295", "13534", "13535", "2", "4294967295", "4294967295", "0", "0", "0", "33884", "0");
INSERT INTO altadv_vars VALUES("80", "Healing Gift", "2", "3", "4294967295", "4294967295", "13536", "13537", "2", "4294967295", "4294967295", "0", "0", "0", "33884", "0");
INSERT INTO altadv_vars VALUES("86", "Spell Casting Reinforcement", "2", "3", "4294967295", "4294967295", "13540", "13541", "2", "4294967295", "4294967295", "0", "0", "0", "50268", "0");
INSERT INTO altadv_vars VALUES("92", "Spell Casting Fury", "2", "3", "4294967295", "4294967295", "13544", "13545", "2", "4294967295", "4294967295", "0", "0", "0", "64892", "0");
INSERT INTO altadv_vars VALUES("95", "Channeling Focus", "2", "3", "4294967295", "4294967295", "13546", "13547", "2", "4294967295", "4294967295", "0", "0", "0", "64892", "0");
INSERT INTO altadv_vars VALUES("107", "Natural Durability", "2", "3", "4294967295", "4294967295", "13554", "13555", "2", "4294967295", "4294967295", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("110", "Natural Healing", "2", "3", "4294967295", "4294967295", "13556", "13557", "2", "4294967295", "4294967295", "0", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("113", "Combat Fury", "2", "3", "4294967295", "4294967295", "13558", "13559", "2", "4294967295", "4294967295", "0", "0", "0", "33192", "0");
INSERT INTO altadv_vars VALUES("116", "Fear Resistance", "2", "3", "4294967295", "4294967295", "13560", "13561", "2", "4294967295", "4294967295", "0", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("119", "Finishing Blow", "2", "3", "4294967295", "4294967295", "13562", "13563", "2", "4294967295", "4294967295", "0", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("122", "Combat Stability", "2", "3", "4294967295", "4294967295", "13564", "13565", "2", "4294967295", "4294967295", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("125", "Combat Agility", "2", "3", "4294967295", "4294967295", "13566", "13567", "2", "4294967295", "4294967295", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("128", "Mass Group Buff", "9", "1", "13811", "13812", "13809", "13810", "3", "5228", "4294967294", "0", "1", "4320", "60508", "0");
INSERT INTO altadv_vars VALUES("163", "Mend Companion", "5", "1", "13640", "13641", "13638", "13639", "3", "2752", "4294967294", "0", "2", "2160", "43008", "0");
INSERT INTO altadv_vars VALUES("198", "Ambidexterity", "9", "1", "4294967295", "4294967295", "13716", "13717", "3", "4294967295", "4294967294", "0", "0", "0", "33682", "0");
INSERT INTO altadv_vars VALUES("247", "Double Riposte", "3", "3", "4294967295", "4294967295", "13775", "13776", "3", "4294967295", "4294967294", "0", "0", "0", "33466", "1");
INSERT INTO altadv_vars VALUES("278", "Body and Mind Rejuvenation", "5", "1", "4294967295", "4294967295", "13819", "13820", "3", "4294967295", "4294967294", "0", "0", "0", "33080", "0");
INSERT INTO altadv_vars VALUES("279", "Physical Enhancement", "5", "1", "4294967295", "4294967295", "13821", "13822", "3", "4294967295", "4294967294", "0", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("288", "Pet Discipline", "6", "1", "4294967295", "4294967295", "13831", "13832", "3", "4294967295", "4294967294", "0", "0", "0", "44064", "0");
INSERT INTO altadv_vars VALUES("289", "Hobble of Spirits", "5", "1", "3300", "3301", "3298", "3299", "3", "3290", "4294967294", "0", "3", "300", "32768", "0");
INSERT INTO altadv_vars VALUES("290", "Frenzy of Spirit", "4", "1", "3304", "3305", "3302", "3303", "3", "3289", "4294967294", "0", "4", "720", "32768", "0");
INSERT INTO altadv_vars VALUES("291", "Paragon of Spirit", "6", "1", "3308", "3309", "3306", "3307", "3", "3291", "4294967294", "0", "5", "900", "32768", "0");
INSERT INTO altadv_vars VALUES("292", "Advanced Innate Strength", "1", "10", "4294967295", "4294967295", "5546", "5550", "4", "4294967295", "1", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("302", "Advanced Innate Stamina", "1", "10", "4294967295", "4294967295", "5551", "5552", "4", "4294967295", "2", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("312", "Advanced Innate Agility", "1", "10", "4294967295", "4294967295", "5553", "5554", "4", "4294967295", "3", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("322", "Advanced Innate Dexterity", "1", "10", "4294967295", "4294967295", "5555", "5556", "4", "4294967295", "4", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("332", "Advanced Innate Intelligence", "1", "10", "4294967295", "4294967295", "5557", "5558", "4", "4294967295", "5", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("342", "Advanced Innate Wisdom", "1", "10", "4294967295", "4294967295", "5559", "5563", "4", "4294967295", "6", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("352", "Advanced Innate Charisma", "1", "10", "4294967295", "4294967295", "5564", "5565", "4", "4294967295", "7", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("362", "Warding of Solusek", "1", "10", "4294967295", "4294967295", "5568", "5569", "4", "4294967295", "8", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("372", "Blessing of E\'ci", "1", "10", "4294967295", "4294967295", "5570", "5571", "4", "4294967295", "9", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("382", "Marr\'s Protection", "1", "10", "4294967295", "4294967295", "5566", "5567", "4", "4294967295", "10", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("392", "Shroud of The Faceless", "1", "10", "4294967295", "4294967295", "5572", "5573", "4", "4294967295", "11", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("402", "Bertoxxulous\' Gift", "1", "10", "4294967295", "4294967295", "5574", "5575", "4", "4294967295", "12", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("412", "New Tanaan Crafting Mastery", "3", "6", "4294967295", "4294967295", "3286", "3287", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("418", "Planar Power", "2", "5", "4294967295", "4294967295", "5547", "5548", "4", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("434", "Advanced Healing Adept", "2", "3", "4294967295", "4294967295", "5578", "5579", "5", "4294967295", "18", "3", "0", "0", "33884", "0");
INSERT INTO altadv_vars VALUES("437", "Advanced Healing Gift", "2", "3", "4294967295", "4294967295", "5580", "5581", "5", "4294967295", "19", "3", "0", "0", "33884", "0");
INSERT INTO altadv_vars VALUES("440", "Coup de Grace", "2", "3", "4294967295", "4294967295", "5618", "5619", "5", "4294967295", "32", "3", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("443", "Fury of the Ages", "3", "3", "4294967295", "4294967295", "5620", "5621", "5", "4294967295", "30", "3", "0", "0", "33192", "0");
INSERT INTO altadv_vars VALUES("449", "Lightning Reflexes", "3", "5", "4294967295", "4294967295", "5636", "5637", "5", "4294967295", "34", "3", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("454", "Innate Defense", "3", "5", "4294967295", "4294967295", "5638", "5639", "5", "4294967295", "33", "3", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("483", "Hastened Mending", "2", "3", "4294967295", "4294967295", "5598", "5599", "5", "4294967295", "58", "1", "0", "0", "43008", "0");
INSERT INTO altadv_vars VALUES("504", "Flash of Steel", "3", "3", "4294967295", "4294967295", "5622", "5623", "5", "4294967295", "104", "3", "0", "0", "33466", "1");
INSERT INTO altadv_vars VALUES("526", "Suspended Minion", "5", "2", "5710", "5711", "5626", "5627", "5", "3248", "0", "0", "0", "1", "60448", "0");
INSERT INTO altadv_vars VALUES("551", "Bestial Frenzy", "2", "5", "4294967295", "4294967295", "5650", "5651", "5", "4294967295", "0", "0", "0", "0", "32768", "0");
INSERT INTO altadv_vars VALUES("658", "Mental Clarity", "2", "3", "4294967295", "4294967295", "13542", "13543", "2", "4294967295", "4294967295", "0", "0", "0", "64892", "0");
INSERT INTO altadv_vars VALUES("661", "Innate Regeneration", "1", "3", "4294967295", "4294967295", "13526", "13527", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("672", "Swift Journey", "5", "2", "4294967295", "4294967295", "9151", "9152", "6", "4294967295", "13", "3", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("674", "Convalescence", "3", "2", "4294967295", "4294967295", "9153", "9154", "6", "4294967295", "225", "3", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("676", "Lasting Breath", "2", "2", "4294967295", "4294967295", "9155", "9156", "6", "4294967295", "16", "3", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("678", "Packrat", "3", "5", "4294967295", "4294967295", "9325", "9326", "6", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("686", "Weapon Affinity", "2", "5", "4294967295", "4294967295", "9159", "9160", "6", "4294967295", "4294967295", "0", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("692", "Persistent Casting", "3", "3", "4294967295", "4294967295", "9187", "9188", "6", "4294967295", "4294967295", "0", "0", "0", "64892", "0");
INSERT INTO altadv_vars VALUES("718", "Bestial Alignment", "3", "3", "9219", "4294967295", "9219", "9220", "6", "4521", "0", "0", "7", "4320", "32768", "0");
INSERT INTO altadv_vars VALUES("723", "Feral Swipe", "9", "1", "9175", "4294967295", "9175", "9176", "6", "4788", "0", "0", "6", "60", "32768", "0");
INSERT INTO altadv_vars VALUES("724", "Warder\'s Fury", "3", "5", "4294967295", "4294967295", "9177", "9178", "6", "4294967295", "0", "0", "0", "0", "32768", "0");
INSERT INTO altadv_vars VALUES("729", "Warder\'s Alacrity", "3", "5", "4294967295", "4294967295", "9179", "9180", "6", "4294967295", "0", "0", "0", "0", "32768", "0");
INSERT INTO altadv_vars VALUES("734", "Pet Affinity", "20", "1", "4294967295", "4294967295", "9285", "9286", "6", "4294967295", "0", "0", "0", "0", "60448", "0");
INSERT INTO altadv_vars VALUES("735", "Mastery of the Past", "3", "3", "4294967295", "4294967295", "9163", "9164", "6", "4294967295", "0", "0", "0", "0", "32784", "0");
INSERT INTO altadv_vars VALUES("767", "Critical Affliction", "5", "3", "4294967295", "4294967295", "9191", "9192", "6", "4294967295", "0", "0", "0", "0", "36192", "0");
INSERT INTO altadv_vars VALUES("806", "Sinister Strikes", "15", "1", "4294967295", "4294967295", "9231", "9232", "6", "4294967295", "81", "1", "0", "0", "33682", "0");
INSERT INTO altadv_vars VALUES("978", "Eternal Breath", "5", "1", "4294967295", "4294967295", "30012", "30013", "7", "4294967295", "232", "2", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("979", "Blacksmithing Mastery", "3", "3", "4294967295", "4294967295", "30016", "30017", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("982", "Baking Mastery", "3", "3", "4294967295", "4294967295", "30028", "30029", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("985", "Brewing Mastery", "3", "3", "4294967295", "4294967295", "30040", "30041", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("988", "Fletching Mastery", "3", "3", "4294967295", "4294967295", "30052", "30053", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("991", "Pottery Mastery", "3", "3", "4294967295", "4294967295", "30064", "30065", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("994", "Tailoring Mastery", "3", "3", "4294967295", "4294967295", "30076", "30077", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("997", "Salvage", "5", "3", "4294967295", "4294967295", "30088", "30089", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1000", "Origin", "7", "1", "30102", "4294967295", "30100", "30101", "7", "5824", "0", "0", "20", "4320", "65534", "1");
INSERT INTO altadv_vars VALUES("1001", "Chaotic Potential", "5", "5", "4294967295", "4294967295", "30104", "30105", "7", "4294967295", "142", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1006", "Discordant Defiance", "5", "5", "4294967295", "4294967295", "30124", "30125", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1021", "Mystical Attuning", "5", "5", "4294967295", "4294967295", "30144", "30145", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1026", "Delay Death", "3", "5", "4294967295", "4294967295", "30164", "30165", "7", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1031", "Healthy Aura", "3", "5", "4294967295", "4294967295", "30184", "30185", "7", "4294967295", "231", "2", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1041", "Veteran\'s Wrath", "3", "3", "4294967295", "4294967295", "30224", "30225", "7", "4294967295", "149", "3", "0", "0", "33192", "0");
INSERT INTO altadv_vars VALUES("1053", "Deathblow", "3", "3", "4294967295", "4294967295", "30272", "30273", "7", "4294967295", "148", "3", "0", "0", "33722", "1");
INSERT INTO altadv_vars VALUES("1061", "Reflexive Mastery", "5", "5", "4294967295", "4294967295", "30304", "30305", "7", "4294967295", "151", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1066", "Defensive Instincts", "5", "5", "4294967295", "4294967295", "30324", "30325", "7", "4294967295", "152", "5", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("1071", "Mnemonic Retention", "3", "1", "4294967295", "4294967295", "30344", "30345", "7", "4294967295", "0", "0", "0", "0", "64892", "0");
INSERT INTO altadv_vars VALUES("1072", "Expansive Mind", "5", "5", "4294967295", "4294967295", "30348", "30349", "7", "4294967295", "0", "0", "0", "0", "64892", "0");
INSERT INTO altadv_vars VALUES("1083", "Healing Adept Mastery", "3", "3", "4294967295", "4294967295", "30392", "30393", "7", "4294967295", "146", "3", "0", "0", "33884", "0");
INSERT INTO altadv_vars VALUES("1086", "Healing Gift Mastery", "3", "3", "4294967295", "4294967295", "30404", "30405", "7", "4294967295", "147", "3", "0", "0", "33884", "0");
INSERT INTO altadv_vars VALUES("1093", "Slippery Attacks", "3", "5", "4294967295", "4294967295", "30432", "30433", "7", "4294967295", "0", "0", "0", "0", "33682", "0");
INSERT INTO altadv_vars VALUES("1099", "Improved Critical Affliction", "3", "3", "4294967295", "4294967295", "30456", "30457", "7", "4294967295", "259", "3", "0", "0", "36192", "0");
INSERT INTO altadv_vars VALUES("1107", "Fury of Magic", "3", "3", "4294967295", "4294967295", "5738", "5739", "7", "4294967295", "23", "3", "0", "0", "33080", "0");
INSERT INTO altadv_vars VALUES("1119", "Roar of Thunder", "3", "3", "30538", "30539", "30536", "30537", "7", "5841", "0", "0", "8", "900", "32768", "0");
INSERT INTO altadv_vars VALUES("1122", "Persistent Minion", "7", "1", "4294967295", "4294967295", "30548", "30549", "7", "4294967295", "176", "2", "0", "0", "60448", "0");
INSERT INTO altadv_vars VALUES("1123", "Perfection of Spirit", "5", "3", "30554", "30555", "30552", "30553", "7", "5854", "128", "1", "5", "900", "32768", "0");
INSERT INTO altadv_vars VALUES("1126", "Replenish Companion", "3", "3", "30566", "30567", "30564", "30565", "7", "5845", "58", "1", "2", "2160", "43008", "0");
INSERT INTO altadv_vars VALUES("1129", "Advanced Pet Discipline", "5", "2", "4294967295", "4294967295", "30576", "30577", "7", "4294967295", "125", "1", "0", "0", "44064", "0");
INSERT INTO altadv_vars VALUES("1181", "Shielding Resistance", "3", "5", "4294967295", "4294967295", "30784", "30785", "7", "4294967295", "0", "0", "0", "0", "33682", "0");
INSERT INTO altadv_vars VALUES("2", "Innate Strength", "1", "5", "4294967295", "4294967295", "13500", "13501", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("7", "Innate Stamina", "1", "5", "4294967295", "4294967295", "13502", "13503", "1", "4294967295", "0", "0", "0", "0", "65534", "1");
INSERT INTO altadv_vars VALUES("83", "Spell Casting Mastery", "2", "3", "4294967295", "4294967295", "13538", "13539", "2", "4294967295", "4294967295", "0", "0", "0", "31812", "0");
INSERT INTO altadv_vars VALUES("98", "Spell Casting Subtlety", "2", "3", "4294967295", "4294967295", "13548", "13549", "2", "4294967295", "4294967295", "0", "0", "0", "31008", "0");
INSERT INTO altadv_vars VALUES("101", "Spell Casting Expertise", "2", "3", "4294967295", "4294967295", "13550", "13551", "2", "4294967295", "4294967295", "0", "0", "0", "31008", "0");
INSERT INTO altadv_vars VALUES("104", "Spell Casting Deftness", "2", "3", "4294967295", "4294967295", "13552", "13553", "2", "4294967295", "4294967295", "0", "0", "0", "30752", "0");
INSERT INTO altadv_vars VALUES("137", "Quick Evacuation", "3", "3", "4294967295", "4294967295", "13592", "13593", "3", "4294967295", "4294967294", "0", "0", "0", "4160", "0");
INSERT INTO altadv_vars VALUES("140", "Exodus", "6", "1", "13596", "4294967295", "13594", "13595", "3", "2771", "4294967294", "0", "3", "4320", "4160", "0");
INSERT INTO altadv_vars VALUES("141", "Quick Damage", "3", "3", "4294967295", "4294967295", "13598", "13599", "3", "4294967295", "23", "3", "0", "0", "12352", "0");
INSERT INTO altadv_vars VALUES("155", "Improved Familiar", "9", "1", "13624", "13625", "13622", "13623", "3", "2758", "4294967294", "0", "1", "420", "4096", "0");
INSERT INTO altadv_vars VALUES("156", "Nexus Gate", "6", "1", "13628", "13629", "13626", "13627", "3", "2734", "4294967294", "0", "2", "4320", "4096", "0");
INSERT INTO altadv_vars VALUES("267", "Spell Casting Fury Mastery", "3", "3", "4294967295", "4294967295", "13803", "13804", "3", "4294967295", "23", "3", "0", "0", "4096", "0");
INSERT INTO altadv_vars VALUES("274", "Strong Root", "5", "1", "13815", "13816", "13813", "13814", "3", "2748", "4294967294", "0", "4", "4320", "4096", "0");
INSERT INTO altadv_vars VALUES("426", "Innate Enlightenment", "3", "5", "4294967295", "4294967295", "5561", "5562", "4", "4294967295", "0", "0", "0", "0", "30720", "0");
INSERT INTO altadv_vars VALUES("446", "Mastery of the Past", "3", "3", "4294967295", "4294967295", "5624", "5625", "5", "4294967295", "26", "3", "0", "0", "31008", "0");
INSERT INTO altadv_vars VALUES("477", "Hastened Exodus", "2", "3", "4294967295", "4294967295", "5594", "5595", "5", "4294967295", "43", "1", "0", "0", "4160", "0");
INSERT INTO altadv_vars VALUES("480", "Hastened Root", "2", "3", "4294967295", "4294967295", "5596", "5597", "5", "4294967295", "117", "1", "0", "0", "4096", "0");
INSERT INTO altadv_vars VALUES("516", "Harvest of Druzzil", "2", "1", "5702", "5703", "5538", "5539", "5", "3338", "0", "0", "5", "480", "4096", "0");
INSERT INTO altadv_vars VALUES("533", "Allegiant Familiar", "6", "1", "9244", "13625", "5632", "5633", "5", "3264", "52", "1", "1", "420", "4096", "0");
INSERT INTO altadv_vars VALUES("619", "Call of Xuzl", "3", "3", "3290", "3291", "3288", "3289", "5", "3292", "0", "0", "6", "900", "4096", "0");
INSERT INTO altadv_vars VALUES("640", "Fury of Magic Mastery", "2", "3", "4294967295", "4294967295", "5740", "5741", "5", "4294967295", "114", "3", "0", "0", "4096", "0");
INSERT INTO altadv_vars VALUES("664", "Mana Burn", "5", "1", "13620", "13621", "13618", "13619", "3", "2751", "224", "3", "7", "8640", "4096", "0");
INSERT INTO altadv_vars VALUES("691", "Secondary Forte", "15", "1", "4294967295", "4294967295", "9161", "9162", "6", "4294967295", "4294967295", "0", "0", "0", "31812", "0");
INSERT INTO altadv_vars VALUES("721", "Wrath of Xuzl", "5", "2", "3290", "3291", "9199", "9200", "6", "5110", "208", "3", "6", "900", "4096", "0");
INSERT INTO altadv_vars VALUES("921", "Ro\'s Flaming Familiar", "15", "1", "9269", "4294967295", "9269", "9270", "6", "4833", "0", "0", "8", "60", "4096", "0");
INSERT INTO altadv_vars VALUES("922", "E\'ci\'s Icy Familiar", "15", "1", "9227", "4294967295", "9227", "9228", "6", "4834", "0", "0", "8", "60", "4096", "0");
INSERT INTO altadv_vars VALUES("923", "Druzzil\'s Mystical Familiar", "15", "1", "9279", "4294967295", "9279", "9280", "6", "4835", "0", "0", "8", "60", "4096", "0");
INSERT INTO altadv_vars VALUES("924", "Advanced Fury of Magic Mastery", "5", "2", "9265", "4294967295", "9265", "9266", "6", "4294967295", "216", "3", "0", "0", "4096", "0");
INSERT INTO altadv_vars VALUES("926", "Ward of Destruction", "3", "5", "9319", "4294967295", "9319", "9320", "6", "4836", "0", "0", "12", "1800", "4096", "0");
INSERT INTO altadv_vars VALUES("931", "Frenzied Devastation", "5", "3", "9340", "4294967295", "9340", "9341", "6", "5245", "23", "1", "13", "4320", "4096", "0");
INSERT INTO altadv_vars VALUES("1089", "Arcane Tongues", "3", "3", "4294967295", "4294967295", "30416", "30417", "7", "4294967295", "0", "0", "0", "0", "30720", "0");
INSERT INTO altadv_vars VALUES("1210", "Destructive Fury", "3", "3", "4294967295", "4294967295", "30900", "30901", "7", "4294967295", "216", "3", "0", "0", "4096", "0");
INSERT INTO altadv_vars VALUES("1229", "Secondary Recall", "7", "1", "30978", "30979", "30976", "30977", "7", "6094", "0", "0", "10", "600", "4160", "0");
INSERT INTO altadv_vars VALUES("1334", "Mind Crash", "3", "3", "31410", "31411", "31408", "31409", "7", "5943", "0", "0", "11", "4320", "4096", "0");
INSERT INTO altadv_vars VALUES("1337", "Prolonged Destruction", "5", "3", "31422", "31423", "31420", "31421", "7", "5946", "308", "3", "13", "4320", "4096", "0");
INSERT INTO altadv_vars VALUES("1340", "Ro\'s Greater Familiar", "12", "1", "31434", "31435", "31432", "31433", "7", "5950", "303", "1", "8", "60", "4096", "0");
INSERT INTO altadv_vars VALUES("1341", "E\'ci\'s Greater Familiar", "12", "1", "31438", "31439", "31436", "31437", "7", "5951", "304", "1", "8", "60", "4096", "0");
INSERT INTO altadv_vars VALUES("1342", "Druzzil\'s Greater Familiar", "12", "1", "31442", "31443", "31440", "31441", "7", "5952", "305", "1", "8", "60", "4096", "0");
INSERT INTO altadv_vars VALUES("1343", "Teleport Bind", "9", "1", "31446", "31447", "31444", "31445", "7", "5953", "0", "0", "9", "300", "4096", "0");
INSERT INTO altadv_vars VALUES("1344", "Devoted Familiar", "12", "1", "31450", "31451", "31448", "31449", "7", "5949", "179", "1", "8", "60", "4096", "0");
