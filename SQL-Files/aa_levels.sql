# MySQL-Front Dump 2.5
#
# Host: 192.168.0.9   Database: eqrc
# --------------------------------------------------------
# Server version 4.0.13-standard


#
# Table structure for table 'aa_levels'
#

DROP TABLE IF EXISTS aa_levels;
CREATE TABLE aa_levels (
  id int(10) unsigned NOT NULL auto_increment,
  aa_id int(10) unsigned NOT NULL default '0',
  ability int(10) unsigned NOT NULL default '0',
  increase_amt int(10) unsigned NOT NULL default '0',
  level tinyint(3) unsigned NOT NULL default '1',
  unknown08 int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY NewIndex (aa_id,level)
) TYPE=MyISAM;



#
# Dumping data for table 'aa_levels'
#

INSERT INTO aa_levels VALUES("2", "7", "7", "2", "1", "0");
INSERT INTO aa_levels VALUES("3", "12", "6", "2", "1", "0");
INSERT INTO aa_levels VALUES("4", "17", "5", "2", "1", "0");
INSERT INTO aa_levels VALUES("5", "22", "8", "2", "1", "0");
INSERT INTO aa_levels VALUES("6", "27", "9", "2", "1", "0");
INSERT INTO aa_levels VALUES("7", "32", "10", "2", "1", "0");
INSERT INTO aa_levels VALUES("8", "37", "46", "2", "1", "0");
INSERT INTO aa_levels VALUES("9", "42", "47", "2", "1", "0");
INSERT INTO aa_levels VALUES("10", "47", "50", "2", "1", "0");
INSERT INTO aa_levels VALUES("11", "52", "48", "2", "1", "0");
INSERT INTO aa_levels VALUES("12", "57", "49", "2", "1", "0");
INSERT INTO aa_levels VALUES("13", "62", "271", "8", "1", "0");
INSERT INTO aa_levels VALUES("14", "68", "233", "110", "1", "0");
INSERT INTO aa_levels VALUES("15", "71", "246", "110", "1", "0");
INSERT INTO aa_levels VALUES("16", "74", "269", "10", "1", "0");
INSERT INTO aa_levels VALUES("17", "77", "125", "2", "1", "2");
INSERT INTO aa_levels VALUES("18", "77", "137", "0", "2", "0");
INSERT INTO aa_levels VALUES("19", "77", "141", "1", "3", "0");
INSERT INTO aa_levels VALUES("20", "77", "125", "2", "4", "2");
INSERT INTO aa_levels VALUES("21", "77", "137", "147", "5", "0");
INSERT INTO aa_levels VALUES("22", "77", "141", "1", "6", "0");
INSERT INTO aa_levels VALUES("23", "80", "274", "3", "1", "0");
INSERT INTO aa_levels VALUES("24", "86", "128", "5", "1", "5");
INSERT INTO aa_levels VALUES("25", "86", "138", "1", "2", "0");
INSERT INTO aa_levels VALUES("26", "86", "140", "1", "3", "0");
INSERT INTO aa_levels VALUES("27", "92", "294", "2", "1", "33");
INSERT INTO aa_levels VALUES("28", "95", "235", "5", "1", "0");
INSERT INTO aa_levels VALUES("29", "107", "214", "200", "1", "0");
INSERT INTO aa_levels VALUES("30", "110", "0", "1", "1", "0");
INSERT INTO aa_levels VALUES("31", "113", "169", "15", "1", "99");
INSERT INTO aa_levels VALUES("32", "116", "181", "5", "1", "0");
INSERT INTO aa_levels VALUES("33", "119", "278", "50", "1", "0");
INSERT INTO aa_levels VALUES("34", "122", "259", "2", "1", "0");
INSERT INTO aa_levels VALUES("35", "125", "172", "2", "1", "0");
INSERT INTO aa_levels VALUES("36", "128", "203", "1", "1", "0");
INSERT INTO aa_levels VALUES("37", "198", "276", "32", "1", "0");
INSERT INTO aa_levels VALUES("38", "247", "224", "15", "1", "0");
INSERT INTO aa_levels VALUES("39", "278", "0", "1", "1", "0");
INSERT INTO aa_levels VALUES("40", "278", "15", "1", "2", "0");
INSERT INTO aa_levels VALUES("41", "279", "214", "200", "1", "0");
INSERT INTO aa_levels VALUES("42", "279", "259", "2", "2", "0");
INSERT INTO aa_levels VALUES("43", "279", "172", "2", "3", "0");
INSERT INTO aa_levels VALUES("44", "288", "257", "1", "1", "0");
INSERT INTO aa_levels VALUES("45", "292", "4", "2", "1", "0");
INSERT INTO aa_levels VALUES("46", "302", "7", "2", "1", "0");
INSERT INTO aa_levels VALUES("47", "312", "6", "2", "1", "0");
INSERT INTO aa_levels VALUES("48", "322", "5", "2", "1", "0");
INSERT INTO aa_levels VALUES("49", "332", "8", "2", "1", "0");
INSERT INTO aa_levels VALUES("50", "342", "9", "2", "1", "0");
INSERT INTO aa_levels VALUES("51", "352", "10", "2", "1", "0");
INSERT INTO aa_levels VALUES("52", "362", "46", "2", "1", "0");
INSERT INTO aa_levels VALUES("53", "372", "47", "2", "1", "0");
INSERT INTO aa_levels VALUES("54", "382", "50", "2", "1", "0");
INSERT INTO aa_levels VALUES("55", "392", "48", "2", "1", "0");
INSERT INTO aa_levels VALUES("56", "402", "49", "2", "1", "0");
INSERT INTO aa_levels VALUES("57", "412", "263", "1", "1", "0");
INSERT INTO aa_levels VALUES("58", "418", "262", "5", "1", "4294967295");
INSERT INTO aa_levels VALUES("59", "434", "125", "13", "1", "13");
INSERT INTO aa_levels VALUES("60", "434", "137", "0", "2", "0");
INSERT INTO aa_levels VALUES("61", "434", "141", "1", "3", "0");
INSERT INTO aa_levels VALUES("62", "434", "125", "13", "4", "13");
INSERT INTO aa_levels VALUES("63", "434", "137", "147", "5", "0");
INSERT INTO aa_levels VALUES("64", "434", "141", "1", "6", "0");
INSERT INTO aa_levels VALUES("65", "437", "274", "2", "1", "0");
INSERT INTO aa_levels VALUES("66", "440", "278", "2", "1", "0");
INSERT INTO aa_levels VALUES("67", "443", "169", "25", "1", "99");
INSERT INTO aa_levels VALUES("68", "449", "172", "3", "1", "0");
INSERT INTO aa_levels VALUES("69", "454", "259", "3", "1", "0");
INSERT INTO aa_levels VALUES("70", "483", "264", "540", "1", "58");
INSERT INTO aa_levels VALUES("71", "483", "264", "540", "2", "418");
INSERT INTO aa_levels VALUES("72", "504", "224", "60", "1", "0");
INSERT INTO aa_levels VALUES("73", "551", "225", "3", "1", "0");
INSERT INTO aa_levels VALUES("74", "658", "15", "1", "1", "0");
INSERT INTO aa_levels VALUES("75", "661", "0", "1", "1", "0");
INSERT INTO aa_levels VALUES("76", "672", "271", "7", "1", "0");
INSERT INTO aa_levels VALUES("77", "674", "0", "1", "1", "0");
INSERT INTO aa_levels VALUES("78", "676", "246", "175", "1", "0");
INSERT INTO aa_levels VALUES("79", "678", "221", "3", "1", "0");
INSERT INTO aa_levels VALUES("80", "686", "200", "10", "1", "0");
INSERT INTO aa_levels VALUES("81", "692", "229", "5", "1", "0");
INSERT INTO aa_levels VALUES("82", "724", "218", "1", "1", "0");
INSERT INTO aa_levels VALUES("83", "729", "280", "4", "1", "0");
INSERT INTO aa_levels VALUES("84", "734", "237", "1", "1", "0");
INSERT INTO aa_levels VALUES("85", "735", "265", "54", "1", "0");
INSERT INTO aa_levels VALUES("86", "767", "273", "3", "1", "0");
INSERT INTO aa_levels VALUES("87", "806", "249", "1", "1", "0");
INSERT INTO aa_levels VALUES("88", "978", "14", "1", "1", "0");
INSERT INTO aa_levels VALUES("89", "979", "268", "10", "1", "63");
INSERT INTO aa_levels VALUES("90", "982", "268", "10", "1", "60");
INSERT INTO aa_levels VALUES("91", "985", "268", "10", "1", "65");
INSERT INTO aa_levels VALUES("92", "988", "268", "10", "1", "64");
INSERT INTO aa_levels VALUES("93", "991", "268", "10", "1", "69");
INSERT INTO aa_levels VALUES("94", "994", "268", "10", "1", "61");
INSERT INTO aa_levels VALUES("95", "997", "331", "5", "1", "0");
INSERT INTO aa_levels VALUES("96", "1001", "262", "5", "1", "4294967295");
INSERT INTO aa_levels VALUES("97", "1006", "262", "5", "1", "7");
INSERT INTO aa_levels VALUES("98", "1006", "262", "5", "2", "8");
INSERT INTO aa_levels VALUES("99", "1006", "262", "5", "3", "9");
INSERT INTO aa_levels VALUES("100", "1006", "262", "5", "4", "10");
INSERT INTO aa_levels VALUES("101", "1006", "262", "5", "5", "11");
INSERT INTO aa_levels VALUES("102", "1021", "327", "1", "1", "0");
INSERT INTO aa_levels VALUES("103", "1026", "328", "50", "1", "0");
INSERT INTO aa_levels VALUES("104", "1031", "0", "1", "1", "0");
INSERT INTO aa_levels VALUES("105", "1041", "330", "25", "1", "0");
INSERT INTO aa_levels VALUES("106", "1041", "330", "25", "2", "1");
INSERT INTO aa_levels VALUES("107", "1041", "330", "25", "3", "2");
INSERT INTO aa_levels VALUES("108", "1041", "330", "25", "4", "3");
INSERT INTO aa_levels VALUES("109", "1041", "330", "25", "5", "28");
INSERT INTO aa_levels VALUES("110", "1041", "330", "25", "6", "36");
INSERT INTO aa_levels VALUES("111", "1053", "278", "1", "1", "0");
INSERT INTO aa_levels VALUES("112", "1061", "172", "1", "1", "0");
INSERT INTO aa_levels VALUES("113", "1066", "259", "2", "1", "0");
INSERT INTO aa_levels VALUES("114", "1071", "326", "1", "1", "0");
INSERT INTO aa_levels VALUES("115", "1072", "318", "1", "1", "0");
INSERT INTO aa_levels VALUES("116", "1083", "125", "22", "1", "22");
INSERT INTO aa_levels VALUES("117", "1083", "137", "0", "2", "0");
INSERT INTO aa_levels VALUES("118", "1083", "141", "1", "3", "0");
INSERT INTO aa_levels VALUES("119", "1083", "125", "22", "4", "22");
INSERT INTO aa_levels VALUES("120", "1083", "137", "147", "5", "0");
INSERT INTO aa_levels VALUES("121", "1083", "141", "1", "6", "0");
INSERT INTO aa_levels VALUES("122", "1086", "274", "2", "1", "0");
INSERT INTO aa_levels VALUES("123", "1093", "304", "4294967276", "1", "0");
INSERT INTO aa_levels VALUES("124", "1099", "273", "3", "1", "0");
INSERT INTO aa_levels VALUES("125", "1107", "294", "2", "1", "100");
INSERT INTO aa_levels VALUES("126", "1122", "308", "1", "1", "0");
INSERT INTO aa_levels VALUES("127", "1129", "267", "1", "1", "19");
INSERT INTO aa_levels VALUES("128", "1181", "305", "4294967276", "1", "0");
INSERT INTO aa_levels VALUES("129", "2", "4", "2", "1", "0");
INSERT INTO aa_levels VALUES("130", "83", "132", "2", "1", "2");
INSERT INTO aa_levels VALUES("131", "98", "114", "4294967291", "1", "0");
INSERT INTO aa_levels VALUES("132", "101", "265", "20", "1", "0");
INSERT INTO aa_levels VALUES("133", "104", "127", "5", "1", "5");
INSERT INTO aa_levels VALUES("134", "104", "138", "1", "2", "0");
INSERT INTO aa_levels VALUES("135", "104", "140", "1", "3", "0");
INSERT INTO aa_levels VALUES("136", "104", "143", "4000", "4", "0");
INSERT INTO aa_levels VALUES("137", "137", "127", "10", "1", "10");
INSERT INTO aa_levels VALUES("138", "137", "137", "88", "2", "0");
INSERT INTO aa_levels VALUES("139", "141", "127", "2", "1", "2");
INSERT INTO aa_levels VALUES("140", "141", "137", "0", "2", "0");
INSERT INTO aa_levels VALUES("141", "141", "138", "0", "3", "0");
INSERT INTO aa_levels VALUES("142", "141", "141", "1", "4", "0");
INSERT INTO aa_levels VALUES("143", "141", "143", "4000", "5", "0");
INSERT INTO aa_levels VALUES("144", "267", "294", "2", "1", "100");
INSERT INTO aa_levels VALUES("145", "426", "262", "10", "1", "5");
INSERT INTO aa_levels VALUES("146", "426", "262", "10", "2", "4");
INSERT INTO aa_levels VALUES("147", "446", "265", "54", "1", "0");
INSERT INTO aa_levels VALUES("148", "477", "264", "432", "1", "43");
INSERT INTO aa_levels VALUES("149", "480", "264", "432", "1", "117");
INSERT INTO aa_levels VALUES("150", "640", "294", "1", "1", "100");
INSERT INTO aa_levels VALUES("151", "691", "248", "100", "1", "0");
INSERT INTO aa_levels VALUES("152", "924", "294", "1", "1", "100");
INSERT INTO aa_levels VALUES("153", "1089", "268", "10", "1", "58");
INSERT INTO aa_levels VALUES("154", "1210", "294", "0", "1", "107");
