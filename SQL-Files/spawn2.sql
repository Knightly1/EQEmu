# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'spawn2'
#

CREATE TABLE spawn2 (
  id int(11) NOT NULL auto_increment,
  spawngroupID int(11) NOT NULL default '0',
  zone varchar(16) NOT NULL default '',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  heading float NOT NULL default '0',
  respawntime int(11) NOT NULL default '0',
  variance smallint(4) NOT NULL default '0',
  pathgrid int(10) NOT NULL default '0',
  timeleft bigint(16) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

