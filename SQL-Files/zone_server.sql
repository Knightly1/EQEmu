# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'zone_server'
#

CREATE TABLE zone_server (
  name varchar(16) NOT NULL default '',
  address text NOT NULL,
  port int(11) NOT NULL default '0',
  player_count int(11) NOT NULL default '0',
  last_alive timestamp(14) NOT NULL,
  rain char(1) NOT NULL default '0',
  PRIMARY KEY  (name)
) TYPE=MyISAM;

