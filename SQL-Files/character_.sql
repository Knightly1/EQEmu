# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'character_'
#

CREATE TABLE character_ (
  id int(11) NOT NULL auto_increment,
  account_id int(11) NOT NULL default '0',
  name varchar(64) NOT NULL default '',
  profile blob,
  guild int(11) default '0',
  guildrank tinyint(2) unsigned default '5',
  timelaston int(11) unsigned default '0',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  zonename varchar(30) NOT NULL default '',
  alt_adv blob,
  zoneid smallint(6) NOT NULL default '0',
  pktime int(8) NOT NULL default '0',
  inventory blob,
  publicnote varchar(100) NOT NULL default '',
  PRIMARY KEY  (id),
  UNIQUE KEY name (name)
) TYPE=MyISAM;

