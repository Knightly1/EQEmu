# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'object_contents'
#

CREATE TABLE object_contents (
  zoneid int(11) unsigned NOT NULL default '0',
  parentid int(11) unsigned NOT NULL default '0',
  bagidx int(11) unsigned NOT NULL default '0',
  itemid int(11) unsigned NOT NULL default '0',
  charges tinyint(3) NOT NULL default '0',
  droptime datetime NOT NULL default '0000-00-00 00:00:00',
  PRIMARY KEY  (parentid,bagidx)
) TYPE=MyISAM;

