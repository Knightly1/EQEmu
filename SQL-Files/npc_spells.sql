# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'npc_spells'
#

CREATE TABLE npc_spells (
  id int(11) unsigned NOT NULL auto_increment,
  name tinytext,
  parent_list int(11) unsigned NOT NULL default '0',
  attack_proc smallint(5) NOT NULL default '-1',
  proc_chance tinyint(3) NOT NULL default '3',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

