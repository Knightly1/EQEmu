# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'loottable_entries'
#

CREATE TABLE loottable_entries (
  loottable_id int(11) unsigned NOT NULL default '0',
  lootdrop_id int(11) unsigned NOT NULL default '0',
  multiplier tinyint(2) unsigned NOT NULL default '1',
  probability tinyint(2) unsigned NOT NULL default '100',
  PRIMARY KEY  (loottable_id,lootdrop_id)
) TYPE=MyISAM;

