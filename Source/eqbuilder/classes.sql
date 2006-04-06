
CREATE TABLE classes (
  name varchar(64) NOT NULL default '',
  id int(11) NOT NULL default '0',
  PRIMARY KEY  (name),
  KEY id (id)
) TYPE=MyISAM;

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
INSERT INTO classes VALUES ('gm beastlord',34);
INSERT INTO classes VALUES ('gm warrior',20);
INSERT INTO classes VALUES ('gm cleric',21);
INSERT INTO classes VALUES ('gm paladin',22);
INSERT INTO classes VALUES ('gm ranger',23);
INSERT INTO classes VALUES ('gm shadowknight',24);
INSERT INTO classes VALUES ('gm druid',25);
INSERT INTO classes VALUES ('gm monk',26);
INSERT INTO classes VALUES ('gm bard',27);
INSERT INTO classes VALUES ('gm rogue',28);
INSERT INTO classes VALUES ('gm shaman',29);
INSERT INTO classes VALUES ('gm necromancer',30);
INSERT INTO classes VALUES ('gm wizard',31);
INSERT INTO classes VALUES ('gm magician',32);
INSERT INTO classes VALUES ('gm enchanter',33);
INSERT INTO classes VALUES ('unknown 63',63);
INSERT INTO classes VALUES ('unknown 35',35);
INSERT INTO classes VALUES ('unknown 16',16);
INSERT INTO classes VALUES ('shadow knight',5);
INSERT INTO classes VALUES ('gm shadow knight',24);
INSERT INTO classes VALUES ('gm berserker',35);
INSERT INTO classes VALUES ('unknown_63',63);
INSERT INTO classes VALUES ('berserker',16);

