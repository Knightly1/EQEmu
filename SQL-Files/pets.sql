# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'pets'
#

CREATE TABLE pets (
  id int(11) NOT NULL default '0',
  max_hp int(11) NOT NULL default '0',
  cur_hp int(11) NOT NULL default '0',
  min_dmg int(10) unsigned NOT NULL default '0',
  max_dmg int(10) unsigned NOT NULL default '0',
  race smallint(5) unsigned NOT NULL default '0',
  class tinyint(2) unsigned NOT NULL default '0',
  level tinyint(2) unsigned NOT NULL default '0',
  size float NOT NULL default '0',
  texture tinyint(2) unsigned NOT NULL default '0',
  description text NOT NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

