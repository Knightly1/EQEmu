# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'zoneserver_auth'
#

CREATE TABLE zoneserver_auth (
  host varchar(30) NOT NULL default '',
  note text,
  PRIMARY KEY  (host)
) TYPE=MyISAM;

