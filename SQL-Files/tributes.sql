CREATE TABLE tributes (
	id INT UNSIGNED NOT NULL,
	unknown INT UNSIGNED NOT NULL,
	name VARCHAR(255) NOT NULL,
	descr MEDIUMTEXT NOT NULL,
	PRIMARY KEY(id)
);

INSERT INTO tributes (id,name,descr,unknown) VALUES(1, 'Aura of Clarity', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(2, 'Vengeful Aura', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(3, 'Power of Will', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(4, 'Bulwark of Honor', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(5, 'Arm of Strength', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(6, 'Body of the Brute', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(7, 'Rabbit\'s Song', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(8, 'Juggler\'s Hands', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(9, 'Sage\'s Advice', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(10, 'Sage\'s Knowledge', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(11, 'Glowing Beauty', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(12, 'Strength of Body', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(13, 'Strength of Mind', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(14, 'Replenishing Body', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(15, 'Harmony of Drums', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(16, 'Harmony of Wind', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(17, 'Harmony of String', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(18, 'Harmony of Horns', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(19, 'Chorus', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(20, 'Concerto', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(21, 'Strength of Will', 'No Text Collected!',6);
INSERT INTO tributes (id,name,descr,unknown) VALUES(22, 'Visions of Suffering', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(23, 'Twinge of Pain', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(24, 'Expeditious Aid', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(25, 'Persistent Boon', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(26, 'Power of Sight', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(27, 'Power of Recovery', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(28, 'Power of Conservation', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(29, 'Power of Alacrity', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(30, 'Visions of Command', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(31, 'Commanding Presence', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(32, 'Swift Arms', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(33, 'Power of Suffering', 'No Text Collected!',4);
INSERT INTO tributes (id,name,descr,unknown) VALUES(34, 'Gills of the Bass', 'No Text Collected!',1);
INSERT INTO tributes (id,name,descr,unknown) VALUES(35, 'Candlelight Vigil', 'No Text Collected!',1);
INSERT INTO tributes (id,name,descr,unknown) VALUES(36, 'Sight of the Falcon', 'No Text Collected!',1);
INSERT INTO tributes (id,name,descr,unknown) VALUES(37, 'Gift of the Enchanter', 'No Text Collected!',1);
INSERT INTO tributes (id,name,descr,unknown) VALUES(38, 'Ethereal Protection', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(39, 'Blazing Shield', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(40, 'Insulation', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(41, 'Antibody', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(42, 'Antidote', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(43, 'Eyes of the Hunter', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(48, 'Arm of Power', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(49, 'Body of Divinity', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(50, 'Symphony of the Rabbit', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(51, 'Juggler\'s Grace', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(52, 'Sage\'s Requital', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(53, 'Sage\'s Comprehension', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(54, 'Countenance of Ardor', 'No Text Collected!',5);
INSERT INTO tributes (id,name,descr,unknown) VALUES(59, 'Second Chance', 'No Text Collected!',3);
INSERT INTO tributes (id,name,descr,unknown) VALUES(60, 'Fury of Combat', 'No Text Collected!',3);

#Stupid mysql wont let you insert the value 0 into a INT UNSIGNED AUTO_INCREMENT field
UPDATE tributes SET id=id-1;

UPDATE tributes SET descr='Our healers give you an antibody, which helps protect you from diseases.<br>Benefit -<br>5 Disease Resistance per tier..' WHERE id=40;
UPDATE tributes SET descr='Our healers give you an antidote, which helps protect you from poison.<br>Benefit -<br>5 Poison Resistance per tier..' WHERE id=41;
UPDATE tributes SET descr='Our greatest warriors sacrifice their power allow your body to break the limits of strength.<br>Benefit -<br>Tier 1: 2 Strength Cap Increase<br>Tier 2: 4 Strength Cap Increase<br>Tier 3: 6 Strength Cap Increase<br>Tier 4: 8 Strength Cap Increase<br>Tier 5: 10 Strength Cap Increase.' WHERE id=47;
UPDATE tributes SET descr='Our greatest warriors focus to increase your strength.<br>Benefit -<br>2 Strength per tier..' WHERE id=4;
UPDATE tributes SET descr='Fills you with inner peace increasing the rate you regain mana.<br>Benefit -<br>Tier 1: Flowing Thought I<br>Tier 2: Flowing Thought II<br>Tier 3: Flowing Thought III<br>Tier 4: Flowing Thought IV<br>Tier 5: Flowing Thought V.' WHERE id=0;
UPDATE tributes SET descr='Our enchanters offer you improved protection from fire.<br>Benefit -<br>5 Fire Resistance per tier..' WHERE id=38;
UPDATE tributes SET descr='Our greatest warriors sacrifice their endurance to allow your body to reach new limits of stamina.<br>Benefit -<br>Tier 1: 2 Stamina Cap Increase<br>Tier 2: 4 Stamina Cap Increase<br>Tier 3: 6 Stamina Cap Increase<br>Tier 4: 8 Stamina Cap Increase<br>Tier 5: 10 Stamina Cap Increase.' WHERE id=48;
UPDATE tributes SET descr='Our greatest warriors focus to increase your stamina.<br>Benefit -<br>2 Stamina per tier..' WHERE id=5;
UPDATE tributes SET descr='The honor of the people forms an armor protecting you.<br>Benefit -<br>Armor Class Increases per tier..' WHERE id=3;
UPDATE tributes SET descr='A candlelight vigil in your honor allows you greater vision when night is its darkest.<br>Benefit -<br>Ultravision.' WHERE id=34;
UPDATE tributes SET descr='The voices of the chorus accentuate your own.<br>Benefit -<br>Each tier increases the efficiency of singing songs..' WHERE id=18;
UPDATE tributes SET descr='Inspiring tales from a heroic commander lend speed to your summons.<br>Benefit -<br>Tier 1: Summoning Haste I<br>Tier 2: Summoning Haste II<br>Tier 3: Summoning Haste III<br>Tier 4: Summoning Haste IV.' WHERE id=30;
UPDATE tributes SET descr='All instruments and voices in the symphony rise in chorus.<br>Benefit -<br>Each tier increases the efficiency of all songs..' WHERE id=19;
UPDATE tributes SET descr='Your favor pools around you with a stunning aura.<br>Benefit -<br>Tier 1: 2 Charisma Cap Increase<br>Tier 2: 4 Charisma Cap Increase<br>Tier 3: 6 Charisma Cap Increase<br>Tier 4: 8 Charisma Cap Increase<br>Tier 5: 10 Charisma Cap Increase.' WHERE id=53;
UPDATE tributes SET descr='Our enchanters offer you improved protection from magic.<br>Benefit -<br>5 Magic Resistance per tier..' WHERE id=37;
UPDATE tributes SET descr='The charitable tales of a talented poet lend efficiency to your beneficial spells.<br>Benefit -<br>Tier 1: Enhancement Haste I<br>Tier 2: Enhancement Haste II<br>Tier 3: Enhancement Haste III<br>Tier 4: Enhancement Haste IV.' WHERE id=23;
UPDATE tributes SET descr='Our finest marksmen grant you improved accuracy.<br>Benefit -<br>Accuracy increases per tier..' WHERE id=42;
UPDATE tributes SET descr='The most skilled warriors bless your attacks.<br>Benefit -<br>Double attack chance increases per tier..' WHERE id=59;
UPDATE tributes SET descr='Our finest enchanters grant you the ability to breathe water as well as improving all aspects of your vision.<br>Benefit -<br>Faerune.' WHERE id=36;
UPDATE tributes SET descr='Our finest enchanters grant you the ability to breathe water as if it were air.<br>Benefit -<br>Enduring Breath.' WHERE id=33;
UPDATE tributes SET descr='Your favor makes you more beautiful on the outside, as well as the inside.<br>Benefit -<br>2 Charisma per tier..' WHERE id=10;
UPDATE tributes SET descr='The drums of the symphony play in harmony with you.<br>Benefit -<br>Each tier increases the efficiency of drum songs..' WHERE id=14;
UPDATE tributes SET descr='The horns of the symphony play in harmony with you.<br>Benefit -<br>Each tier increases the efficiency of brass songs..' WHERE id=17;
UPDATE tributes SET descr='The lutes of the symphony play in harmony with you.<br>Benefit -<br>Each tier increases the efficiency of string songs..' WHERE id=16;
UPDATE tributes SET descr='The flutes of the symphony play in harmony with you.<br>Benefit -<br>Each tier increases the efficiency of wind songs..' WHERE id=15;
UPDATE tributes SET descr='Our enchanters offer you improved protection from cold.<br>Benefit -<br>5 Cold Resistance per tier..' WHERE id=39;
UPDATE tributes SET descr='Our greatest juggliers sacrifice their grace to allow your to surpass the limits of dexterity.<br>Benefit -<br>Tier 1: 2 Dexterity Cap Increase<br>Tier 2: 4 Dexterity Cap Increase<br>Tier 3: 6 Dexterity Cap Increase<br>Tier 4: 8 Dexterity Cap Increase<br>Tier 5: 10 Dexterity Cap Increase.' WHERE id=50;
UPDATE tributes SET descr='Dexterity is achieved through invoking the will of our finest jugglers.<br>Benefit -<br>2 Dexterity per tier..' WHERE id=7;
UPDATE tributes SET descr='The charitable tales of a talented poet lend persistence to your beneficial spells.<br>Benefit -<br>Tier 1: Extend Enhancement I<br>Tier 2: Extend Enhancement II<br>Tier 3: Extend Enhancement III<br>Tier 4: Extend Enhancement IV.' WHERE id=24;
UPDATE tributes SET descr='The admiration of the people increases the speed of your magic.<br>Benefit -<br>Tier 1: Spell Haste I<br>Tier 2: Spell Haste II<br>Tier 3: Spell Haste III<br>Tier 4: Spell Haste IV.' WHERE id=28;
UPDATE tributes SET descr='The admiration of the people increases the efficiency of your magic.<br>Benefit -<br>Tier 1: Mana Preservation I<br>Tier 2: Mana Preservation II<br>Tier 3: Mana Preservation III<br>Tier 4: Mana Preservation IV.' WHERE id=27;
UPDATE tributes SET descr='The admiration of the people increases the potency of your healing.<br>Benefit -<br>Tier 1: Improved Healing I<br>Tier 2: Improved Healing II<br>Tier 3: Improved Healing III<br>Tier 4: Improved Healing IV.' WHERE id=26;
UPDATE tributes SET descr='The admiration of the people increases the range of your magic.<br>Benefit -<br>Tier 1: Extended Range I<br>Tier 2: Extended Range II<br>Tier 3: Extended Range III<br>Tier 4: Extended Range IV.' WHERE id=25;
UPDATE tributes SET descr='Memories of your ancestors sear the minds and bodies of your enemies.<br>Benefit -<br>Tier 1: Burning Affliction I<br>Tier 2: Burning Affliction II<br>Tier 3: Burning Affliction III<br>Tier 4: Burning Affliction IV.' WHERE id=32;
UPDATE tributes SET descr='The admiration of the people increases the potency of your magic.<br>Benefit -<br>Tier 1: Improved Damage I<br>Tier 2: Improved Damage II<br>Tier 3: Improved Damage III<br>Tier 4: Improved Damage IV.' WHERE id=2;
UPDATE tributes SET descr='Visions of your childhood invoke agile hands.<br>Benefit -<br>2 Agility per tier..' WHERE id=6;
UPDATE tributes SET descr='Our cities empaths constantly replenish your health.<br>Benefit -<br>Tier 1: Regeneration II<br>Tier 2: Regeneration IV<br>Tier 3: Regeneration VI<br>Tier 4: Regeneration VIII<br>Tier 5: Regeneration X.' WHERE id=13;
UPDATE tributes SET descr='Words of wisdom fill your head.<br>Benefit -<br>2 Wisdom per tier..' WHERE id=8;
UPDATE tributes SET descr='Our greatest sage\'s meditations allow your mind to comprehend beyond the limits of space and time.<br>Benefit -<br>Tier 1: 2 Intelligence Cap Increase<br>Tier 2: 4 Intelligence Cap Increase<br>Tier 3: 6 Intelligence Cap Increase<br>Tier 4: 8 Intelligence Cap Increase<br>Tier 5: 10 Intelligence Cap Increase.' WHERE id=52;
UPDATE tributes SET descr='Our greatest sages share their intellect with you.<br>Benefit -<br>2 Intelligence per tier..' WHERE id=9;
UPDATE tributes SET descr='Our greatest sages pool their insight to allow you wisdom beyond your years.<br>Benefit -<br>Tier 1: 2 Wisdom Cap Increase<br>Tier 2: 4 Wisdom Cap Increase<br>Tier 3: 6 Wisdom Cap Increase<br>Tier 4: 8 Wisdom Cap Increase<br>Tier 5: 10 Wisdom Cap Increase.' WHERE id=51;
UPDATE tributes SET descr='Our priests pray for your life, granting you a chance to return from the dead.<br>Benefit -<br>Each tier increases the chance you will resurrect upon death..' WHERE id=58;
UPDATE tributes SET descr='Our most practiced scouts offer you the gift of their vision.<br>Benefit -<br>See Invisible.' WHERE id=35;
UPDATE tributes SET descr='Our greatest warriors give you a portion of their vitality.<br>Benefit -<br>Tier 1: 10 Hit Points<br>Tier 2: 20 Hit Points<br>Tier 3: 50 Hit Points<br>Tier 4: 70 Hit Points<br>Tier 5: 100 Hit Points<br>Tier 6: 150 Hit Points.' WHERE id=11;
UPDATE tributes SET descr='Our greatest wizards give you a portion of their mana.<br>Benefit -<br>Tier 1: 10 Mana<br>Tier 2: 20 Mana<br>Tier 3: 50 Mana<br>Tier 4: 70 Mana<br>Tier 5: 100 Mana<br>Tier 6: 150 Mana.' WHERE id=12;
UPDATE tributes SET descr='Our greatest hunters give you a portion of their endurance.<br>Benefit -<br>Tier 1: 10 Endurance<br>Tier 2: 20 Endurance<br>Tier 3: 50 Endurance<br>Tier 4: 70 Endurance<br>Tier 5: 100 Endurance<br>Tier 6: 150 Endurance.' WHERE id=20;
UPDATE tributes SET descr='Songs from our greatest bards lend speed to your attacks<br>Benefit -<br>Tier 1: 21% Haste<br>Tier 2: 26% Haste<br>Tier 3: 31% Haste<br>Tier 4: 36% Haste<br>Tier 5: 41% Haste.' WHERE id=31;
UPDATE tributes SET descr='The dancing of our children allows you to move with unheard of agility.<br>Benefit -<br>Tier 1: 2 Agility Cap Increase<br>Tier 2: 4 Agility Cap Increase<br>Tier 3: 6 Agility Cap Increase<br>Tier 4: 8 Agility Cap Increase<br>Tier 5: 10 Agility Cap Increase.' WHERE id=49;
UPDATE tributes SET descr='Memories of your ancestors lend speed to your afflictions.<br>Benefit -<br>Tier 1: Affliction Haste I<br>Tier 2: Affliction Haste II<br>Tier 3: Affliction Haste III<br>Tier 4: Affliction Haste IV.' WHERE id=22;
UPDATE tributes SET descr='You become a tool of reckoning! All of your weapons are more efficient.<br>Benefit -<br>Tier 1: Vengeance II<br>Tier 2: Vengeance IV<br>Tier 3: Vengeance VI<br>Tier 4: Vengeance VIII<br>Tier 5: Vengeance X.' WHERE id=1;
UPDATE tributes SET descr='Inspiring tales from a heroic commander lend efficiency to your summons.<br>Benefit -<br>Tier 1: Summoning Efficiency I<br>Tier 2: Summoning Efficiency II<br>Tier 3: Summoning Efficiency III<br>Tier 4: Summoning Efficiency IV.' WHERE id=29;
UPDATE tributes SET descr='Memories of your ancestors lend efficiency to your afflictions.<br>Benefit -<br>Tier 1: Affliction Efficiency I<br>Tier 2: Affliction Efficiency II<br>Tier 3: Affliction Efficiency III<br>Tier 4: Affliction Efficiency IV.' WHERE id=21;

