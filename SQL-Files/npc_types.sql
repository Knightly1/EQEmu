# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'npc_types'
#

CREATE TABLE npc_types (
  id int(11) NOT NULL auto_increment,
  name text NOT NULL,
  lastname varchar(32) default NULL,
  level tinyint(2) unsigned NOT NULL default '0',
  race smallint(5) unsigned NOT NULL default '0',
  class tinyint(2) unsigned NOT NULL default '0',
  bodytype int(11) default NULL,
  hp int(11) NOT NULL default '0',
  gender tinyint(2) unsigned NOT NULL default '0',
  texture tinyint(2) unsigned NOT NULL default '0',
  helmtexture tinyint(2) unsigned NOT NULL default '0',
  size float NOT NULL default '0',
  hp_regen_rate int(11) unsigned NOT NULL default '0',
  mana_regen_rate int(11) unsigned NOT NULL default '0',
  loottable_id int(11) unsigned NOT NULL default '0',
  merchant_id int(11) unsigned NOT NULL default '0',
  mindmg int(10) unsigned NOT NULL default '0',
  maxdmg int(10) unsigned NOT NULL default '0',
  usedspells varchar(70) NOT NULL default '',
  npcspecialattks char(1) NOT NULL default '',
  banish int(10) unsigned NOT NULL default '0',
  aggroradius int(10) unsigned NOT NULL default '0',
  social int(10) unsigned NOT NULL default '0',
  face int(10) unsigned NOT NULL default '1',
  luclin_hairstyle int(10) unsigned NOT NULL default '1',
  luclin_haircolor int(10) unsigned NOT NULL default '1',
  luclin_eyecolor int(10) unsigned NOT NULL default '1',
  luclin_beardcolor int(10) unsigned NOT NULL default '1',
  fixedz tinyint(2) unsigned NOT NULL default '0',
  d_meele_texture1 int(10) unsigned NOT NULL default '0',
  d_meele_texture2 int(10) unsigned NOT NULL default '0',
  walkspeed float NOT NULL default '0',
  runspeed float NOT NULL default '0',
  npc_spells_id int(11) unsigned NOT NULL default '0',
  npc_faction_id int(11) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

