# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'player_corpses'
#

CREATE TABLE player_corpses (
  id int(11) unsigned NOT NULL auto_increment,
  charid int(11) unsigned NOT NULL default '0',
  charname varchar(64) NOT NULL default '',
  zoneid int(11) unsigned NOT NULL default '0',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  heading float NOT NULL default '0',
  data blob NOT NULL,
  timeofdeath datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (id),
  KEY zoneid (zoneid)
) TYPE=MyISAM;

