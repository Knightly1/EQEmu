# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'account'
#

CREATE TABLE account (
  id int(11) NOT NULL auto_increment,
  name varchar(30) NOT NULL default '',
  charname varchar(64) NOT NULL default '',
  packencrypt blob NOT NULL,
  password varchar(50) NOT NULL default '',
  status int(5) NOT NULL default '0',
  lsaccount_id int(11) unsigned default NULL,
  gmspeed tinyint(3) unsigned NOT NULL default '0',
  sharedbank blob,
  PRIMARY KEY  (id),
  UNIQUE KEY name (name),
  UNIQUE KEY lsaccount_id (lsaccount_id)
) TYPE=MyISAM;

