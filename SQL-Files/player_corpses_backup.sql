# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'player_corpses_backup'
#

CREATE TABLE player_corpses_backup (
  id int(11) unsigned NOT NULL auto_increment,
  charid int(11) unsigned NOT NULL default '0',
  parent_corpse_id int(11) unsigned NOT NULL default '0',
  zoneid int(11) unsigned NOT NULL default '0',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  heading float NOT NULL default '0',
  data blob NOT NULL,
  timeofdeath datetime NOT NULL default '0000-00-00 00:00:00',
  timeofdelete datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (id),
  KEY charid (charid)
) TYPE=MyISAM;

