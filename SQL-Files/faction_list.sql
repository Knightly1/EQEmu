# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'faction_list'
#

CREATE TABLE faction_list (
  id int(11) NOT NULL auto_increment,
  name varchar(50) NOT NULL default '',
  base smallint(6) NOT NULL default '0',
  mod_c1 smallint(6) NOT NULL default '0',
  mod_c2 smallint(6) NOT NULL default '0',
  mod_c3 smallint(6) NOT NULL default '0',
  mod_c4 smallint(6) NOT NULL default '0',
  mod_c5 smallint(6) NOT NULL default '0',
  mod_c6 smallint(6) NOT NULL default '0',
  mod_c7 smallint(6) NOT NULL default '0',
  mod_c8 smallint(6) NOT NULL default '0',
  mod_c9 smallint(6) NOT NULL default '0',
  mod_c10 smallint(6) NOT NULL default '0',
  mod_c11 smallint(6) NOT NULL default '0',
  mod_c12 smallint(6) NOT NULL default '0',
  mod_c13 smallint(6) NOT NULL default '0',
  mod_c14 smallint(6) NOT NULL default '0',
  mod_c15 smallint(6) NOT NULL default '0',
  mod_r1 smallint(6) NOT NULL default '0',
  mod_r2 smallint(6) NOT NULL default '0',
  mod_r3 smallint(6) NOT NULL default '0',
  mod_r4 smallint(6) NOT NULL default '0',
  mod_r5 smallint(6) NOT NULL default '0',
  mod_r6 smallint(6) NOT NULL default '0',
  mod_r7 smallint(6) NOT NULL default '0',
  mod_r8 smallint(6) NOT NULL default '0',
  mod_r9 smallint(6) NOT NULL default '0',
  mod_r10 smallint(6) NOT NULL default '0',
  mod_r11 smallint(6) NOT NULL default '0',
  mod_r12 smallint(6) NOT NULL default '0',
  mod_r14 smallint(6) NOT NULL default '0',
  mod_r60 smallint(6) NOT NULL default '0',
  mod_r75 smallint(6) NOT NULL default '0',
  mod_r108 smallint(6) NOT NULL default '0',
  mod_r120 smallint(6) NOT NULL default '0',
  mod_r128 smallint(6) NOT NULL default '0',
  mod_r130 smallint(6) NOT NULL default '0',
  mod_r161 smallint(6) NOT NULL default '0',
  mod_d140 smallint(6) NOT NULL default '0',
  mod_d201 smallint(6) NOT NULL default '0',
  mod_d202 smallint(6) NOT NULL default '0',
  mod_d203 smallint(6) NOT NULL default '0',
  mod_d204 smallint(6) NOT NULL default '0',
  mod_d205 smallint(6) NOT NULL default '0',
  mod_d206 smallint(6) NOT NULL default '0',
  mod_d207 smallint(6) NOT NULL default '0',
  mod_d208 smallint(6) NOT NULL default '0',
  mod_d209 smallint(6) NOT NULL default '0',
  mod_d210 smallint(6) NOT NULL default '0',
  mod_d211 smallint(6) NOT NULL default '0',
  mod_d212 smallint(6) NOT NULL default '0',
  mod_d213 smallint(6) NOT NULL default '0',
  mod_d214 smallint(6) NOT NULL default '0',
  mod_d215 smallint(6) NOT NULL default '0',
  mod_d216 smallint(6) NOT NULL default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY id (id),
  UNIQUE KEY name (name)
) TYPE=MyISAM;

