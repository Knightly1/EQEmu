# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'npc_faction_entries'
#

CREATE TABLE npc_faction_entries (
  npc_faction_id int(11) unsigned NOT NULL default '0',
  faction_id int(11) unsigned NOT NULL default '0',
  value int(11) NOT NULL default '0',
  npc_value TINYINT UNSIGNED DEFAULT '0' NOT NULL,
  PRIMARY KEY  (npc_faction_id,faction_id)
) TYPE=MyISAM;

