# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'loottable'
#

CREATE TABLE loottable (
  id int(11) unsigned NOT NULL auto_increment,
  name varchar(255) NOT NULL default '',
  mincash int(11) unsigned NOT NULL default '0',
  maxcash int(11) unsigned NOT NULL default '0',
  avgcoin smallint(4) unsigned NOT NULL default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY name (name)
) TYPE=MyISAM;

