# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'sharedbank'
#

CREATE TABLE sharedbank (
  acctid int(10) unsigned default '0',
  slotid smallint(6) unsigned default '0',
  itemid mediumint(7) unsigned default '0',
  charges tinyint(3) unsigned default '0',
  color int(10) unsigned default '0',
  UNIQUE KEY account (acctid,slotid)
) TYPE=MyISAM;

