# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'eventlog'
#

CREATE TABLE eventlog (
  id bigint(10) unsigned NOT NULL auto_increment,
  accountname varchar(30) NOT NULL default '',
  accountid int(10) unsigned default '0',
  status int(5) NOT NULL default '0',
  charname varchar(64) NOT NULL default '',
  target varchar(64) default 'None',
  time timestamp(14) NOT NULL,
  descriptiontype varchar(50) NOT NULL default '',
  description text NOT NULL,
  event_nid int(11) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

