# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'npc_spells_entries'
#

CREATE TABLE npc_spells_entries (
  id int(11) unsigned NOT NULL auto_increment,
  npc_spells_id int(11) NOT NULL default '0',
  spellid smallint(5) NOT NULL default '0',
  type smallint(5) unsigned NOT NULL default '0',
  minlevel tinyint(3) unsigned NOT NULL default '0',
  maxlevel tinyint(3) unsigned NOT NULL default '255',
  manacost smallint(5) NOT NULL default '-1',
  recast_delay int(11) NOT NULL default '-1',
  priority smallint(5) NOT NULL default '0',
  PRIMARY KEY  (id),
  KEY npc_spells_id (npc_spells_id)
) TYPE=MyISAM;

