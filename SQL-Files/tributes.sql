CREATE TABLE tributes (
	id INT UNSIGNED AUTO_INCREMENT,
	name VARCHAR(255) NOT NULL,
	descr MEDIUMTEXT NOT NULL,
	PRIMARY KEY(id)
);

INSERT INTO tributes (id,name,descr) VALUES(1, 'Aura of Clarity', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(2, 'Vengeful Aura', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(3, 'Power of Will', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(4, 'Bulwark of Honor', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(5, 'Arm of Strength', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(6, 'Body of the Brute', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(7, 'Rabbit\'s Song', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(8, 'Juggler\'s Hands', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(9, 'Sage\'s Advice', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(10, 'Sage\'s Knowledge', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(11, 'Glowing Beauty', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(12, 'Strength of Body', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(13, 'Strength of Mind', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(14, 'Replenishing Body', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(15, 'Harmony of Drums', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(16, 'Harmony of Wind', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(17, 'Harmony of String', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(18, 'Harmony of Horns', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(19, 'Chorus', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(20, 'Concerto', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(21, 'Strength of Will', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(22, 'Visions of Suffering', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(23, 'Twinge of Pain', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(24, 'Expeditious Aid', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(25, 'Persistent Boon', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(26, 'Power of Sight', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(27, 'Power of Recovery', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(28, 'Power of Conservation', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(29, 'Power of Alacrity', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(30, 'Visions of Command', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(31, 'Commanding Presence', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(32, 'Swift Arms', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(33, 'Power of Suffering', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(34, 'Gills of the Bass', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(35, 'Candlelight Vigil', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(36, 'Sight of the Falcon', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(37, 'Gift of the Enchanter', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(38, 'Ethereal Protection', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(39, 'Blazing Shield', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(40, 'Insulation', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(41, 'Antibody', 'No Text Collected!');
INSERT INTO tributes (id,name,descr) VALUES(42, 'Antidote', 'No Text Collected!');


#Stupid mysql wont let you insert the value 0 into a INT UNSIGNED AUTO_INCREMENT field
UPDATE tributes SET id=id-1;
