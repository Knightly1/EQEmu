# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'object'
#

CREATE TABLE object (
  id int(11) NOT NULL auto_increment,
  zoneid int(11) unsigned NOT NULL default '0',
  xpos float NOT NULL default '0',
  ypos float NOT NULL default '0',
  zpos float NOT NULL default '0',
  heading float NOT NULL default '0',
  itemid int(11) NOT NULL default '0',
  charges tinyint(3) unsigned NOT NULL default '0',
  objectname varchar(16) NOT NULL default '',
  type int(11) NOT NULL default '0',
  icon int(11) NOT NULL default '0',
  linked_list_addr_01 int(11) NOT NULL default '0',
  linked_list_addr_02 int(11) NOT NULL default '0',
  unknown08 mediumint(5) NOT NULL default '0',
  unknown10 mediumint(5) NOT NULL default '0',
  unknown20 int(11) NOT NULL default '0',
  unknown24 int(11) NOT NULL default '0',
  unknown60 int(11) NOT NULL default '0',
  unknown64 int(11) NOT NULL default '0',
  unknown68 int(11) NOT NULL default '0',
  unknown72 int(11) NOT NULL default '0',
  unknown76 int(11) NOT NULL default '0',
  unknown84 int(11) NOT NULL default '0',
  unknown88 int(11) NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;

