# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'aa_timers'
#

CREATE TABLE aa_timers (
  charid int(12) unsigned NOT NULL default '0',
  ability smallint(5) unsigned NOT NULL default '0',
  begin int(10) unsigned NOT NULL default '0',
  end int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (charid,ability)
) TYPE=MyISAM;

