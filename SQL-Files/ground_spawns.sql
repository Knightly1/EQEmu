# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'ground_spawns'
#

CREATE TABLE ground_spawns (
  id int(11) unsigned NOT NULL auto_increment,
  zoneid int(10) unsigned NOT NULL default '0',
  max_x float NOT NULL default '2000',
  max_y float NOT NULL default '2000',
  max_z float NOT NULL default '10000',
  min_x float NOT NULL default '-2000',
  min_y float NOT NULL default '-2000',
  heading float NOT NULL default '0',
  name varchar(16) NOT NULL default '',
  item int(10) unsigned NOT NULL default '0',
  max_allowed int(10) unsigned NOT NULL default '1',
  respawn_timer bigint(20) unsigned NOT NULL default '300000',
  comment varchar(255) NOT NULL default '',
  PRIMARY KEY  (id)
) TYPE=MyISAM;



#
# Dumping data for table 'ground_spawns'
#

INSERT INTO ground_spawns VALUES("1", "54", "2400", "2400", "10000", "-2400", "-2400", "0", "IT403_ACTORDEF", "22119", "50", "300000", "Evergreen Leaves for Gfay");
INSERT INTO ground_spawns VALUES("2", "68", "3000", "3000", "10000", "-3000", "-3000", "0", "IT403_ACTORDEF", "16490", "50", "300000", "Oak Bark For Butcherblock");
