# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'petitions'
#

CREATE TABLE petitions (
  dib int(10) unsigned NOT NULL auto_increment,
  petid int(10) unsigned NOT NULL default '0',
  charname varchar(32) NOT NULL default '',
  accountname varchar(32) NOT NULL default '',
  lastgm varchar(32) NOT NULL default '',
  petitiontext text NOT NULL,
  gmtext text,
  zone varchar(32) NOT NULL default '',
  urgency int(11) NOT NULL default '0',
  charclass int(11) NOT NULL default '0',
  charrace int(11) NOT NULL default '0',
  charlevel int(11) NOT NULL default '0',
  checkouts int(11) NOT NULL default '0',
  unavailables int(11) NOT NULL default '0',
  ischeckedout tinyint(4) NOT NULL default '0',
  senttime bigint(11) NOT NULL default '0',
  PRIMARY KEY  (dib),
  KEY petid (petid)
) TYPE=MyISAM COMMENT='Table for Petitions';

