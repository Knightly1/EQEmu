# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'zone_state_dump'
#

CREATE TABLE zone_state_dump (
  zonename varchar(16) NOT NULL default '',
  spawn2_count int(10) unsigned NOT NULL default '0',
  npc_count int(10) unsigned NOT NULL default '0',
  npcloot_count int(10) unsigned NOT NULL default '0',
  gmspawntype_count int(10) unsigned NOT NULL default '0',
  spawn2 mediumblob,
  npcs mediumblob,
  npc_loot mediumblob,
  gmspawntype mediumblob,
  time timestamp(14) NOT NULL,
  PRIMARY KEY  (zonename)
) TYPE=MyISAM;

