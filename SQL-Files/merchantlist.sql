# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'merchantlist'
#

CREATE TABLE merchantlist (
  merchantid int(11) NOT NULL default '0',
  slot int(11) NOT NULL auto_increment,
  item int(11) NOT NULL default '0',
  PRIMARY KEY  (merchantid,slot)
) TYPE=MyISAM;

