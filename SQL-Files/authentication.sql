# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'authentication'
#

CREATE TABLE authentication (
  account_id int(11) NOT NULL default '0',
  char_name varchar(16) NOT NULL default '',
  zone_name varchar(16) NOT NULL default '',
  time timestamp(14) NOT NULL,
  ip int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (account_id)
) TYPE=MyISAM;

