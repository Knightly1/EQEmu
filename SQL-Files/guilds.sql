# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'guilds'
#

CREATE TABLE guilds (
  id int(11) NOT NULL auto_increment,
  eqid smallint(4) NOT NULL default '0',
  name varchar(32) NOT NULL default '',
  leader int(11) NOT NULL default '0',
  motd text NOT NULL,
  rank0title varchar(100) NOT NULL default '',
  rank1title varchar(100) NOT NULL default '',
  rank1 varchar(8) NOT NULL default '',
  rank2title varchar(100) NOT NULL default '',
  rank2 varchar(8) NOT NULL default '',
  rank3title varchar(100) NOT NULL default '',
  rank3 varchar(8) NOT NULL default '',
  rank4title varchar(100) NOT NULL default '',
  rank4 varchar(8) NOT NULL default '',
  rank5title varchar(100) NOT NULL default '',
  rank5 varchar(8) NOT NULL default '',
  minstatus smallint(5) NOT NULL default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY eqid (eqid),
  UNIQUE KEY name (name),
  UNIQUE KEY leader (leader)
) TYPE=MyISAM;

