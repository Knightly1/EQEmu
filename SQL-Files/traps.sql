
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
