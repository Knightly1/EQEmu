# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'zonename'
#

CREATE TABLE zonename (
  id int(11) unsigned NOT NULL auto_increment,
  charid int(11) unsigned NOT NULL default '0',
  charname varchar(30) NOT NULL default '',
  zonename varchar(16) NOT NULL default '',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  heading float NOT NULL default '0',
  data blob NOT NULL,
  time timestamp(14) NOT NULL,
  timeofdeath timestamp(14) NOT NULL,
  PRIMARY KEY  (id),
  KEY zonename (zonename)
) TYPE=MyISAM;



#
# Dumping data for table 'zonename'
#

