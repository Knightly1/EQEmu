# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eqrc
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'merchantlist_temp'
#

CREATE TABLE merchantlist_temp (
  npcid int(10) unsigned NOT NULL default '0',
  slot tinyint(3) unsigned NOT NULL default '0',
  itemid int(10) unsigned NOT NULL default '0',
  charges int(10) unsigned NOT NULL default '1',
  UNIQUE KEY merchantid (npcid,slot)
) TYPE=MyISAM;

