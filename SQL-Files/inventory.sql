# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eq
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'inventory'
#

CREATE TABLE inventory (
  charid int(10) unsigned default '0',
  slotid smallint(6) unsigned default '0',
  itemid mediumint(7) unsigned default '0',
  charges tinyint(3) unsigned default '0',
  UNIQUE KEY slotid (slotid,charid)
) TYPE=MyISAM;
ALTER TABLE `inventory` ADD `color` INT(11)  UNSIGNED DEFAULT "0" NOT NULL;