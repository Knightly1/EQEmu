# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'start_zones'
#

CREATE TABLE start_zones (
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  zone_id int(4) NOT NULL default '0',
  player_deity int(4) NOT NULL default '0',
  player_race int(4) NOT NULL default '0',
  player_class int(2) NOT NULL default '0',
  player_choice int(2) NOT NULL default '0'
) TYPE=MyISAM;

