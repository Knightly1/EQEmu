# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'lootdrop_entries'
#

CREATE TABLE lootdrop_entries (
  lootdrop_id int(11) unsigned NOT NULL default '0',
  item_id int(11) NOT NULL default '0',
  item_charges tinyint(2) NOT NULL default '1',
  equip_item tinyint(2) unsigned NOT NULL default '0',
  chance tinyint(2) unsigned NOT NULL default '1',
  PRIMARY KEY  (lootdrop_id,item_id)
) TYPE=MyISAM;

