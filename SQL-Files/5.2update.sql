
#
# Table structure for table 'bugs'
#

DROP TABLE IF EXISTS bugs;
CREATE TABLE bugs (
  id int(11) NOT NULL auto_increment,
  type varchar(64) default '0',
  name varchar(96) default NULL,
  account_id int(11) default NULL,
  bugtext text,
  flag varchar(64) default NULL,
  ui varchar(128) default NULL,
  x float default NULL,
  y float default NULL,
  z float default NULL,
  heading float default NULL,
  PRIMARY KEY  (id)
) TYPE=MyISAM;


#
# Table structure for table 'eventlog'
#

DROP TABLE IF EXISTS eventlog;
CREATE TABLE eventlog (
  id int(10) unsigned NOT NULL auto_increment,
  accountname varchar(30) NOT NULL default '',
  accountid int(10) unsigned default '0',
  status int(5) NOT NULL default '0',
  charname varchar(64) NOT NULL default '',
  target varchar(64) default 'None',
  time timestamp(14) NOT NULL,
  descriptiontype varchar(50) NOT NULL default '',
  description text NOT NULL,
  event_nid int(10) unsigned NOT NULL default '0',
  PRIMARY KEY  (id)
) TYPE=MyISAM;


#
# Table structure for table 'inventory'
#

DROP TABLE IF EXISTS inventory;
CREATE TABLE inventory (
  charid int(10) unsigned default '0',
  slotid smallint(6) unsigned default '0',
  itemid mediumint(7) unsigned default '0',
  charges tinyint(3) unsigned default '0',
  color int(11) unsigned NOT NULL default '0',
  UNIQUE KEY slotid (slotid,charid)
) TYPE=MyISAM;




#
# Table structure for table 'zone_points'
#

DROP TABLE IF EXISTS zone_points;
CREATE TABLE zone_points (
  id int(11) NOT NULL auto_increment,
  zone char(16) NOT NULL default '',
  number tinyint(3) unsigned NOT NULL default '1',
  x float NOT NULL default '0',
  y float NOT NULL default '0',
  z float NOT NULL default '0',
  target_x float NOT NULL default '0',
  target_y float NOT NULL default '0',
  target_z float NOT NULL default '0',
  target_heading float NOT NULL default '0',
  target_zone char(16) NOT NULL default '',
  heading float NOT NULL default '0',
  keep_x tinyint(1) NOT NULL default '0',
  keep_y tinyint(1) NOT NULL default '0',
  zoneinst smallint(5) unsigned default '0',
  PRIMARY KEY  (id),
  UNIQUE KEY NewIndex (number,zone)
) TYPE=MyISAM;



#
# Dumping data for table 'zone_points'
#

INSERT INTO zone_points VALUES("1", "qeynos", "1", "-2", "-155", "3", "464", "-440", "351", "0", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("476", "qeynos2", "1", "0", "0", "0", "-714", "-168", "-10", "999", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("3", "qeynos", "2", "30", "-30", "3", "590", "-76", "351", "0", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("477", "qeynos2", "2", "0", "0", "0", "-153", "-30", "9", "250", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("472", "qeynos", "3", "0", "0", "0", "-64", "230", "-80", "129", "feerrott", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("484", "qcat", "1", "0", "0", "0", "127", "-189", "-90", "999", "qeynos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("7", "qeynos2", "3", "1061", "-48", "-34.7", "0", "0", "3", "0", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("485", "qcat", "2", "0", "0", "0", "-608", "-147", "-24", "999", "qeynos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("478", "qeynos2", "4", "0", "0", "0", "141", "-175", "-80", "384", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("700", "qeytoqrg", "1", "0", "0", "0", "999999", "1395", "999999", "999", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("701", "qeytoqrg", "2", "0", "0", "0", "45", "36", "999999", "999", "qey2hh1", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("12", "qrg", "1", "98", "5182", "-2.8", "114", "-610", "3", "0", "qeytoqrg", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("703", "qeytoqrg", "3", "0", "0", "0", "-63", "231", "-52", "999", "qey2hh1", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("799", "qey2hh1", "1", "0", "0", "0", "-2449", "1238", "-2", "999", "qeytoqrg", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("704", "qeytoqrg", "4", "0", "0", "0", "114", "-611", "5", "999", "qrg", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("802", "blackburrow", "1", "0", "0", "0", "-1155", "3431", "999999", "999", "qeytoqrg", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("17", "qey2hh1", "2", "3242", "627", "0", "-15910", "646", "0", "0", "northkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("18", "northkarana", "1", "-15910", "646", "0", "3242", "627", "0", "0", "qey2hh1", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("19", "northkarana", "2", "1066", "12", "-3.35", "-3096", "6", "-33.5", "0", "eastkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("20", "eastkarana", "1", "-3096", "6", "-3.35", "1066", "12", "-33.5", "0", "northkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("21", "northkarana", "3", "899", "2880", "-3", "1212", "-4487", "-30", "0", "southkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("740", "southkarana", "1", "0", "0", "0", "1151", "4374", "1", "999", "lakerathe", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("23", "eastkarana", "2", "-388", "-1125", "232", "-2014", "3618", "8", "0", "beholder", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("24", "beholder", "1", "-2092", "3565", "19", "-412", "-1172", "30.3", "0", "eastkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("25", "eastkarana", "3", "111", "821", "4", "-8336", "-3080", "693", "0", "highpass", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("627", "highpass", "1", "0", "0", "0", "-8338", "-3097", "690", "999", "eastkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("27", "southkarana", "2", "56", "-101", "3", "926", "-3153", "-8.8", "0", "paw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("970", "paw", "1", "0", "0", "0", "931", "-3143", "-11", "999", "southkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("29", "southkarana", "3", "1153", "4351", "4", "1180", "-8520", "3.3", "0", "lakerathe", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("806", "lakerathe", "1", "0", "0", "0", "3036", "3411", "-2", "999", "rathemtn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("31", "beholder", "2", "-1846", "907", "-4.7", "-1855", "902", "4", "0", "runnyeye", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("906", "runnyeye", "1", "0", "0", "0", "-1844", "903", "4", "999", "beholder", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("907", "runnyeye", "2", "0", "0", "0", "1431", "-830", "-9", "999", "misty", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("675", "misty", "1", "0", "0", "0", "152", "228", "6", "999", "runnyeye", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("628", "highpass", "2", "0", "0", "0", "4892", "557", "691", "999", "kithicor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("648", "kithicor", "1", "0", "0", "0", "91", "-979", "6", "999", "highpass", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("37", "highpass", "3", "92", "63", "4", "109", "63", "3.7", "0", "highkeep", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("38", "highkeep", "1", "-105", "64", "4.4", "86", "-89", "3.7", "0", "highpass", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("649", "kithicor", "2", "0", "0", "0", "-282", "-369", "5", "999", "rivervale", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("40", "commons", "1", "-1092", "1310", "-4", "4203", "918", "48.3", "0", "kithicor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("41", "kithicor", "3", "-279", "-366", "3.7", "3809", "2009", "466.7", "0", "rivervale", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("42", "rivervale", "1", "3824", "2014", "466", "156", "-49", "3.7", "0", "kithicor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("43", "rivervale", "2", "-2575", "405", "-0.8", "152", "-63", "2", "0", "misty", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("676", "misty", "2", "0", "0", "0", "78", "-79", "5", "999", "rivervale", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("45", "commons", "2", "4974", "12", "-72.2", "0", "0", "3", "0", "ecommons", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("575", "ecommons", "1", "0", "0", "0", "2903", "2603", "1", "999", "nro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("47", "commons", "3", "34", "-75", "4", "576", "-1146", "-38.5", "0", "befallen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("801", "befallen", "1", "0", "0", "0", "596", "-1162", "-41", "999", "commons", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("49", "ecommons", "2", "625", "9", "-24", "1581", "-6", "-51", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("50", "freportw", "1", "-1572", "5", "-50.7", "1180", "100", "-24", "0", "ecommons", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("51", "ecommons", "3", "-613", "-2686", "-17", "612", "1524", "-18", "0", "nektulos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("52", "nektulos", "1", "612", "1524", "-17", "-613", "-2686", "-18", "0", "ecommons", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("53", "ecommons", "4", "2810", "2616", "3.7", "-1266", "-3089", "3", "0", "nro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("686", "nro", "1", "0", "0", "0", "999999", "-1296", "-56", "999", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("55", "freportw", "2", "-342", "445", "-24", "-80", "-935", "-21", "0", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("56", "freporte", "1", "-935", "-80", "-21", "449", "-344", "-24", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("57", "freportw", "3", "-84", "-382", "-24", "586", "-352", "999999", "999", "freportn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("58", "freportn", "1", "487", "-411", "-10", "-381", "-83", "-24", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("59", "freporte", "2", "410", "3962", "-24", "-620", "-1587", "-52", "0", "nro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("687", "nro", "2", "0", "0", "0", "999999", "2539", "999999", "999", "oasis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("803", "blackburrow", "2", "0", "0", "0", "-530", "-3058", "-109", "999", "everfrost", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("590", "everfrost", "1", "0", "0", "0", "-73", "-649", "5", "999", "halas", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("591", "everfrost", "2", "0", "0", "0", "-340", "95", "4", "999", "blackburrow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("905", "permafrost", "1", "0", "0", "0", "-7073", "2022", "-55", "999", "everfrost", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("592", "everfrost", "3", "0", "0", "0", "100", "-60", "4", "999", "permafrost", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("397", "halas", "1", "0", "0", "0", "380", "3684", "5", "999", "everfrost", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("67", "nektulos", "2", "156", "41", "32", "2268", "-1108", "3.8", "0", "neriaka", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("413", "neriaka", "1", "0", "0", "0", "999999", "-242", "999999", "999", "neriakb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("69", "nektulos", "3", "-172", "-2107", "-15", "316", "3044", "-14", "0", "lavastorm", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("657", "lavastorm", "1", "0", "0", "0", "856", "-15", "5", "999", "najena", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("419", "neriakc", "1", "0", "0", "0", "999999", "200", "999999", "999", "neriakb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("416", "neriakb", "1", "0", "0", "0", "999999", "-247", "999999", "999", "neriaka", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("417", "neriakb", "2", "0", "0", "0", "-361", "999999", "999999", "999", "neriaka", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("414", "neriaka", "2", "0", "0", "0", "-367", "999999", "999999", "999", "neriakb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("75", "lavastorm", "2", "0", "0", "3", "330", "1351", "148", "0", "soltemple", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("752", "soltemple", "1", "0", "0", "0", "331", "1344", "149", "999", "lavastorm", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("77", "lavastorm", "3", "-523", "-455", "73.8", "485", "909", "54", "0", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("910", "soldunga", "1", "0", "0", "0", "227.54", "783.95", "127.36", "999", "lavastorm", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("79", "lavastorm", "4", "-572", "-164", "2", "232", "797", "129", "0", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("955", "soldungb", "1", "0", "0", "0", "484.37", "911.18", "57.44", "999", "lavastorm", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("81", "lavastorm", "5", "862", "-16", "3.7", "-1089", "942", "15", "0", "najena", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("889", "najena", "1", "0", "0", "0", "-1058", "-936", "13", "999", "lavastorm", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("688", "nro", "3", "0", "0", "0", "-152", "-2461", "4", "999", "ecommons", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("692", "oasis", "1", "0", "0", "0", "999999", "-1886", "999999", "999", "nro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("738", "sro", "1", "0", "0", "0", "999999", "-1899", "7", "999", "oasis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("693", "oasis", "2", "0", "0", "0", "999999", "1530", "3", "999", "sro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("739", "sro", "2", "0", "0", "0", "1148", "2575", "999999", "999", "innothule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("640", "innothule", "1", "0", "0", "0", "1139", "-3192", "999999", "999", "sro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("641", "innothule", "2", "0", "0", "0", "-3128", "-1133", "-10", "999", "feerrott", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("851", "guktop", "1", "0", "0", "0", "-136", "1667", "-98", "360", "gukbottom", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("642", "innothule", "3", "0", "0", "0", "50", "-130", "5", "999", "gukta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("92", "grobb", "1", "-615", "-2786", "-31", "18", "-177", "4", "0", "innothule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("643", "innothule", "4", "0", "0", "0", "34", "-52", "4", "999", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("94", "feerrott", "1", "1886", "-1132", "-1", "-3108", "-1122", "-10", "0", "innothule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("852", "guktop", "2", "0", "0", "0", "-13", "1498", "-94", "419", "gukbottom", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("857", "gukbottom", "1", "0", "0", "0", "359", "1630", "-88", "999", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("97", "feerrott", "2", "-3088", "413", "4", "3143", "375", "2", "0", "rathemtn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("725", "rathemtn", "1", "0", "0", "0", "-2282", "2518", "4", "999", "lakerathe", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("99", "feerrott", "3", "-96", "-363", "40", "0", "0", "3", "0", "oggok", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("440", "oggok", "1", "0", "0", "0", "808", "1672", "58", "999", "feerrott", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("101", "feerrott", "4", "-69", "72", "40", "-7", "-1489", "50", "0", "cazicthule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("805", "cazicthule", "1", "0", "0", "0", "-107", "-1464", "52", "5", "feerrott", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("726", "rathemtn", "2", "0", "0", "0", "3424", "377", "2", "999", "feerrott", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("807", "lakerathe", "2", "0", "0", "0", "-841", "-59", "9", "999", "arena", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("808", "lakerathe", "3", "0", "0", "0", "1156", "-8563", "4", "999", "southkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("800", "arena", "1", "0", "0", "0", "2696", "2345", "93", "999", "lakerathe", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("107", "lakerathe", "4", "1148", "-8549", "40", "0", "0", "3", "0", "southkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("108", "southkarana", "4", "1153", "4351", "4", "0", "0", "3", "0", "lakerathe", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("369", "erudnext", "3", "0", "0", "0", "296", "2550", "-48", "999", "tox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("182", "erudnint", "1", "-183", "-771", "54.7", "-190", "-1570", "5", "0", "erudnext", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("367", "erudnext", "1", "0", "0", "0", "-185", "-1410", "40", "999", "erudnext", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("770", "tox", "1", "0", "0", "0", "-184", "-1556", "-45", "999", "erudnext", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("771", "tox", "2", "0", "0", "0", "98", "883", "1", "999", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("441", "paineel", "1", "0", "0", "0", "-429", "-2607", "-42", "40", "tox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("772", "tox", "3", "0", "0", "0", "-942", "425", "24", "999", "kerraridge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("647", "kerraridge", "1", "0", "0", "0", "2653", "-512", "-34", "999", "tox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("442", "paineel", "2", "0", "0", "0", "-940", "628", "-94", "483", "hole", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("870", "hole", "1", "0", "0", "0", "-941", "595", "-94", "247", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("443", "paineel", "3", "0", "0", "0", "-914", "748", "-34", "388", "warrens", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("118", "warrens", "1", "0", "0", "3", "0", "0", "3", "0", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("975", "warrens", "2", "0", "0", "0", "2920", "-3735", "-36", "251", "stonebrunt", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("744", "stonebrunt", "1", "0", "0", "0", "1146", "-88", "-107", "511", "warrens", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("121", "erudsxing", "1", "0", "0", "3", "0", "0", "3", "0", "erudnext", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("368", "erudnext", "2", "0", "0", "0", "-311", "-1410", "-40", "999", "erudnext", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("339", "gfaydark", "2", "0", "0", "0", "-1202", "2175", "1", "999", "lfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("663", "lfaydark", "1", "0", "0", "0", "2205", "579", "-112", "999", "steamfont", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("338", "gfaydark", "1", "-2632", "-1931", "23", "200", "40", "4", "999", "felwithea", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("336", "felwithea", "2", "-720", "344", "-10", "-834", "244", "-10", "999", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("370", "felwitheb", "1", "-834", "244", "-10", "-720", "344", "-10", "999", "felwithea", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("341", "gfaydark", "4", "-53", "2600", "16", "162", "-660", "4", "999", "crushbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("345", "crushbone", "1", "162", "-660", "4", "-53", "2608", "16", "999", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("340", "gfaydark", "3", "0", "0", "0", "-3082", "-1320", "1", "999", "butcher", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("549", "butcher", "1", "0", "0", "0", "2673", "-1650", "1", "999", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("550", "butcher", "2", "0", "0", "0", "42", "-64", "4", "999", "kaladima", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("398", "kaladima", "1", "0", "0", "0", "-179", "3131", "1", "999", "butcher", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("399", "kaladima", "2", "0", "0", "0", "340", "416", "-22", "0", "kaladimb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("401", "kaladimb", "1", "0", "0", "0", "334", "422", "-6", "256", "kaladima", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("551", "butcher", "3", "0", "0", "0", "262", "2856", "473", "999", "cauldron", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("557", "cauldron", "1", "0", "0", "0", "-337", "-2935", "1", "999", "butcher", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("558", "cauldron", "2", "0", "0", "0", "30", "110", "326", "999", "kedge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("971", "unrest", "1", "0", "0", "0", "-626", "-2032", "91", "999", "cauldron", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("559", "cauldron", "3", "0", "0", "0", "330", "64", "2", "999", "unrest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("884", "kedge", "1", "0", "0", "0", "-1009", "-1182", "-334", "999", "cauldron", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("664", "lfaydark", "2", "0", "0", "0", "-1113", "-2624", "1", "999", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("741", "steamfont", "1", "0", "0", "0", "-77", "54", "4", "999", "akanon", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("665", "lfaydark", "3", "0", "0", "0", "126", "-347", "-182", "999", "mistmoore", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("888", "mistmoore", "1", "0", "0", "0", "3359", "-1115", "4", "999", "lfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("742", "steamfont", "2", "0", "0", "0", "-2177", "918", "-4", "999", "lfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("148", "akanon", "1", "533", "-2047", "-107", "-62", "73", "3.1", "0", "steamfont", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("354", "cabeast", "1", "0", "0", "0", "3680", "-2555", "8", "306", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("150", "cabwest", "1", "312", "-35", "4", "318", "-12", "4", "0", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("355", "cabeast", "2", "0", "0", "0", "3532", "-2735", "8", "47", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("152", "swampofnohope", "1", "-574", "-245", "4", "0", "0", "3", "0", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("356", "cabeast", "3", "0", "0", "0", "3056", "3119", "2", "193", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("155", "cabwest", "2", "0", "0", "3", "0", "0", "3", "0", "warslikswoods", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("156", "warslikswoods", "3", "0", "0", "3", "0", "0", "3", "0", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("157", "cabwest", "3", "0", "0", "3", "0", "0", "3", "0", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("650", "lakeofillomen", "1", "0", "0", "0", "-4620", "-406", "115", "176", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("746", "swampofnohope", "2", "0", "0", "0", "999999", "4990", "999999", "999", "firiona", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("776", "trakanon", "1", "0", "0", "0", "-4542", "1735", "90", "256", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("747", "swampofnohope", "3", "0", "0", "0", "999999", "4935", "999999", "999", "firiona", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("777", "trakanon", "2", "0", "0", "0", "999999", "-3425", "999999", "0", "emeraldjungle", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("900", "sebilis", "1", "0", "0", "0", "-4759", "-1637", "-475", "256", "trakanon", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("778", "trakanon", "3", "0", "0", "0", "-1", "248", "41", "257", "sebilis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("586", "emeraldjungle", "1", "0", "0", "0", "-1210", "999999", "999999", "999", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("587", "emeraldjungle", "2", "0", "0", "0", "-886", "-26", "5", "128", "citymist", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("553", "citymist", "1", "0", "0", "0", "-1865", "326", "-327", "384", "emeraldjungle", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("588", "emeraldjungle", "3", "0", "0", "0", "-886", "26", "5", "128", "citymist", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("881", "kaesora", "1", "0", "0", "0", "-162", "-1893", "-128", "380", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("885", "kurn", "1", "0", "0", "0", "1022", "459", "70", "999", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("651", "lakeofillomen", "2", "0", "0", "0", "0", "-637", "-26", "0", "veksar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("609", "frontiermtns", "1", "0", "0", "0", "3345", "-410", "115", "384", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("652", "lakeofillomen", "3", "0", "0", "0", "0", "-637", "-26", "0", "veksar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("973", "veksar", "1", "0", "0", "0", "-59", "-418", "-190", "128", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("610", "frontiermtns", "2", "0", "0", "0", "-3474", "2341", "140", "999", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("809", "chardok", "1", "0", "0", "0", "-4149", "7313", "-234", "0", "burningwood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("473", "qeynos", "4", "0", "0", "0", "-184", "307", "-38", "250", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("184", "erudnext", "4", "0", "0", "3", "0", "0", "3", "0", "qeynos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("185", "freportn", "2", "0", "0", "3", "0", "0", "3", "0", "butcher", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("186", "butcher", "4", "0", "0", "3", "0", "0", "3", "0", "freportn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("187", "oasis", "3", "0", "0", "3", "0", "0", "3", "0", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("694", "overthere", "1", "0", "0", "0", "4070", "-241", "246", "384", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("193", "freporte", "3", "-621", "-401", "-24", "99", "-61", "-23", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("192", "freportw", "4", "-61", "99", "-24", "-401", "-621", "-24", "0", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("381", "freportw", "5", "0", "0", "0", "-656", "12", "-55", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("195", "freportn", "3", "-126", "225", "-10", "-414", "490", "-10", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("595", "fieldofbone", "1", "0", "0", "0", "-618", "1160", "8", "999", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("596", "fieldofbone", "2", "0", "0", "0", "-417", "1362", "8", "999", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("597", "fieldofbone", "3", "0", "0", "0", "40", "370", "102", "256", "kaesora", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("598", "fieldofbone", "4", "0", "0", "0", "63", "-280", "0", "999", "kurn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("196", "highpass", "4", "89", "-90", "31", "-89", "-104", "31", "0", "highkeep", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("197", "highkeep", "2", "-104", "-89", "31", "-90", "89", "31", "0", "highpass", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("198", "freportn", "4", "-283", "1588", "31", "370", "731", "-108", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("382", "freportw", "6", "0", "0", "0", "-682", "147", "-13", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("200", "freportn", "5", "-581", "735", "-198", "6", "-440", "-198", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("383", "freportw", "7", "0", "0", "0", "-83", "-376", "999999", "999", "freportn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("202", "freporte", "4", "-1629", "-756", "-948", "-148", "342", "-948", "0", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("384", "freportw", "8", "-126", "225", "-10", "586", "-352", "999999", "999", "freportn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("204", "guktop", "3", "-215", "1196", "-808", "1195", "-187", "-808", "0", "gukbottom", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("858", "gukbottom", "2", "0", "0", "0", "-59", "1527", "-94", "165", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("853", "guktop", "4", "0", "0", "0", "650", "1127", "-85", "0", "gukbottom", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("859", "gukbottom", "3", "0", "0", "0", "0", "0", "5", "0", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("854", "guktop", "5", "0", "0", "0", "-212", "1196", "-78", "999", "gukbottom", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("860", "gukbottom", "4", "0", "0", "0", "608", "1134", "-81", "0", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("479", "qeynos2", "5", "0", "0", "0", "105", "637", "-38", "999", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("486", "qcat", "3", "0", "0", "0", "-483", "175", "-27", "999", "qeynos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("480", "qeynos2", "6", "0", "0", "0", "217", "889", "-50", "999", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("487", "qcat", "4", "0", "0", "0", "-553", "273", "-62", "999", "qeynos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("214", "qeynos", "5", "-63", "230", "-600", "174", "-483", "-500", "0", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("488", "qcat", "5", "0", "0", "0", "90", "175", "-38", "999", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("216", "qeynos", "6", "-315", "217", "-388", "-146", "-593", "-248", "0", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("489", "qcat", "6", "0", "0", "0", "340", "190", "-30", "999", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("218", "qeynos", "7", "150", "-176", "-808", "-188", "140", "-948", "0", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("490", "qcat", "7", "0", "0", "0", "-852", "1122", "-50", "275", "tox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("911", "soldunga", "2", "0", "0", "0", "-489", "999999", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("956", "soldungb", "2", "0", "0", "0", "-495", "999999", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("912", "soldunga", "3", "0", "0", "0", "-532", "999999", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("957", "soldungb", "3", "0", "0", "0", "-538", "999999", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("913", "soldunga", "4", "0", "0", "0", "-448", "999999", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("959", "soldungb", "4", "0", "0", "0", "-175", "343", "-52", "999", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("914", "soldunga", "5", "0", "0", "0", "999999", "-371", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("960", "soldungb", "5", "0", "0", "0", "999999", "-361", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("418", "neriakb", "3", "0", "0", "0", "999999", "206", "999999", "999", "neriakc", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("415", "neriaka", "3", "0", "0", "0", "-1105", "2252", "1", "999", "nektulos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("400", "kaladima", "3", "0", "0", "0", "-224", "354", "4", "0", "kaladimb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("402", "kaladimb", "2", "0", "0", "0", "-264", "390", "4", "256", "kaladima", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("357", "cabeast", "4", "0", "0", "0", "3230", "2972", "2", "446", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("599", "fieldofbone", "5", "0", "0", "0", "-63", "-280", "0", "999", "kurn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("234", "cabeast", "5", "3224", "2974", "32", "-281", "-456", "31", "0", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("748", "swampofnohope", "4", "0", "0", "0", "4455", "1555", "-271", "0", "trakanon", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("236", "cabwest", "4", "-1103", "-2255", "2659", "872", "1126", "31", "0", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("789", "warslikswood", "1", "0", "0", "0", "4300", "999999", "999999", "999", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("362", "cabwest", "5", "0", "0", "0", "-6777", "6330", "37", "999", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("653", "lakeofillomen", "4", "0", "0", "0", "3516", "1291", "210", "999", "firiona", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("604", "firiona", "1", "0", "0", "0", "-6102", "291", "140", "999", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("749", "swampofnohope", "5", "0", "0", "0", "-460", "-281", "8", "999", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("605", "firiona", "2", "0", "0", "0", "999999", "-4391", "999999", "999", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("750", "swampofnohope", "6", "0", "0", "0", "-656", "-78", "8", "999", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("554", "citymist", "2", "0", "0", "0", "-1865", "272", "-327", "384", "emeraldjungle", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("589", "emeraldjungle", "4", "0", "0", "0", "999999", "3975", "999999", "256", "trakanon", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("600", "fieldofbone", "6", "0", "0", "0", "-3933", "999999", "999999", "999", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("886", "kurn", "2", "0", "0", "0", "969", "459", "70", "999", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("606", "firiona", "3", "0", "0", "0", "999999", "-4490", "999999", "999", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("654", "lakeofillomen", "5", "0", "0", "0", "1190", "-3649", "999999", "999", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("607", "firiona", "4", "0", "0", "0", "-4410", "1293", "216", "999", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("567", "dreadlands", "1", "0", "0", "0", "6010", "299", "-76", "999", "firiona", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("601", "fieldofbone", "7", "0", "0", "0", "999999", "5566", "999999", "999", "swampofnohope", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("751", "swampofnohope", "7", "0", "0", "0", "999999", "-3365", "999999", "999", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("695", "overthere", "2", "0", "0", "0", "1423", "5970", "370", "256", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("790", "warslikswood", "2", "0", "0", "0", "-4390", "-241", "248", "128", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("696", "overthere", "3", "0", "0", "0", "-4327", "-1089", "44", "999", "skyfire", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("874", "charasis", "1", "0", "0", "0", "0", "0", "-2", "999", "charasis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("697", "overthere", "4", "0", "0", "0", "-4327", "-1199", "44", "999", "skyfire", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("611", "frontiermtns", "3", "0", "0", "0", "-4625", "-2804", "-17", "999", "burningwood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("698", "overthere", "5", "0", "0", "0", "0", "0", "-2", "999", "charasis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("734", "skyfire", "1", "0", "0", "0", "999999", "5275", "999999", "999", "burningwood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("262", "veeshan", "1", "2979", "2706", "-77", "40", "1680", "271", "0", "skyfire", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("735", "skyfire", "2", "0", "0", "0", "4216", "-1100", "104", "999", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("545", "burningwood", "1", "0", "0", "0", "999999", "3380", "999999", "999", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("736", "skyfire", "3", "0", "0", "0", "4216", "-988", "104", "999", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("546", "burningwood", "2", "0", "0", "0", "999999", "-5708", "999999", "999", "skyfire", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("810", "chardok", "2", "0", "0", "0", "-190", "290", "7", "256", "chardokb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("547", "burningwood", "3", "0", "0", "0", "4300", "-2321", "-450", "999", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("612", "frontiermtns", "4", "0", "0", "0", "-1799", "-2227", "4", "999", "nurga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("548", "burningwood", "4", "0", "0", "0", "930", "-100", "105", "384", "chardok", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("568", "dreadlands", "2", "0", "0", "0", "-792", "-5047", "201", "999", "burningwood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("882", "karnor", "1", "0", "0", "0", "-1948", "702", "30", "999", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("569", "dreadlands", "3", "0", "0", "0", "-3234", "-3801", "-340", "999", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("613", "frontiermtns", "5", "0", "0", "0", "376", "1412", "4", "999", "droga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("570", "dreadlands", "4", "0", "0", "0", "389", "-82", "0", "999", "karnor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("276", "frontiermtns", "6", "375", "1403", "31", "3298", "2967", "3448", "0", "droga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("818", "droga", "1", "0", "0", "0", "2966", "3279", "350", "999", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("614", "frontiermtns", "7", "0", "0", "0", "999999", "-6050", "999999", "999", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("279", "nurga", "1", "-538", "-2701", "-4968", "-2212", "-1799", "31", "0", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("280", "droga", "2", "-920", "-845", "-1728", "-1312", "-919", "-768", "0", "nurga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("897", "nurga", "2", "0", "0", "0", "-538", "-2686", "-490", "999", "frontiermtns", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("282", "overthere", "6", "-4306", "-1202", "392", "-1099", "4186", "987", "0", "skyfire", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("283", "skyfire", "4", "4186", "-1099", "987", "-1202", "-4306", "392", "0", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("883", "karnor", "2", "0", "0", "0", "-1948", "496", "30", "999", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("571", "dreadlands", "5", "0", "0", "0", "389", "115", "0", "999", "karnor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("286", "droga", "3", "103", "-1037", "-1088", "-1392", "264", "-1888", "0", "nurga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("287", "nurga", "3", "264", "-1392", "-1888", "-1037", "103", "-1088", "0", "droga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("444", "paineel", "4", "0", "0", "0", "292", "626", "-197", "399", "hole", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("289", "hole", "2", "319", "483", "-100", "483", "319", "-500", "0", "paineel", "0", "1", "1", "0");
INSERT INTO zone_points VALUES("637", "iceclad", "1", "0", "0", "0", "-5915", "999999", "999999", "999", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("291", "frozenshadow", "1", "0", "0", "0", "0", "0", "0", "0", "iceclad", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("292", "iceclad", "2", "-5803", "-4500", "5602", "-4500", "10636", "5602", "0", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("581", "eastwastes", "1", "0", "0", "0", "10700", "999999", "999999", "999", "iceclad", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("879", "kael", "1", "0", "0", "0", "-5043", "-561", "-195", "128", "wakening", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("582", "eastwastes", "2", "0", "0", "0", "-655", "-8", "124", "128", "kael", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("616", "greatdivide", "1", "0", "0", "0", "6675", "-1769", "197", "999", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("583", "eastwastes", "3", "0", "0", "0", "-5103", "-1785", "233", "999", "greatdivide", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("811", "crystal", "1", "0", "0", "0", "478", "-4065", "145", "505", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("299", "eastwastes", "4", "0", "0", "0", "0", "0", "0", "0", "crystal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("786", "wakening", "1", "0", "0", "0", "-730", "-210", "10", "20", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("880", "kael", "2", "0", "0", "0", "7020", "-6248", "-299", "381", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("787", "wakening", "2", "0", "0", "0", "3190", "-133", "-395", "384", "kael", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("922", "skyshrine", "1", "0", "0", "0", "-155", "745", "258", "125", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("555", "cobaltscar", "1", "0", "0", "0", "70", "-598", "-93", "510", "sirens", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("923", "skyshrine", "2", "0", "0", "0", "-290", "1450", "195", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("306", "cobaltscar", "2", "71", "-585", "-938", "1573", "1627", "660", "0", "sirens", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("908", "sirens", "1", "0", "0", "0", "1626", "1583", "68", "265", "cobaltscar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("796", "westwastes", "1", "0", "0", "0", "-91", "205", "3.75", "252", "sirens", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("909", "sirens", "2", "0", "0", "0", "-3850", "-5156", "-248", "132", "westwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("310", "westwastes", "2", "0", "0", "0", "0", "0", "0", "0", "necropolis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("311", "necropolis", "1", "0", "0", "0", "0", "0", "0", "0", "westwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("312", "westwastes", "3", "0", "0", "0", "0", "0", "0", "0", "templeveeshan", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("313", "templeveeshan", "1", "0", "0", "0", "0", "0", "0", "0", "westwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("584", "eastwastes", "5", "0", "0", "0", "303", "487", "-74", "384", "crystal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("315", "sleeper", "1", "0", "0", "0", "0", "0", "0", "0", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("617", "greatdivide", "2", "0", "0", "0", "-22", "-1230", "0", "0", "thurgadina", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("542", "thurgadinb", "1", "0", "0", "0", "35", "240", "3", "256", "thurgadina", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("539", "thurgadina", "1", "0", "0", "0", "-114", "49", "100", "256", "greatdivide", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("543", "thurgadinb", "2", "0", "0", "0", "2080", "-1643", "132", "999", "greatdivide", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("618", "greatdivide", "3", "0", "0", "0", "-72", "589", "-152", "254", "velketor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("972", "velketor", "1", "0", "0", "0", "3332", "-6759", "-36", "395", "greatdivide", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("540", "thurgadina", "2", "0", "0", "0", "-78", "-760", "-95", "0", "thurgadinb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("544", "thurgadinb", "3", "0", "0", "0", "-88", "-665", "-95", "256", "thurgadina", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("324", "61", "1", "0", "0", "0", "200", "40", "4", "999", "", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("325", "57", "2", "0", "0", "0", "-1202", "2175", "1", "999", "", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("326", "68", "3", "0", "0", "0", "-3082", "-1320", "1", "999", "", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("327", "58", "4", "0", "0", "0", "162", "-660", "4", "999", "", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("328", "202", "177", "0", "0", "0", "882", "838", "-157", "2", "", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("329", "202", "178", "0", "0", "0", "97", "812", "-157", "380", "", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("342", "gfaydark", "177", "0", "0", "0", "882", "838", "-157", "2", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("343", "gfaydark", "178", "0", "0", "0", "97", "812", "-157", "380", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("337", "felwithea", "3", "0", "0", "0", "-2632", "-1931", "21", "999", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("346", "gunthak", "1", "0", "0", "0", "520", "560", "5", "352", "dulak", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("347", "gunthak", "2", "0", "0", "0", "-1545", "-4110", "-375", "0", "stonebrunt", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("348", "gunthak", "3", "0", "0", "0", "420", "-660", "-25", "506", "nadox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("349", "gunthak", "4", "0", "0", "0", "-644", "-1349", "-40", "384", "nadox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("350", "akanon", "255", "0", "0", "0", "528", "-2062", "-110", "999", "steamfont", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("351", "bazaar", "1", "0", "0", "0", "675", "305", "-100", "256", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("352", "bazaar", "2", "0", "0", "0", "795", "305", "-100", "256", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("353", "bazaar", "3", "0", "0", "0", "-90", "-420", "-60", "256", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("358", "cabeast", "9", "0", "0", "0", "-189", "1007", "48", "260", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("359", "cabeast", "10", "0", "0", "0", "-188", "1028", "23", "257", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("360", "cabeast", "80", "0", "0", "0", "300", "999999", "999999", "999", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("361", "cabeast", "81", "0", "0", "0", "300", "999999", "999999", "999", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("363", "cabwest", "6", "0", "0", "0", "-6628", "6504", "37", "999", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("364", "cabwest", "7", "0", "0", "0", "-930", "-2404", "271", "999", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("365", "cabwest", "8", "0", "0", "0", "-1107", "-2254", "271", "999", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("366", "cabwest", "81", "0", "0", "0", "270", "999999", "999999", "999", "cabeast", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("371", "felwitheb", "2", "-585", "435", "31", "-497", "524", "-2", "176", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("372", "felwitheb", "3", "-600", "455", "33", "-602", "583", "-5", "43", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("373", "felwitheb", "4", "-613", "435", "33", "-717", "484", "-4", "469", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("374", "felwitheb", "5", "0", "0", "0", "-587", "301", "-10", "494", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("375", "felwitheb", "6", "0", "0", "0", "-587", "301", "-10", "494", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("376", "felwitheb", "7", "0", "0", "0", "-587", "301", "-10", "494", "felwitheb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("377", "freporte", "9", "0", "0", "0", "-926", "-84", "-28", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("378", "freporte", "10", "0", "0", "0", "-622", "-418", "-28", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("379", "freporte", "11", "0", "0", "0", "999999", "4154", "-27", "999", "nro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("380", "freporte", "53", "0", "0", "0", "-1632", "-740", "-94", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("385", "freportw", "9", "0", "0", "0", "-64", "100", "-24", "999", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("386", "freportw", "10", "0", "0", "0", "-360", "448", "-28", "999", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("387", "freportw", "12", "0", "0", "0", "-1592", "999999", "-54", "999", "ecommons", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("388", "freportw", "51", "0", "0", "0", "722", "372", "-8", "999", "freportn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("389", "freportw", "52", "0", "0", "0", "-441", "-2", "-18", "999", "freportn", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("390", "freportw", "53", "0", "0", "0", "344", "-155", "-94", "999", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("391", "freportw", "177", "0", "0", "0", "-234", "-406", "-157", "258", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("392", "freportn", "7", "0", "0", "0", "-700", "267", "999999", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("393", "freportn", "8", "0", "0", "0", "-124", "227", "999999", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("394", "freportn", "51", "0", "0", "0", "-275", "1590", "4", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("395", "freportn", "52", "0", "0", "0", "-581", "730", "-18", "999", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("396", "gukta", "1", "0", "0", "0", "-615", "-2754", "-32", "999", "innothule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("403", "katta", "1", "0", "0", "0", "1801", "-59", "-36", "380", "tenebrous", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("404", "katta", "2", "0", "0", "0", "-1976", "-183", "95", "130", "twilight", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("405", "katta", "3", "0", "0", "0", "-1977", "-32", "95", "129", "twilight", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("406", "katta", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("407", "katta", "7", "0", "0", "0", "-96", "-907", "73", "256", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("408", "katta", "8", "0", "0", "0", "-96", "-907", "143", "256", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("409", "katta", "9", "0", "0", "0", "-96", "-907", "210", "256", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("410", "katta", "10", "0", "0", "0", "-100", "-760", "4", "510", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("411", "katta", "11", "0", "0", "0", "-1085", "-163", "-266", "244", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("412", "katta", "12", "0", "0", "0", "-1085", "-163", "-266", "244", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("420", "neriakc", "2", "0", "0", "0", "1146", "-876", "-1", "252", "lfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("425", "nexus", "0", "0", "0", "0", "9655", "3113", "1047", "251", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("426", "nexus", "2", "0", "0", "0", "-345", "2045", "-63", "125", "netherbian", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("427", "nexus", "3", "0", "0", "0", "1570", "170", "-57", "999", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("428", "nexus", "4", "0", "0", "0", "192", "-875", "4", "0", "bazaar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("429", "nexus", "6", "0", "0", "0", "-3590", "-2068", "-94", "128", "shadeweaver", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("430", "nexus", "7", "0", "0", "0", "3255", "1167", "112", "384", "hollowshade", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("431", "nexus", "8", "0", "0", "0", "-232", "1166", "59", "0", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("432", "nexus", "9", "0", "0", "0", "1479", "170", "-55", "0", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("433", "nexus", "10", "0", "0", "0", "-550", "636", "3", "256", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("434", "nexus", "11", "0", "0", "0", "1209", "-3685", "-5", "256", "northkarana", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("435", "nexus", "12", "0", "0", "0", "9658", "3047", "1052", "249", "dreadlands", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("436", "nexus", "13", "0", "0", "0", "1571", "170", "-50", "0", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("437", "nexus", "14", "0", "0", "0", "-1858", "-420", "-15", "128", "twilight", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("438", "nexus", "25", "0", "0", "0", "-623", "-1249", "-29", "0", "letalis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("439", "nexus", "77", "0", "0", "0", "-76", "381", "-157", "1", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("445", "paineel", "5", "0", "0", "0", "999999", "725", "999999", "999", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("446", "paineel", "10", "0", "0", "0", "551", "959", "-79", "128", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("447", "paineel", "11", "0", "0", "0", "525", "906", "-121", "256", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("448", "paineel", "12", "0", "0", "0", "658", "1065", "-40", "256", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("449", "paineel", "13", "0", "0", "0", "217", "464", "41", "0", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("450", "paineel", "14", "0", "0", "0", "755", "886", "-77", "0", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("451", "paineel", "15", "0", "0", "0", "658", "1091", "-41", "0", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("452", "poknowledge", "1", "0", "0", "0", "-1260", "-513", "9", "259", "misty", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("453", "poknowledge", "2", "0", "0", "0", "-734", "-188", "-3", "430", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("454", "poknowledge", "3", "0", "0", "0", "-523", "1726", "-1", "45", "butcher", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("455", "poknowledge", "4", "0", "0", "0", "-31", "2835", "-62", "453", "everfrost", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("456", "poknowledge", "5", "0", "0", "0", "-1824", "-2223", "-1", "244", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("457", "poknowledge", "6", "0", "0", "0", "-538", "2327", "-42", "375", "tox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("458", "poknowledge", "7", "0", "0", "0", "442", "48", "-29", "135", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("459", "poknowledge", "8", "0", "0", "0", "-1811", "-41", "391", "0", "greatdivide", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("460", "poknowledge", "9", "0", "0", "0", "-2433", "-2970", "-215", "236", "shadeweaver", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("461", "poknowledge", "10", "0", "0", "0", "487", "219", "2", "267", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("462", "poknowledge", "11", "0", "0", "0", "-1463", "774", "-878", "131", "potranquility", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("463", "poknowledge", "12", "0", "0", "0", "4673", "-455", "9", "128", "firiona", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("464", "poknowledge", "13", "0", "0", "0", "76", "-658", "-32", "255", "freportw", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("465", "poknowledge", "14", "0", "0", "0", "931", "-1329", "-109", "248", "steamfont", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("466", "poknowledge", "15", "0", "0", "0", "1845", "-2980", "11", "259", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("467", "poknowledge", "16", "0", "0", "0", "-40", "-706", "-25", "216", "innothule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("468", "poknowledge", "17", "0", "0", "0", "-349", "705", "-4", "20", "nektulos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("469", "poknowledge", "18", "0", "0", "0", "-163", "908", "-9", "248", "feerrott", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("470", "poknowledge", "19", "0", "0", "0", "1888", "3133", "-51", "128", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("471", "poknowledge", "20", "0", "0", "0", "272", "-2347", "-47", "116", "tox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("474", "qeynos", "22", "0", "0", "0", "147", "-175", "-77", "999", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("475", "qeynos", "33", "0", "0", "0", "-301", "217", "-38", "999", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("481", "qeynos2", "14", "0", "0", "0", "756", "893", "-82", "0", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("482", "qeynos2", "77", "0", "0", "0", "-289", "147", "-157", "383", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("483", "qeynos2", "99", "0", "0", "0", "-49", "1057", "-43", "999", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("491", "qcat", "99", "0", "0", "0", "-161", "302", "-13", "999", "qeynos2", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("492", "rivervale", "98", "0", "0", "0", "3825", "2022", "463", "999", "kithicor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("493", "rivervale", "99", "0", "0", "0", "-2552", "410", "-4", "999", "misty", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("494", "sseru", "1", "0", "0", "0", "-1064", "-2140", "-27", "128", "mseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("495", "sseru", "2", "0", "0", "0", "2081", "-72", "89", "384", "dawnshroud", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("496", "sseru", "3", "0", "0", "0", "2089", "74", "89", "384", "dawnshroud", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("497", "sseru", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("498", "sseru", "8", "0", "0", "0", "-232", "1166", "59", "0", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("499", "sseru", "188", "0", "0", "0", "-12", "1017", "225", "248", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("500", "sseru", "189", "0", "0", "0", "-447", "1016", "225", "249", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("501", "sseru", "191", "0", "0", "0", "281", "-137", "157", "252", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("502", "sseru", "192", "0", "0", "0", "64", "82", "157", "376", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("503", "sseru", "193", "0", "0", "0", "65", "-22", "122", "380", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("504", "sseru", "194", "0", "0", "0", "179", "-132", "122", "247", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("505", "sseru", "195", "0", "0", "0", "-743", "-132", "157", "259", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("506", "sseru", "196", "0", "0", "0", "-526", "82", "157", "119", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("507", "sseru", "197", "0", "0", "0", "-533", "-17", "122", "128", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("508", "sseru", "198", "0", "0", "0", "-642", "-134", "122", "252", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("509", "sseru", "201", "0", "0", "0", "280", "-730", "157", "3", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("510", "sseru", "202", "0", "0", "0", "65", "-947", "157", "388", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("511", "sseru", "203", "0", "0", "0", "66", "-844", "122", "383", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("512", "sseru", "204", "0", "0", "0", "179", "-730", "122", "510", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("513", "sseru", "205", "0", "0", "0", "-528", "-944", "157", "122", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("514", "sseru", "206", "0", "0", "0", "-746", "-724", "157", "503", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("515", "sseru", "207", "0", "0", "0", "-643", "-730", "122", "503", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("516", "sseru", "208", "0", "0", "0", "-531", "-843", "122", "120", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("517", "sseru", "209", "0", "0", "0", "-372", "-282", "130", "124", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("518", "sseru", "210", "0", "0", "0", "-92", "-579", "193", "509", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("519", "sseru", "211", "0", "0", "0", "367", "1148", "59", "121", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("520", "sseru", "212", "0", "0", "0", "-133", "995", "59", "246", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("521", "sseru", "213", "0", "0", "0", "-331", "987", "59", "251", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("522", "shadowhaven", "1", "0", "0", "0", "0", "0", "-30", "999", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("523", "shadowhaven", "2", "0", "0", "0", "-900", "835", "-28", "0", "echo", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("524", "shadowhaven", "3", "0", "0", "0", "-81", "-1000", "0", "0", "echo", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("525", "shadowhaven", "4", "0", "0", "0", "874", "-2210", "-315", "384", "paludal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("526", "shadowhaven", "6", "0", "0", "0", "1300", "190", "-55", "999", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("527", "shadowhaven", "7", "0", "0", "0", "1230", "190", "-109", "999", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("528", "shadowhaven", "8", "0", "0", "0", "-135", "-875", "15", "0", "bazaar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("529", "shadowhaven", "9", "0", "0", "0", "-215", "-875", "15", "0", "bazaar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("530", "shadowhaven", "10", "0", "0", "0", "25", "-1323", "60", "999", "echo", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("531", "shadowhaven", "11", "0", "0", "0", "1059", "-2122", "-315", "384", "paludal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("532", "shadowhaven", "12", "0", "0", "0", "431", "-675", "-340", "999", "paludal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("533", "sharvahl", "1", "0", "0", "0", "-3585", "-2068", "-94", "125", "shadeweaver", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("534", "sharvahl", "2", "0", "0", "0", "3260", "1170", "111", "380", "hollowshade", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("535", "sharvahl", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("536", "qrg", "3", "0", "0", "0", "-64", "230", "-57", "406", "neriakd", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("537", "qrg", "5", "0", "0", "0", "84", "5198", "-2", "999", "qeytoqrg", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("538", "qrg", "7", "0", "0", "0", "1790", "1315", "-13", "128", "jaggedpine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("541", "thurgadina", "3", "0", "0", "0", "45", "106", "3", "0", "thurgadinb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("552", "butcher", "77", "0", "0", "0", "474", "829", "-157", "511", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("556", "cobaltscar", "10", "0", "0", "0", "-328", "449", "43.72", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("560", "dawnshroud", "1", "0", "0", "0", "-305", "1469", "59", "257", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("561", "dawnshroud", "2", "0", "0", "0", "71", "-696", "3", "2", "netherbian", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("562", "dawnshroud", "3", "0", "0", "0", "3513", "-14", "-4", "369", "griegsend", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("563", "dawnshroud", "4", "0", "0", "0", "2045", "1015", "-210", "256", "maiden", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("564", "dawnshroud", "5", "0", "0", "0", "-145", "1443", "59", "256", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("565", "dawnshroud", "6", "0", "0", "0", "-54", "-706", "3", "2", "netherbian", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("566", "dreadlands", "0", "0", "0", "0", "0", "0", "-32", "128", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("572", "dreadlands", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("573", "dulak", "1", "0", "0", "0", "-629", "-321", "3", "384", "torgiran", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("574", "dulak", "2", "0", "0", "0", "2470", "-1050", "68", "496", "gunthak", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("585", "eastwastes", "13", "0", "0", "0", "-37", "-13", "3", "128", "sleeper", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("593", "everfrost", "4", "0", "0", "0", "-4158", "4733", "-97", "999", "everfrost", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("594", "everfrost", "77", "0", "0", "0", "131", "846", "-157", "511", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("602", "fieldofbone", "8", "0", "0", "0", "5330", "999999", "999999", "999", "emeraldjungle", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("603", "fieldofbone", "77", "0", "0", "0", "36", "-655", "-157", "1", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("608", "firiona", "77", "0", "0", "0", "-279", "-363", "-157", "383", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("615", "greatdivide", "0", "0", "0", "0", "0", "0", "-32", "128", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("619", "greatdivide", "4", "0", "0", "0", "-395", "-1410", "114", "0", "mischiefplane", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("620", "greatdivide", "77", "0", "0", "0", "-235", "393", "-157", "810", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("621", "thegrey", "1", "0", "0", "0", "-2", "1401", "50", "340", "letalis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("622", "thegrey", "2", "0", "0", "0", "-25", "-34", "3", "128", "ssratemple", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("623", "thegrey", "3", "0", "0", "0", "1965", "779", "-39", "384", "scarlet", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("624", "grimling", "1", "0", "0", "0", "-663", "30", "4", "256", "acrylia", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("625", "grimling", "2", "0", "0", "0", "-2528", "3326", "266", "256", "hollowshade", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("626", "grimling", "3", "0", "0", "0", "-1133", "-1749", "-23", "470", "tenebrous", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("633", "hollowshade", "1", "0", "0", "0", "-475", "1665", "-212", "256", "sharvahl", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("634", "hollowshade", "2", "0", "0", "0", "-1213", "-1790", "37", "0", "grimling", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("635", "hollowshade", "3", "0", "0", "0", "-338", "1584", "124", "264", "paludal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("636", "hollowshade", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("638", "iceclad", "7", "0", "0", "0", "200", "100", "0", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("639", "iceclad", "9", "0", "0", "0", "-840", "810", "-3", "145", "nro", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("644", "innothule", "77", "0", "0", "0", "95", "-812", "-157", "384", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("645", "jaggedpine", "1", "0", "0", "0", "-1739", "-122", "256", "128", "182", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("646", "jaggedpine", "2", "0", "0", "0", "155", "155", "-155", "255", "blackburrow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("655", "lakeofillomen", "6", "0", "0", "0", "767", "-783", "8", "382", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("656", "lakeofillomen", "7", "0", "0", "0", "566", "-987", "8", "4", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("658", "lavastorm", "25", "0", "0", "0", "289", "3107", "-17", "999", "nektulos", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("659", "lavastorm", "31", "0", "0", "0", "-525", "-443", "70", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("660", "lavastorm", "32", "0", "0", "0", "-265", "-413", "-112", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("661", "lavastorm", "77", "0", "0", "0", "56", "250", "3", "999", "soltemple", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("662", "lavastorm", "88", "0", "0", "0", "56", "250", "3", "999", "soltemple", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("666", "lfaydark", "9", "0", "0", "0", "-1775", "910", "-75", "226", "neriakc", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("667", "maiden", "1", "0", "0", "0", "-2035", "-710", "52", "256", "dawnshroud", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("668", "maiden", "2", "0", "0", "0", "-70", "-1430", "23", "0", "akheva", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("669", "maiden", "3", "0", "0", "0", "180", "-1430", "23", "0", "akheva", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("670", "maiden", "4", "0", "0", "0", "2115", "-575", "23", "385", "umbral", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("671", "mseru", "1", "0", "0", "0", "389", "1151", "59", "379", "sseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("672", "mseru", "2", "0", "0", "0", "1020", "525", "72", "377", "netherbian", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("673", "mseru", "3", "0", "0", "0", "-687", "-1390", "11", "100", "letalis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("674", "mseru", "4", "0", "0", "0", "1031", "398", "72", "382", "netherbian", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("677", "misty", "77", "0", "0", "0", "1227", "830", "-157", "511", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("678", "letalis", "1", "0", "0", "0", "353", "-2113", "-59", "489", "thegrey", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("679", "letalis", "2", "0", "0", "0", "-1170", "2139", "161", "237", "mseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("680", "letalis", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("681", "nadox", "1", "0", "0", "0", "1690", "-2260", "-49", "416", "gunthak", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("682", "nadox", "2", "0", "0", "0", "-1030", "110", "0", "128", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("683", "nadox", "3", "0", "0", "0", "2900", "-3795", "558", "128", "gunthak", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("684", "nadox", "4", "0", "0", "0", "-6", "1165", "-20", "335", "torgiran", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("685", "nektulos", "77", "0", "0", "0", "132", "-841", "-157", "257", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("689", "nro", "4", "0", "0", "0", "390", "5330", "-15", "322", "iceclad", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("690", "northkarana", "0", "0", "0", "0", "0", "0", "-32", "128", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("691", "northkarana", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("699", "overthere", "77", "0", "0", "0", "884", "-840", "-157", "254", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("705", "potranquility", "1", "0", "0", "0", "223", "140", "9", "350", "potimea", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("706", "potranquility", "3", "0", "0", "0", "-66", "230", "-76", "999", "neriakd", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("707", "potranquility", "5", "0", "0", "0", "-227", "-1555", "68", "128", "povalor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("708", "potranquility", "17", "0", "0", "0", "-1750", "-1245", "-59", "0", "podisease", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("709", "potranquility", "18", "0", "0", "0", "-341", "1706", "-491", "0", "potorment", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("710", "potranquility", "19", "0", "0", "0", "-170", "-65", "-93", "128", "codecay", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("711", "potranquility", "21", "0", "0", "0", "-210", "10", "-35", "0", "potactics", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("712", "potranquility", "22", "0", "0", "0", "190", "-1668", "65", "0", "povalor", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("713", "potranquility", "23", "0", "0", "0", "-2755", "0", "0", "128", "hohonora", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("714", "potranquility", "24", "0", "0", "0", "-1795", "-2059", "-473", "0", "postorms", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("715", "potranquility", "25", "0", "0", "0", "178", "207", "-1620", "0", "bothunder", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("716", "potranquility", "26", "0", "0", "0", "-1", "-2915", "-766", "0", "solrotower", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("717", "potranquility", "27", "0", "0", "0", "-93", "-1234", "11", "105", "powater", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("718", "potranquility", "28", "0", "0", "0", "532", "884", "-90", "0", "poair", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("719", "potranquility", "29", "0", "0", "0", "-1387", "1210", "-180", "251", "pofire", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("720", "potranquility", "30", "0", "0", "0", "-1150", "200", "71", "0", "poeartha", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("721", "potranquility", "37", "0", "0", "0", "263", "516", "-53", "187", "poinnovation", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("722", "potranquility", "47", "0", "0", "0", "58", "-61", "5", "127", "pojustice", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("723", "potranquility", "57", "0", "0", "0", "1668", "282", "212", "5", "ponightmare", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("724", "potranquility", "77", "0", "0", "0", "-285", "-148", "-159", "383", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("727", "scarlet", "1", "0", "0", "0", "932", "-99", "-80", "1", "griegsend", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("728", "scarlet", "2", "0", "0", "0", "2001", "-1187", "-29", "0", "twilight", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("729", "scarlet", "3", "0", "0", "0", "-2049", "631", "4", "145", "thegrey", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("730", "shadeweaver", "1", "0", "0", "0", "495", "-1685", "-212", "256", "sharvahl", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("731", "shadeweaver", "3", "0", "0", "0", "-288", "-3906", "196", "131", "paludal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("732", "shadeweaver", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("733", "shadeweaver", "77", "0", "0", "0", "-293", "364", "-157", "383", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("737", "skyfire", "5", "0", "0", "0", "1680", "40", "32", "999", "veeshan", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("743", "steamfont", "77", "0", "0", "0", "-76", "-401", "-157", "255", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("745", "stonebrunt", "5", "0", "0", "0", "-940", "1460", "16", "128", "gunthak", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("753", "tenebrous", "1", "0", "0", "0", "-693", "1585", "63", "261", "grimling", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("754", "tenebrous", "2", "0", "0", "0", "-669", "700", "3", "252", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("755", "timorous", "1", "0", "0", "0", "767", "-783", "8", "999", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("756", "timorous", "2", "0", "0", "0", "-950", "-14", "-52", "999", "freporte", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("757", "timorous", "3", "0", "0", "0", "0", "0", "3", "999", "halas", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("758", "timorous", "4", "0", "0", "0", "94", "-25", "3", "999", "felwithea", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("759", "timorous", "5", "0", "0", "0", "10", "-20", "0", "999", "gfaydark", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("760", "timorous", "6", "0", "0", "0", "-500", "3", "-10", "999", "neriakb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("761", "timorous", "7", "0", "0", "0", "0", "0", "20", "999", "erudnext", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("762", "timorous", "8", "0", "0", "0", "-99", "-345", "4", "999", "oggok", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("763", "timorous", "9", "0", "0", "0", "0", "-100", "3", "999", "gukta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("764", "timorous", "10", "0", "0", "0", "0", "0", "4", "999", "rivervale", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("765", "timorous", "11", "0", "0", "0", "-35", "47", "4", "999", "akanon", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("766", "timorous", "12", "0", "0", "0", "-2", "-18", "3", "999", "kaladima", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("767", "torgiran", "1", "0", "0", "0", "-1585", "5", "7", "0", "nadox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("768", "torgiran", "2", "0", "0", "0", "-580", "790", "56", "256", "dulak", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("769", "tox", "0", "0", "0", "0", "0", "0", "-32", "128", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("773", "tox", "4", "0", "0", "0", "203", "266", "-38", "0", "qcat", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("774", "tox", "77", "0", "0", "0", "36", "662", "-157", "256", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("775", "tox", "177", "0", "0", "0", "1227", "-840", "-157", "257", "poknowledge", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("779", "twilight", "1", "0", "0", "0", "-972", "-1750", "-266", "0", "katta", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("780", "twilight", "2", "0", "0", "0", "-1810", "-1118", "-98", "0", "scarlet", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("781", "twilight", "4", "0", "0", "0", "2333", "1955", "19", "384", "fungusgrove", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("782", "twilight", "6", "0", "0", "0", "0", "0", "-25", "0", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("783", "umbral", "1", "0", "0", "0", "-572", "-2186", "153", "380", "umbral", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("784", "umbral", "2", "0", "0", "0", "-2082", "-220", "-152", "128", "maiden", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("785", "umbral", "45", "0", "0", "0", "-1664", "216", "-35", "2", "vexthal", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("788", "wakening", "7", "0", "0", "0", "3019", "-2493", "-21", "256", "growthplane", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("791", "warslikswood", "3", "0", "0", "0", "1126", "869", "8", "999", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("792", "warslikswood", "4", "0", "0", "0", "1327", "668", "8", "260", "cabwest", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("793", "warslikswood", "5", "0", "0", "0", "95", "-6", "4", "999", "dalnir", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("794", "warslikswood", "6", "0", "0", "0", "1160", "4334", "999999", "999", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("795", "commons", "0", "0", "0", "0", "0", "0", "-32", "128", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("797", "westwastes", "20", "0", "0", "0", "1976", "-86", "4", "425", "necropolis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("798", "westwastes", "56", "0", "0", "0", "-499", "-2025", "-44", "211", "templeveeshan", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("804", "blackburrow", "7", "0", "0", "0", "2730", "270", "-2", "340", "jaggedpine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("812", "crystal", "2", "0", "0", "0", "1066", "-3405", "150", "372", "eastwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("813", "dalnir", "1", "0", "0", "0", "4548", "2568", "-238", "60", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("814", "dalnir", "2", "0", "0", "0", "4548", "2568", "-238", "60", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("815", "dalnir", "99", "0", "0", "0", "4548", "2568", "-238", "60", "warslikswood", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("816", "thedeep", "1", "0", "0", "0", "-506", "-865", "-256", "256", "ssratemple", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("817", "thedeep", "2", "0", "0", "0", "1586", "-1765", "128", "0", "echo", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("819", "droga", "5", "0", "0", "0", "105", "-1032", "-106", "999", "nurga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("820", "droga", "6", "0", "0", "0", "-920", "-840", "-172", "999", "nurga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("821", "echo", "1", "0", "0", "0", "1798", "380", "-54", "256", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("822", "echo", "2", "0", "0", "0", "-1145", "-2328", "-308", "128", "fungusgrove", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("823", "echo", "3", "0", "0", "0", "-735", "-385", "-60", "128", "thedeep", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("824", "echo", "4", "0", "0", "0", "1628", "-1111", "-74", "0", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("825", "echo", "5", "0", "0", "0", "-1145", "-2328", "-308", "128", "fungusgrove", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("826", "frozenshadow", "2", "0", "0", "0", "20", "250", "355", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("827", "frozenshadow", "3", "0", "0", "0", "660", "100", "40", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("828", "frozenshadow", "4", "0", "0", "0", "166", "477", "5", "256", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("829", "frozenshadow", "5", "0", "0", "0", "670", "750", "75", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("830", "frozenshadow", "6", "0", "0", "0", "20", "250", "355", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("831", "frozenshadow", "7", "0", "0", "0", "615", "660", "23", "256", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("832", "frozenshadow", "8", "0", "0", "0", "20", "250", "355", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("833", "frozenshadow", "9", "0", "0", "0", "170", "755", "175", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("834", "frozenshadow", "10", "0", "0", "0", "20", "250", "355", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("835", "frozenshadow", "11", "0", "0", "0", "-150", "160", "217", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("836", "frozenshadow", "12", "0", "0", "0", "175", "1160", "190", "256", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("837", "frozenshadow", "13", "0", "0", "0", "-320", "725", "12", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("838", "frozenshadow", "14", "0", "0", "0", "20", "250", "355", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("839", "frozenshadow", "15", "0", "0", "0", "-100", "610", "220", "384", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("840", "frozenshadow", "16", "0", "0", "0", "20", "250", "355", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("841", "frozenshadow", "17", "0", "0", "0", "-490", "175", "2", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("842", "frozenshadow", "18", "0", "0", "0", "10", "65", "310", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("843", "frozenshadow", "19", "0", "0", "0", "200", "100", "0", "0", "frozenshadow", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("844", "frozenshadow", "20", "0", "0", "0", "2739", "1786", "113", "348", "iceclad", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("845", "fungusgrove", "1", "0", "0", "0", "1025", "1145", "-124", "384", "echo", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("846", "fungusgrove", "3", "0", "0", "0", "373", "-1257", "156", "0", "twilight", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("847", "griegsend", "1", "0", "0", "0", "-504", "2262", "10", "129", "scarlet", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("848", "griegsend", "2", "0", "0", "0", "-864", "-1948", "103", "2", "dawnshroud", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("849", "griegsend", "3", "0", "0", "0", "2400", "166", "112", "382", "griegsend", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("850", "griegsend", "4", "0", "0", "0", "2390", "191", "-54", "378", "griegsend", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("855", "guktop", "6", "0", "0", "0", "-823", "143", "-11", "999", "innothule", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("856", "guktop", "7", "0", "0", "0", "-136", "1667", "-98", "360", "gukbottom", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("861", "gukbottom", "5", "0", "0", "0", "-192", "1196", "-78", "999", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("862", "gukbottom", "7", "0", "0", "0", "359", "1630", "-88", "999", "guktop", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("863", "hatesfury", "1", "0", "0", "0", "1490", "-1705", "-100", "0", "nadox", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("864", "hatesfury", "2", "0", "0", "0", "-92", "1120", "-600", "380", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("865", "hatesfury", "3", "0", "0", "0", "1475", "958", "-568", "0", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("866", "hatesfury", "4", "0", "0", "0", "-929", "88", "0", "0", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("867", "hatesfury", "5", "0", "0", "0", "-20", "-220", "65", "256", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("868", "hatesfury", "6", "0", "0", "0", "-1066", "-48", "-284", "380", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("869", "hatesfury", "7", "0", "0", "0", "-1360", "-290", "74", "128", "hatesfury", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("871", "hole", "3", "0", "0", "0", "231", "482", "43", "384", "paineel", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("872", "hole", "4", "0", "0", "0", "-811", "512", "-24", "256", "neriakc", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("873", "hole", "5", "0", "0", "0", "885", "630", "-29", "384", "erudnint", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("875", "charasis", "2", "0", "0", "0", "0", "0", "-2", "999", "charasis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("876", "charasis", "3", "0", "0", "0", "0", "0", "-2", "999", "charasis", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("877", "charasis", "4", "0", "0", "0", "796", "-92", "-502", "999", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("878", "charasis", "5", "0", "0", "0", "796", "-92", "-502", "999", "overthere", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("887", "kurn", "3", "0", "0", "0", "997", "405", "66", "504", "fieldofbone", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("890", "necropolis", "66", "0", "0", "0", "508", "-2867", "-196", "386", "westwastes", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("891", "netherbian", "1", "0", "0", "0", "16", "418", "-29", "383", "nexus", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("892", "netherbian", "2", "0", "0", "0", "-415", "-609", "-4", "162", "netherbian", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("893", "netherbian", "3", "0", "0", "0", "-1835", "600", "-21", "126", "mseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("894", "netherbian", "4", "0", "0", "0", "-1838", "473", "-21", "127", "mseru", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("895", "netherbian", "5", "0", "0", "0", "-113", "2074", "87", "380", "dawnshroud", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("896", "netherbian", "6", "0", "0", "0", "-245", "2080", "87", "114", "dawnshroud", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("898", "nurga", "5", "0", "0", "0", "264", "-1396", "-185", "999", "droga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("899", "nurga", "6", "0", "0", "0", "-920", "-1320", "-76", "999", "droga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("901", "sebilis", "2", "0", "0", "0", "-4759", "-1637", "-475", "256", "trakanon", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("902", "paludal", "1", "0", "0", "0", "-467", "2414", "-381", "265", "shadeweaver", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("903", "paludal", "2", "0", "0", "0", "328", "-1069", "-28", "0", "shadowhaven", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("904", "paludal", "3", "0", "0", "0", "3030", "-1635", "-124", "385", "hollowshade", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("915", "soldunga", "6", "0", "0", "0", "-404", "999999", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("916", "soldunga", "7", "0", "0", "0", "999999", "-278", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("917", "soldunga", "8", "0", "0", "0", "-576", "999999", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("918", "soldunga", "9", "0", "0", "0", "340", "-458", "-67", "0", "soldungc", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("919", "soldunga", "10", "0", "0", "0", "-448", "999999", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("920", "soldunga", "11", "0", "0", "0", "999999", "-371", "999999", "999", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("921", "soldunga", "12", "0", "0", "0", "-381", "-907", "10", "128", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("924", "skyshrine", "3", "0", "0", "0", "-1280", "2260", "195", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("925", "skyshrine", "4", "0", "0", "0", "-920", "2980", "195", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("926", "skyshrine", "5", "0", "0", "0", "-110", "2980", "195", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("927", "skyshrine", "6", "0", "0", "0", "-1280", "1720", "195", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("928", "skyshrine", "7", "0", "0", "0", "-1110", "1450", "195", "125", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("929", "skyshrine", "8", "0", "0", "0", "-1730", "2980", "195", "125", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("930", "skyshrine", "9", "0", "0", "0", "-920", "2980", "375", "125", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("931", "skyshrine", "10", "0", "0", "0", "-560", "1540", "375", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("932", "skyshrine", "11", "0", "0", "0", "-1280", "1720", "375", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("933", "skyshrine", "12", "0", "0", "0", "-470", "2710", "375", "125", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("934", "skyshrine", "13", "0", "0", "0", "-1110", "1450", "375", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("935", "skyshrine", "14", "0", "0", "0", "-650", "2350", "375", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("936", "skyshrine", "15", "0", "0", "0", "-1110", "1720", "375", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("937", "skyshrine", "16", "0", "0", "0", "-1080", "1590", "15", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("938", "skyshrine", "17", "0", "0", "0", "-1460", "2980", "375", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("939", "skyshrine", "18", "0", "0", "0", "-2000", "1990", "195", "385", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("940", "skyshrine", "19", "0", "0", "0", "-200", "1450", "375", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("941", "skyshrine", "20", "0", "0", "0", "-1730", "1630", "195", "250", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("942", "skyshrine", "21", "0", "0", "0", "-1110", "2080", "195", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("943", "skyshrine", "22", "0", "0", "0", "-740", "1630", "195", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("944", "skyshrine", "23", "0", "0", "0", "660", "-60", "0", "999", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("945", "skyshrine", "24", "0", "0", "0", "1201", "1213", "3.75", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("946", "skyshrine", "30", "0", "0", "0", "4766", "1543", "-60", "372", "wakening", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("947", "skyshrine", "40", "0", "0", "0", "894", "-942", "315", "5", "cobaltscar", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("948", "skyshrine", "41", "0", "0", "0", "-326", "425", "43.72", "999", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("949", "skyshrine", "42", "0", "0", "0", "660", "-60", "3.75", "385", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("950", "skyshrine", "43", "0", "0", "0", "-326", "425", "43.72", "999", "kael", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("951", "skyshrine", "44", "0", "0", "0", "-326", "425", "43.72", "999", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("952", "skyshrine", "45", "0", "0", "0", "660", "-60", "3.75", "0", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("953", "skyshrine", "46", "0", "0", "0", "1198", "1231", "3.75", "999", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("954", "skyshrine", "50", "0", "0", "0", "-157", "663", "333", "254", "skyshrine", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("962", "soldungb", "6", "0", "0", "0", "-398", "999999", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("963", "soldungb", "7", "0", "0", "0", "999999", "-284", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("964", "soldungb", "8", "0", "0", "0", "-570", "999999", "21", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("965", "soldungb", "9", "0", "0", "0", "-712", "780", "-49", "256", "soldungc", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("966", "soldungb", "10", "0", "0", "0", "-454", "999999", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("967", "soldungb", "11", "0", "0", "0", "999999", "-361", "999999", "999", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("968", "soldungc", "1", "0", "0", "0", "-475", "-996", "-23", "175", "soldungb", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("969", "soldungc", "2", "0", "0", "0", "-566", "-986", "2", "425", "soldunga", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("974", "veksar", "2", "0", "0", "0", "-59", "-340", "-190", "128", "lakeofillomen", "0", "0", "0", "0");
INSERT INTO zone_points VALUES("976", "warrens", "3", "0", "0", "0", "-876", "712", "-33", "107", "paineel", "0", "0", "0", "0");
