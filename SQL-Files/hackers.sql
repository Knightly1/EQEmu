# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'hackers'
#

CREATE TABLE hackers (
  id int(4) NOT NULL auto_increment,
  account text NOT NULL,
  name text NOT NULL,
  hacked text NOT NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;

