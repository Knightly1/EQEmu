# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'zonepoints_raw'
#

CREATE TABLE zonepoints_raw (
  id int(11) NOT NULL auto_increment,
  zone varchar(16) NOT NULL default '',
  zoneline blob,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

