# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'variables'
#

CREATE TABLE variables (
  varname varchar(25) NOT NULL default '',
  value text NOT NULL,
  ts timestamp(14) NOT NULL,
  PRIMARY KEY  (varname)
) TYPE=MyISAM;

