ALTER TABLE items ADD attuneable tinyint(3) unsigned NOT NULL default '0';

CREATE TABLE traps (
  id int(11) NOT NULL auto_increment,
  zone varchar(16) NOT NULL default '',
  x int(11) NOT NULL default '0',
  y int(11) NOT NULL default '0',
  z int(11) NOT NULL default '0',
  chance tinyint NOT NULL default '0',
  maxzdiff float NOT NULL default '0',
  radius float NOT NULL default '0',
  effect int(11) NOT NULL default '0',
  effectvalue int(11) NOT NULL default '0',
  effectvalue2 int(11) NOT NULL default '0',
  skill int(11) NOT NULL default '0',
  spawnchance int(11) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

drop table forage;
CREATE TABLE forage (
  id int(11) NOT NULL auto_increment,
  zoneid int(4) NOT NULL default '0',
  Itemid int(11) NOT NULL default '0',
  level smallint(6) NOT NULL default '0',
  chance smallint(6) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

CREATE TABLE fishing (
  id int(11) NOT NULL auto_increment,
  zoneid int(4) NOT NULL default '0',
  Itemid int(11) NOT NULL default '0',
  skill_level smallint(6) NOT NULL default '0',
  chance smallint(6) NOT NULL default '0',
  npc_id int NOT NULL default 0,
  npc_chance int NOT NULL default 0,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

CREATE TABLE timers (
	char_id INT(11) NOT NULL,
	type MEDIUMINT UNSIGNED NOT NULL,
	start INT UNSIGNED NOT NULL,
	duration INT UNSIGNED NOT NULL,
	enable TINYINT NOT NULL,
	PRIMARY KEY(char_id, type)
);

CREATE TABLE aa_swarmpets (
	spell_id mediumint unsigned not null,
	count tinyint unsigned not null,
	npc_id int not null,
	duration mediumint unsigned not null,
	PRIMARY KEY(spell_id)
);

CREATE TABLE aa_actions (
  aaid mediumint(8) unsigned NOT NULL default '0',
  rank tinyint(3) unsigned NOT NULL default '0',
  reuse_time mediumint(8) unsigned NOT NULL default '0',
  spell_id mediumint(8) unsigned NOT NULL default '0',
  target tinyint(3) unsigned NOT NULL default '0',
  nonspell_action tinyint(3) unsigned NOT NULL default '0',
  nonspell_mana mediumint(8) unsigned NOT NULL default '0',
  nonspell_duration mediumint(8) unsigned NOT NULL default '0',
  redux_aa mediumint(8) unsigned NOT NULL default '0',
  redux_rate tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (aaid,rank)
) TYPE=MyISAM;
