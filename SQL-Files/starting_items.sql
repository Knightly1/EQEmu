# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'starting_items'
#

CREATE TABLE starting_items (
  id int(11) unsigned NOT NULL auto_increment,
  race int(11) NOT NULL default '0',
  class int(11) NOT NULL default '0',
  deityid int(11) NOT NULL default '0',
  zoneid int(11) NOT NULL default '0',
  itemid int(11) NOT NULL default '0',
  item_charges tinyint(3) unsigned NOT NULL default '1',
  gm tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (id,race)
) TYPE=MyISAM;

