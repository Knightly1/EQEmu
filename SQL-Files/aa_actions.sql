# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eqrc
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'aa_actions'
#

DROP TABLE IF EXISTS aa_actions;
CREATE TABLE aa_actions (
  aaid mediumint(8) unsigned NOT NULL default '0',
  rank tinyint(3) unsigned NOT NULL default '0',
  reuse_time mediumint(8) unsigned NOT NULL default '0',
  spell_id mediumint(8) unsigned NOT NULL default '0',
  target tinyint(3) unsigned NOT NULL default '0',
  nonspell_action tinyint(3) unsigned NOT NULL default '0',
  nonspell_mana mediumint(8) unsigned NOT NULL default '0',
  nonspell_duration mediumint(8) unsigned NOT NULL default '0',
  redux_aa mediumint(8) unsigned NOT NULL default '0',
  redux_rate tinyint(4) NOT NULL default '0',
  PRIMARY KEY  (aaid,rank)
) TYPE=MyISAM;



#
# Dumping data for table 'aa_actions'
#

INSERT INTO aa_actions VALUES("35", "0", "4320", "0", "1", "1", "0", "0", "264", "10");
INSERT INTO aa_actions VALUES("36", "0", "64800", "1", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("37", "0", "7", "1", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("38", "0", "900", "100", "1", "6", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("39", "0", "8640", "0", "2", "0", "0", "0", "154", "10");
INSERT INTO aa_actions VALUES("40", "0", "4320", "2500", "2", "0", "0", "0", "155", "10");
INSERT INTO aa_actions VALUES("40", "1", "4320", "2500", "2", "0", "0", "0", "155", "10");
INSERT INTO aa_actions VALUES("40", "2", "4320", "2500", "2", "0", "0", "0", "155", "10");
INSERT INTO aa_actions VALUES("41", "0", "1800", "0", "2", "0", "0", "0", "156", "10");
INSERT INTO aa_actions VALUES("43", "0", "4320", "100", "1", "0", "0", "0", "159", "10");
INSERT INTO aa_actions VALUES("46", "0", "4320", "6000", "2", "7", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("46", "1", "4320", "6000", "2", "7", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("46", "2", "4320", "6000", "2", "7", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("38", "1", "900", "100", "1", "6", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("38", "2", "900", "100", "1", "6", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("47", "0", "180", "2500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("50", "0", "7200", "100", "1", "0", "0", "0", "158", "10");
INSERT INTO aa_actions VALUES("52", "1", "480", "16000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("52", "0", "480", "16000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("53", "0", "4320", "100", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("57", "0", "8640", "2000", "1", "0", "0", "0", "157", "10");
INSERT INTO aa_actions VALUES("58", "0", "2160", "1", "5", "0", "0", "0", "161", "10");
INSERT INTO aa_actions VALUES("60", "0", "4320", "2000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("61", "0", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("61", "1", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("61", "2", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("62", "0", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("62", "1", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("62", "2", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("63", "0", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("63", "1", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("63", "2", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("64", "0", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("64", "1", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("64", "2", "900", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("66", "0", "4320", "2500", "1", "0", "0", "0", "162", "10");
INSERT INTO aa_actions VALUES("66", "1", "4320", "2500", "1", "0", "0", "0", "162", "10");
INSERT INTO aa_actions VALUES("66", "2", "4320", "2500", "1", "0", "0", "0", "162", "10");
INSERT INTO aa_actions VALUES("68", "0", "8640", "6000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("69", "0", "4320", "3000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("70", "0", "4320", "3000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("71", "0", "7", "3000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("72", "0", "4320", "5000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("73", "0", "30", "1500", "2", "0", "0", "0", "196", "24");
INSERT INTO aa_actions VALUES("76", "0", "4320", "0", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("77", "0", "1", "2000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("80", "0", "7", "1", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("85", "0", "1", "2000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("86", "0", "4320", "1", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("87", "0", "4320", "1", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("98", "0", "4320", "0", "1", "0", "0", "0", "165", "10");
INSERT INTO aa_actions VALUES("102", "0", "4320", "0", "1", "0", "0", "0", "166", "10");
INSERT INTO aa_actions VALUES("107", "0", "4320", "0", "1", "0", "0", "0", "167", "10");
INSERT INTO aa_actions VALUES("109", "0", "6000", "0", "1", "0", "0", "0", "164", "10");
INSERT INTO aa_actions VALUES("110", "0", "900", "0", "1", "0", "0", "0", "163", "10");
INSERT INTO aa_actions VALUES("111", "0", "2160", "0", "4", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("111", "1", "2160", "0", "4", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("111", "2", "2160", "0", "4", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("117", "0", "4320", "2000", "2", "0", "0", "0", "160", "10");
INSERT INTO aa_actions VALUES("126", "0", "300", "3000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("127", "0", "720", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("128", "0", "900", "5000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("153", "0", "180", "3500", "1", "0", "0", "0", "256", "10");
INSERT INTO aa_actions VALUES("153", "1", "180", "3500", "1", "0", "0", "0", "256", "10");
INSERT INTO aa_actions VALUES("153", "2", "180", "3500", "1", "0", "0", "0", "256", "10");
INSERT INTO aa_actions VALUES("169", "0", "180", "750", "4", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("169", "1", "180", "750", "4", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("169", "2", "180", "750", "4", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("170", "0", "240", "1000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("170", "1", "240", "1000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("170", "2", "240", "1000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("171", "0", "120", "2000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("171", "1", "120", "2000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("171", "2", "120", "2000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("172", "0", "480", "10000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("173", "0", "600", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("173", "1", "600", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("173", "2", "600", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("174", "0", "540", "500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("174", "1", "540", "500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("174", "2", "540", "500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("175", "0", "540", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("175", "1", "540", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("175", "2", "540", "8000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("176", "0", "0", "4000", "5", "10", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("176", "1", "0", "4000", "5", "10", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("177", "0", "720", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("177", "1", "720", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("177", "2", "720", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("180", "0", "2160", "0", "1", "0", "0", "0", "282", "10");
INSERT INTO aa_actions VALUES("180", "1", "2160", "0", "1", "0", "0", "0", "282", "10");
INSERT INTO aa_actions VALUES("180", "2", "2160", "0", "1", "0", "0", "0", "282", "10");
INSERT INTO aa_actions VALUES("184", "0", "900", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("184", "1", "900", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("184", "2", "900", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("185", "0", "1320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("185", "1", "1320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("185", "2", "1320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("199", "0", "18", "0", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("207", "0", "1320", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("207", "1", "1320", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("207", "2", "1320", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("208", "0", "900", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("208", "1", "900", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("208", "2", "900", "6500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("212", "0", "0", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("217", "0", "0", "0", "1", "13", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("219", "0", "5", "1750", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("226", "0", "8640", "6000", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("245", "0", "4320", "0", "1", "11", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("254", "0", "7200", "3000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("247", "0", "60", "0", "2", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("254", "1", "7200", "3000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("254", "2", "7200", "3000", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("255", "0", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("255", "1", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("255", "2", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("255", "3", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("255", "4", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("257", "0", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("257", "1", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("257", "2", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("257", "3", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("257", "4", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("261", "0", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("261", "1", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("261", "2", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("265", "0", "900", "0", "1", "5", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("276", "0", "3600", "0", "1", "4", "0", "10000", "0", "0");
INSERT INTO aa_actions VALUES("277", "0", "1800", "500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("277", "1", "1800", "500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("277", "2", "1800", "500", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("285", "0", "180", "0", "1", "0", "0", "0", "256", "10");
INSERT INTO aa_actions VALUES("285", "1", "180", "0", "1", "0", "0", "0", "256", "10");
INSERT INTO aa_actions VALUES("285", "2", "180", "0", "1", "0", "0", "0", "256", "10");
INSERT INTO aa_actions VALUES("286", "0", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("289", "0", "4320", "0", "1", "2", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("290", "0", "4320", "0", "1", "3", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("298", "0", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("298", "1", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("298", "2", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("298", "3", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("298", "4", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("300", "0", "3600", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("300", "1", "3600", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("300", "2", "3600", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("303", "0", "60", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("304", "0", "60", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("305", "0", "69", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("307", "0", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("307", "1", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("307", "2", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("307", "3", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("307", "4", "1800", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("308", "0", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("308", "1", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("308", "2", "4320", "0", "1", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("89", "0", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("89", "1", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("89", "2", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("89", "3", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("89", "4", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("89", "5", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("68", "1", "0", "0", "0", "0", "0", "0", "0", "0");
INSERT INTO aa_actions VALUES("276", "1", "3600", "0", "1", "4", "0", "10000", "0", "0");
INSERT INTO aa_actions VALUES("276", "2", "3600", "0", "1", "4", "0", "10000", "0", "0");
