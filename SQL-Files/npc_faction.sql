# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'npc_faction'
#

CREATE TABLE npc_faction (
  id int(11) NOT NULL auto_increment,
  name tinytext,
  primaryfaction int(11) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

