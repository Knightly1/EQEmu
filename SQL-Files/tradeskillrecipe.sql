# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'tradeskillrecipe'
#

CREATE TABLE tradeskillrecipe (
  id int(11) NOT NULL auto_increment,
  tradeskill smallint(6) NOT NULL default '0',
  skillneeded smallint(6) NOT NULL default '0',
  trivial smallint(6) NOT NULL default '0',
  product smallint(6) NOT NULL default '0',
  product2 smallint(6) NOT NULL default '0',
  failproduct smallint(6) NOT NULL default '0',
  productcount smallint(6) NOT NULL default '0',
  i1 smallint(6) NOT NULL default '0',
  i2 smallint(6) NOT NULL default '0',
  i3 smallint(6) NOT NULL default '0',
  i4 smallint(6) NOT NULL default '0',
  i5 smallint(6) NOT NULL default '0',
  i6 smallint(6) NOT NULL default '0',
  i7 smallint(6) NOT NULL default '0',
  i8 smallint(6) NOT NULL default '0',
  i9 smallint(6) NOT NULL default '0',
  i10 smallint(6) NOT NULL default '0',
  notes text,
  PRIMARY KEY  (id),
  UNIQUE KEY id (id)
) TYPE=MyISAM;

