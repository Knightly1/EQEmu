# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'character_backup'
#

CREATE TABLE character_backup (
  id int(11) unsigned NOT NULL auto_increment,
  charid int(11) unsigned NOT NULL default '0',
  account_id int(11) unsigned NOT NULL default '0',
  name varchar(64) NOT NULL default '',
  profile blob,
  guild int(11) unsigned default '0',
  guildrank tinyint(2) unsigned default '5',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  zonename varchar(30) NOT NULL default '',
  alt_adv blob,
  zoneid smallint(5) default NULL,
  ts timestamp(14) NOT NULL,
  pktime bigint(20) NOT NULL default '0',
  backupreason tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (id),
  KEY name (name),
  KEY charid (account_id)
) TYPE=MyISAM;

