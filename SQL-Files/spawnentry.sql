# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'spawnentry'
#

CREATE TABLE spawnentry (
  spawngroupID int(11) NOT NULL default '0',
  npcID int(11) NOT NULL default '0',
  chance smallint(4) NOT NULL default '0',
  PRIMARY KEY  (spawngroupID,npcID)
) TYPE=MyISAM;

