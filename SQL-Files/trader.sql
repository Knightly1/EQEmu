# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'trader'
#

CREATE TABLE trader (
  char_id int(10) unsigned NOT NULL default '0',
  item_id int(10) unsigned NOT NULL default '0',
  item_cost int(10) unsigned NOT NULL default '0',
  slot_id tinyint(3) unsigned NOT NULL default '0',
  PRIMARY KEY  (char_id,slot_id)
) TYPE=MyISAM;

