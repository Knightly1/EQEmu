# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'zone'
#

CREATE TABLE zone (
  short_name varchar(16) NOT NULL default '',
  file_name varchar(16) default NULL,
  long_name text,
  safe_x float NOT NULL default '0',
  safe_y float NOT NULL default '0',
  safe_z float NOT NULL default '0',
  minium_level tinyint(3) unsigned NOT NULL default '0',
  minium_status tinyint(3) unsigned NOT NULL default '0',
  zoneidnumber int(4) NOT NULL default '0',
  timezone int(5) NOT NULL default '0',
  maxclients int(5) NOT NULL default '0',
  weather smallint(6) NOT NULL default '1',
  PRIMARY KEY  (short_name),
  UNIQUE KEY zoneidnumber (zoneidnumber)
) TYPE=MyISAM;

