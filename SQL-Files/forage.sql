# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'forage'
#

CREATE TABLE forage (
  id int(11) NOT NULL auto_increment,
  zoneid int(4) NOT NULL default '0',
  Itemid int(11) NOT NULL default '0',
  level smallint(6) unsigned NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

