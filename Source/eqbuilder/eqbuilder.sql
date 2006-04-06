
CREATE TABLE builder_faction (
 zone varchar(16) NOT NULL,
 npc_faction_id INT(11) NOT NULL,
 match varchar(255) NOT NULL,
 PRIMARY KEY(zone,match)
);

CREATE TABLE builder_loot (
 zone varchar(16) NOT NULL,
 loottable_id INT(11) NOT NULL,
 match varchar(255) NOT NULL,
 PRIMARY KEY(zone,match)
);


--
-- Table structure for table `logs`
--

CREATE TABLE logs (
  zone varchar(16) NOT NULL default '',
  name varchar(128) NOT NULL default '',
  type int(10) unsigned NOT NULL default '0',
  KEY zone (zone),
  KEY name (name)
) TYPE=MyISAM;

--
-- Dumping data for table `logs`
--


--
-- Table structure for table `races`
--

CREATE TABLE races (
  name varchar(64) NOT NULL default '',
  id int(11) NOT NULL default '0',
  PRIMARY KEY  (name),
  KEY id (id)
) TYPE=MyISAM;

--
-- Dumping data for table `races`
--

INSERT INTO races VALUES ('Vase',379);
INSERT INTO races VALUES ('Spectre Pirate Boss',334);
INSERT INTO races VALUES ('Giant Spider',38);
INSERT INTO races VALUES ('Sarnak Golem',164);
INSERT INTO races VALUES ('Orc',54);
INSERT INTO races VALUES ('Gnome',12);
INSERT INTO races VALUES ('Sabertooth Cat',119);
INSERT INTO races VALUES ('Boat',141);
INSERT INTO races VALUES ('Undead Froglok',350);
INSERT INTO races VALUES ('Freeport Guards',44);
INSERT INTO races VALUES ('Aviak',13);
INSERT INTO races VALUES ('Yeti',138);
INSERT INTO races VALUES ('High Elf',5);
INSERT INTO races VALUES ('Velious Dragons',184);
INSERT INTO races VALUES ('StoneGrabber',220);
INSERT INTO races VALUES ('Bubonian',269);
INSERT INTO races VALUES ('Pusling',270);
INSERT INTO races VALUES ('Cockatrice',96);
INSERT INTO races VALUES ('Flying Monkey',168);
INSERT INTO races VALUES ('Chest',378);
INSERT INTO races VALUES ('Rujakian Orc',361);
INSERT INTO races VALUES ('Storm Celestial',312);
INSERT INTO races VALUES ('Shark',61);
INSERT INTO races VALUES ('Prismatic Dragon',198);
INSERT INTO races VALUES ('Evil Eye',21);
INSERT INTO races VALUES ('Kunark Goblin',137);
INSERT INTO races VALUES ('Troll Sea Rover',333);
INSERT INTO races VALUES ('Zombie',70);
INSERT INTO races VALUES ('Forest Giant',140);
INSERT INTO races VALUES ('Coldain',183);
INSERT INTO races VALUES ('Efreeti',101);
INSERT INTO races VALUES ('Gorilla',41);
INSERT INTO races VALUES ('Junk Beast',273);
INSERT INTO races VALUES ('Goo',145);
INSERT INTO races VALUES ('Kerra',23);
INSERT INTO races VALUES ('Griffin',47);
INSERT INTO races VALUES ('Cazic-Thule',95);
INSERT INTO races VALUES ('Djinn',126);
INSERT INTO races VALUES ('Wasp',109);
INSERT INTO races VALUES ('Minotaur',53);
INSERT INTO races VALUES ('Tunare',62);
INSERT INTO races VALUES ('Human Beggar',55);
INSERT INTO races VALUES ('Froglok',330);
INSERT INTO races VALUES ('Mouths of Insanity',281);
INSERT INTO races VALUES ('Walrus Man',191);
INSERT INTO races VALUES ('Ghost Dragon',196);
INSERT INTO races VALUES ('Kedge',103);
INSERT INTO races VALUES ('Gnoll',39);
INSERT INTO races VALUES ('Evil Eye New',375);
INSERT INTO races VALUES ('Undead Footman',324);
INSERT INTO races VALUES ('Teleport Man',240);
INSERT INTO races VALUES ('Halfling',11);
INSERT INTO races VALUES ('Ghoul',33);
INSERT INTO races VALUES ('Giant/Cyclops',18);
INSERT INTO races VALUES ('Iksar Scorpion',149);
INSERT INTO races VALUES ('Mermaid',110);
INSERT INTO races VALUES ('Snow Bunny',176);
INSERT INTO races VALUES ('Armadillo',87);
INSERT INTO races VALUES ('Synarcana',363);
INSERT INTO races VALUES ('Vallon Zek',289);
INSERT INTO races VALUES ('Box',376);
INSERT INTO races VALUES ('Ent',244);
INSERT INTO races VALUES ('Scorpion',129);
INSERT INTO races VALUES ('Elemental',75);
INSERT INTO races VALUES ('Fungal Fiend',218);
INSERT INTO races VALUES ('Invisible Man',127);
INSERT INTO races VALUES ('Golem New',374);
INSERT INTO races VALUES ('Tallon Zek',290);
INSERT INTO races VALUES ('Spectral Banshee',250);
INSERT INTO races VALUES ('Grobb Citizen',92);
INSERT INTO races VALUES ('Golem',17);
INSERT INTO races VALUES ('ShikNar',199);
INSERT INTO races VALUES ('Spectre',85);
INSERT INTO races VALUES ('Imp',46);
INSERT INTO races VALUES ('Froglok Skeleton',349);
INSERT INTO races VALUES ('Shade',224);
INSERT INTO races VALUES ('Wyvern',157);
INSERT INTO races VALUES ('Will O\' Wisp',69);
INSERT INTO races VALUES ('Table',380);
INSERT INTO races VALUES ('Black Knight',322);
INSERT INTO races VALUES ('Blood Raven',279);
INSERT INTO races VALUES ('Giant Rat',36);
INSERT INTO races VALUES ('Iksar Golem',160);
INSERT INTO races VALUES ('Walrus',177);
INSERT INTO races VALUES ('Hag',185);
INSERT INTO races VALUES ('Gelatinous Cube',31);
INSERT INTO races VALUES ('Manticore',172);
INSERT INTO races VALUES ('War Wraith',313);
INSERT INTO races VALUES ('Ogre',10);
INSERT INTO races VALUES ('Mimic',52);
INSERT INTO races VALUES ('Mosquito',134);
INSERT INTO races VALUES ('Xalgoz',136);
INSERT INTO races VALUES ('Human Pirate',341);
INSERT INTO races VALUES ('Clockwork Beetle',276);
INSERT INTO races VALUES ('Daisy Man',97);
INSERT INTO races VALUES ('Hippogriff',186);
INSERT INTO races VALUES ('Wrulon',314);
INSERT INTO races VALUES ('Fly Man',245);
INSERT INTO races VALUES ('Bixie',79);
INSERT INTO races VALUES ('Human',1);
INSERT INTO races VALUES ('Troll',9);
INSERT INTO races VALUES ('Iksar Hand',166);
INSERT INTO races VALUES ('Luggald Armored',346);
INSERT INTO races VALUES ('Ogre Pirate',340);
INSERT INTO races VALUES ('Spire Spirit',231);
INSERT INTO races VALUES ('Akheva',230);
INSERT INTO races VALUES ('Vampire',65);
INSERT INTO races VALUES ('Wolf',42);
INSERT INTO races VALUES ('Tentacle',68);
INSERT INTO races VALUES ('Succulent',167);
INSERT INTO races VALUES ('Bones',383);
INSERT INTO races VALUES ('The Rathe',298);
INSERT INTO races VALUES ('Sun Revenant',226);
INSERT INTO races VALUES ('Ghost',32);
INSERT INTO races VALUES ('Rallos Zek',66);
INSERT INTO races VALUES ('Mammoth',107);
INSERT INTO races VALUES ('Lava Dragon',49);
INSERT INTO races VALUES ('Eye of Zomm',108);
INSERT INTO races VALUES ('Oggok Citizen',93);
INSERT INTO races VALUES ('Tegi',215);
INSERT INTO races VALUES ('Bone Golem',362);
INSERT INTO races VALUES ('Pirate Boss',335);
INSERT INTO races VALUES ('Cold Spectre',174);
INSERT INTO races VALUES ('Scarlet Cheetah',221);
INSERT INTO races VALUES ('Old Froglok Ghoul',27);
INSERT INTO races VALUES ('Fungusman',28);
INSERT INTO races VALUES ('Spectral Sarnak',146);
INSERT INTO races VALUES ('Mummy New',368);
INSERT INTO races VALUES ('Pirate Dark Shaman',336);
INSERT INTO races VALUES ('Disease Boss',253);
INSERT INTO races VALUES ('Guard of Justice',251);
INSERT INTO races VALUES ('Clockwork Brain',249);
INSERT INTO races VALUES ('Seru',236);
INSERT INTO races VALUES ('Fairy',25);
INSERT INTO races VALUES ('Ghost Dwarf',117);
INSERT INTO races VALUES ('Ratman',156);
INSERT INTO races VALUES ('Alligator',91);
INSERT INTO races VALUES ('Leech',104);
INSERT INTO races VALUES ('Valorian',318);
INSERT INTO races VALUES ('Skeletal Horse',282);
INSERT INTO races VALUES ('Vah Shir King',238);
INSERT INTO races VALUES ('Gargoyle',29);
INSERT INTO races VALUES ('Raptor',163);
INSERT INTO races VALUES ('Zebuxoruk',295);
INSERT INTO races VALUES ('New Rallos Zek',288);
INSERT INTO races VALUES ('Harpie',111);
INSERT INTO races VALUES ('Unknown 180',180);
INSERT INTO races VALUES ('Enchanted Armor',175);
INSERT INTO races VALUES ('Launch',73);
INSERT INTO races VALUES ('Rujakian Orc Elite',366);
INSERT INTO races VALUES ('Luggald Robed',347);
INSERT INTO races VALUES ('Nightmare Goblin',277);
INSERT INTO races VALUES ('Broken Clockwork',274);
INSERT INTO races VALUES ('Faun',182);
INSERT INTO races VALUES ('Owlbear',206);
INSERT INTO races VALUES ('Sarnak',131);
INSERT INTO races VALUES ('Rhino',135);
INSERT INTO races VALUES ('Clockwork Dragon',192);
INSERT INTO races VALUES ('Troll Zombie',344);
INSERT INTO races VALUES ('Gnome Pirate',338);
INSERT INTO races VALUES ('Qeynos Citizen',71);
INSERT INTO races VALUES ('Wurm',158);
INSERT INTO races VALUES ('Ronnie Test',197);
INSERT INTO races VALUES ('Troll Freebooter',332);
INSERT INTO races VALUES ('Unknown_252',252);
INSERT INTO races VALUES ('Ottermen',190);
INSERT INTO races VALUES ('Barbarian',2);
INSERT INTO races VALUES ('Erudite Pirate',342);
INSERT INTO races VALUES ('PoP Efreeti',320);
INSERT INTO races VALUES ('Invalid',0);
INSERT INTO races VALUES ('Tribunal',151);
INSERT INTO races VALUES ('Tormentor',285);
INSERT INTO races VALUES ('Fish',24);
INSERT INTO races VALUES ('Lion',50);
INSERT INTO races VALUES ('Ship',72);
INSERT INTO races VALUES ('Rivervale Citizen',81);
INSERT INTO races VALUES ('Unicorn',124);
INSERT INTO races VALUES ('Water Dragon',165);
INSERT INTO races VALUES ('Chosen Wizard',352);
INSERT INTO races VALUES ('Fennin Ro',284);
INSERT INTO races VALUES ('Saryn',283);
INSERT INTO races VALUES ('Naiad',242);
INSERT INTO races VALUES ('Yak Man',181);
INSERT INTO races VALUES ('Puma',76);
INSERT INTO races VALUES ('Erollisi',150);
INSERT INTO races VALUES ('Unknown 142',142);
INSERT INTO races VALUES ('Queztocoatal',317);
INSERT INTO races VALUES ('Storm Mana',310);
INSERT INTO races VALUES ('Dragon Skeleton',122);
INSERT INTO races VALUES ('Skunk',83);
INSERT INTO races VALUES ('Unknown 143',143);
INSERT INTO races VALUES ('Water Elemental',211);
INSERT INTO races VALUES ('Old Froglok Tadpole',102);
INSERT INTO races VALUES ('Dire Wolf',171);
INSERT INTO races VALUES ('War Boar Unarmored',321);
INSERT INTO races VALUES ('Shrieker',227);
INSERT INTO races VALUES ('Highpass Citizen',67);
INSERT INTO races VALUES ('Innoruuk',123);
INSERT INTO races VALUES ('Scarecrow',82);
INSERT INTO races VALUES ('Pixie',56);
INSERT INTO races VALUES ('Piranha',74);
INSERT INTO races VALUES ('Poison Frog',316);
INSERT INTO races VALUES ('Gorgon',121);
INSERT INTO races VALUES ('Xegony',299);
INSERT INTO races VALUES ('Rock-gem Men',178);
INSERT INTO races VALUES ('Unknown 179',179);
INSERT INTO races VALUES ('Treant',64);
INSERT INTO races VALUES ('Mithaniel Marr',296);
INSERT INTO races VALUES ('Recuso',237);
INSERT INTO races VALUES ('Galorian',228);
INSERT INTO races VALUES ('Vacuum Worm',203);
INSERT INTO races VALUES ('Vampyre',208);
INSERT INTO races VALUES ('Mutant Humanoid',235);
INSERT INTO races VALUES ('Clockwork Gnome',88);
INSERT INTO races VALUES ('Tiger',63);
INSERT INTO races VALUES ('Iksar',128);
INSERT INTO races VALUES ('Sand Elf',364);
INSERT INTO races VALUES ('Troll Buccaneer',331);
INSERT INTO races VALUES ('Rallos Zek Minion',325);
INSERT INTO races VALUES ('Storm Satuur',307);
INSERT INTO races VALUES ('PoP Bear',305);
INSERT INTO races VALUES ('Weapons Rack',381);
INSERT INTO races VALUES ('Doppleganger',20);
INSERT INTO races VALUES ('Neriak Citizen',77);
INSERT INTO races VALUES ('Vampire Elite',360);
INSERT INTO races VALUES ('Zeb Cage',328);
INSERT INTO races VALUES ('Vah Shir',130);
INSERT INTO races VALUES ('Siren',187);
INSERT INTO races VALUES ('Demi Lich',45);
INSERT INTO races VALUES ('Giant Snake',37);
INSERT INTO races VALUES ('Totem',173);
INSERT INTO races VALUES ('Abhorent',193);
INSERT INTO races VALUES ('Shissar',217);
INSERT INTO races VALUES ('Grimling',202);
INSERT INTO races VALUES ('Barrel',377);
INSERT INTO races VALUES ('Animated Armor',323);
INSERT INTO races VALUES ('PoP Dragon',304);
INSERT INTO races VALUES ('Sonic Wolf',232);
INSERT INTO races VALUES ('Dwarf',8);
INSERT INTO races VALUES ('Pegasus',125);
INSERT INTO races VALUES ('Kunark Fish',148);
INSERT INTO races VALUES ('Fayguard',112);
INSERT INTO races VALUES ('Denizen',99);
INSERT INTO races VALUES ('Dark Elf Pirate',339);
INSERT INTO races VALUES ('Giant Clockwork',275);
INSERT INTO races VALUES ('Sphinx',86);
INSERT INTO races VALUES ('Devourer',159);
INSERT INTO races VALUES ('Veksar Boss',355);
INSERT INTO races VALUES ('BoT Portal',329);
INSERT INTO races VALUES ('Fire Mephit',293);
INSERT INTO races VALUES ('Lujein',241);
INSERT INTO races VALUES ('Ground Shaker',233);
INSERT INTO races VALUES ('Kaladim Citizen',94);
INSERT INTO races VALUES ('Half-Elf',7);
INSERT INTO races VALUES ('Bristlebane',153);
INSERT INTO races VALUES ('Lycanthrope',133);
INSERT INTO races VALUES ('Erudin Citizen',78);
INSERT INTO races VALUES ('Vampire Volatalis',219);
INSERT INTO races VALUES ('Vah Shir Guard',239);
INSERT INTO races VALUES ('Zelniak',222);
INSERT INTO races VALUES ('Wetfang Minnow',213);
INSERT INTO races VALUES ('Spectral Iksar',147);
INSERT INTO races VALUES ('Clockwork Golem',248);
INSERT INTO races VALUES ('Sunflower',225);
INSERT INTO races VALUES ('Old Froglok',26);
INSERT INTO races VALUES ('Fay Drake',154);
INSERT INTO races VALUES ('Snake Elemental',84);
INSERT INTO races VALUES ('Beetle',22);
INSERT INTO races VALUES ('Insect',370);
INSERT INTO races VALUES ('Veksar',353);
INSERT INTO races VALUES ('Netherbian',229);
INSERT INTO races VALUES ('Sea Horse',116);
INSERT INTO races VALUES ('Wood Elf',4);
INSERT INTO races VALUES ('Jokester',384);
INSERT INTO races VALUES ('Skeleton New',367);
INSERT INTO races VALUES ('Storm Fire',311);
INSERT INTO races VALUES ('Test Object',301);
INSERT INTO races VALUES ('Sol Ro',247);
INSERT INTO races VALUES ('Solusek Ro',58);
INSERT INTO races VALUES ('Kobold',48);
INSERT INTO races VALUES ('Frost Giant',188);
INSERT INTO races VALUES ('Kahli Shah',205);
INSERT INTO races VALUES ('Skeleton',60);
INSERT INTO races VALUES ('Halas Citizen',90);
INSERT INTO races VALUES ('New Bertox',255);
INSERT INTO races VALUES ('Iksar Citizen',139);
INSERT INTO races VALUES ('Underbulk',201);
INSERT INTO races VALUES ('Undead Veksar',358);
INSERT INTO races VALUES ('Undead Chokadai',357);
INSERT INTO races VALUES ('Crystal Spider',327);
INSERT INTO races VALUES ('Iksar Skeleton',161);
INSERT INTO races VALUES ('Chokadai',356);
INSERT INTO races VALUES ('Crab',302);
INSERT INTO races VALUES ('Undead Knight',297);
INSERT INTO races VALUES ('Vah Shir Skeleton',234);
INSERT INTO races VALUES ('Lizard Man',51);
INSERT INTO races VALUES ('Bertoxxulous',152);
INSERT INTO races VALUES ('Poison Dart Frog',343);
INSERT INTO races VALUES ('Earth Mephit',292);
INSERT INTO races VALUES ('Air Mephit',291);
INSERT INTO races VALUES ('Stormrider',272);
INSERT INTO races VALUES ('Ghost Ship',114);
INSERT INTO races VALUES ('Elf Vampire',98);
INSERT INTO races VALUES ('Erudite Ghost',118);
INSERT INTO races VALUES ('Sea Turtle',194);
INSERT INTO races VALUES ('Drake',89);
INSERT INTO races VALUES ('Greater Veksar',354);
INSERT INTO races VALUES ('Sol Ro Guard',254);
INSERT INTO races VALUES ('Brownie',15);
INSERT INTO races VALUES ('Storm Giant',189);
INSERT INTO races VALUES ('Pirate Officer',337);
INSERT INTO races VALUES ('Phoenix',303);
INSERT INTO races VALUES ('Chosen Warrior',351);
INSERT INTO races VALUES ('Storm Taarid',306);
INSERT INTO races VALUES ('Bear',43);
INSERT INTO races VALUES ('Coffin',382);
INSERT INTO races VALUES ('Air Elemental',210);
INSERT INTO races VALUES ('Thought Horror',214);
INSERT INTO races VALUES ('Swordfish',105);
INSERT INTO races VALUES ('Clam',115);
INSERT INTO races VALUES ('Were Wolf',14);
INSERT INTO races VALUES ('Earth Elemental',209);
INSERT INTO races VALUES ('Giant Eel',35);
INSERT INTO races VALUES ('War Boar',319);
INSERT INTO races VALUES ('Brontotherium',169);
INSERT INTO races VALUES ('Centaur',16);
INSERT INTO races VALUES ('Froglok Ghost',371);
INSERT INTO races VALUES ('Storm Kuraaln',308);
INSERT INTO races VALUES ('Nightmare Gargoyle',280);
INSERT INTO races VALUES ('Vampire Master',365);
INSERT INTO races VALUES ('Arachnid',326);
INSERT INTO races VALUES ('Karana',278);
INSERT INTO races VALUES ('Felguard',106);
INSERT INTO races VALUES ('Evan Test',204);
INSERT INTO races VALUES ('Trakanon',19);
INSERT INTO races VALUES ('Shadow Creatue',373);
INSERT INTO races VALUES ('Drixie',113);
INSERT INTO races VALUES ('Dark Elf',6);
INSERT INTO races VALUES ('Sarnak Skeleton',155);
INSERT INTO races VALUES ('Necro Priest',286);
INSERT INTO races VALUES ('Goblin New',369);
INSERT INTO races VALUES ('Luggald Land',345);
INSERT INTO races VALUES ('Storm Volaas',309);
INSERT INTO races VALUES ('Goblin',40);
INSERT INTO races VALUES ('Man Eating Plant',162);
INSERT INTO races VALUES ('Rhino Beetle',207);
INSERT INTO races VALUES ('Bloodgills',59);
INSERT INTO races VALUES ('Vampire Lesser',359);
INSERT INTO races VALUES ('Fiend',300);
INSERT INTO races VALUES ('Nightmare Mephit',294);
INSERT INTO races VALUES ('Snow Dervish',170);
INSERT INTO races VALUES ('Dervish New',372);
INSERT INTO races VALUES ('Froglok Mount',348);
INSERT INTO races VALUES ('Tarew Marr',246);
INSERT INTO races VALUES ('Rockhopper',200);
INSERT INTO races VALUES ('Wolf Elemental',120);
INSERT INTO races VALUES ('Dervish',100);
INSERT INTO races VALUES ('Dracnid',57);
INSERT INTO races VALUES ('Fire Elemental',212);
INSERT INTO races VALUES ('Horse',216);
INSERT INTO races VALUES ('Reanimated Hand',80);
INSERT INTO races VALUES ('Kraken',315);
INSERT INTO races VALUES ('Nymph',243);
INSERT INTO races VALUES ('Lightcrawler',223);
INSERT INTO races VALUES ('Giant Bat',34);
INSERT INTO races VALUES ('Erudite',3);
INSERT INTO races VALUES ('Nightmare',287);
INSERT INTO races VALUES ('Draglock',132);
INSERT INTO races VALUES ('Black and White Dragons',195);
INSERT INTO races VALUES ('Burynai',144);

ALTER TABLE races add no_coin TINYINT(1) NOT NULL DEFAULT '0';
UPDATE races SET no_coin=1 where id in (91,326,87,377,20,43,22,279,329,115,96,302,19,327,171,182,24,168,34,35,36,37,38,41,216,370,0,148,104,50,107,162,134,206,74,343,316,76,163,135,207,200,119,116,192,61,83,176,232,225,105,380,68,63,124,177,319,321,109,42);

--
-- Table structure for table `classes`
--

CREATE TABLE classes (
  name varchar(64) NOT NULL default '',
  id int(11) NOT NULL default '0',
  PRIMARY KEY  (name),
  KEY id (id)
) TYPE=MyISAM;

--
-- Dumping data for table `classes`
--

INSERT INTO classes VALUES ('warrior',1);
INSERT INTO classes VALUES ('cleric',2);
INSERT INTO classes VALUES ('paladin',3);
INSERT INTO classes VALUES ('ranger',4);
INSERT INTO classes VALUES ('shadowknight',5);
INSERT INTO classes VALUES ('druid',6);
INSERT INTO classes VALUES ('monk',7);
INSERT INTO classes VALUES ('bard',8);
INSERT INTO classes VALUES ('rogue',9);
INSERT INTO classes VALUES ('shaman',10);
INSERT INTO classes VALUES ('necromancer',11);
INSERT INTO classes VALUES ('wizard',12);
INSERT INTO classes VALUES ('magician',13);
INSERT INTO classes VALUES ('enchanter',14);
INSERT INTO classes VALUES ('beastlord',15);
INSERT INTO classes VALUES ('warriorgm',20);
INSERT INTO classes VALUES ('clericgm',21);
INSERT INTO classes VALUES ('paladingm',22);
INSERT INTO classes VALUES ('rangergm',23);
INSERT INTO classes VALUES ('shadowknightgm',24);
INSERT INTO classes VALUES ('druidgm',25);
INSERT INTO classes VALUES ('monkgm',26);
INSERT INTO classes VALUES ('bardgm',27);
INSERT INTO classes VALUES ('roguegm',28);
INSERT INTO classes VALUES ('shamangm',29);
INSERT INTO classes VALUES ('necromancergm',30);
INSERT INTO classes VALUES ('wizardgm',31);
INSERT INTO classes VALUES ('magiciangm',32);
INSERT INTO classes VALUES ('enchantergm',33);
INSERT INTO classes VALUES ('beastlordgm',34);
INSERT INTO classes VALUES ('banker',40);
INSERT INTO classes VALUES ('merchant',41);
INSERT INTO classes VALUES ('adventuremerchant',61);
INSERT INTO classes VALUES ('16',0);
INSERT INTO classes VALUES ('35',0);
INSERT INTO classes VALUES ('63',0);

