# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'altadv_vars'
#

CREATE TABLE altadv_vars (
  skill_id int(11) default NULL,
  name varchar(128) default NULL,
  cost int(11) default NULL,
  max_level int(11) default NULL
) TYPE=MyISAM;

