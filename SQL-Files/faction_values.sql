# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'faction_values'
#

CREATE TABLE faction_values (
  char_id int(4) NOT NULL default '0',
  faction_id int(4) NOT NULL default '0',
  current_value smallint(6) NOT NULL default '0',
  PRIMARY KEY  (char_id,faction_id)
) TYPE=MyISAM;

