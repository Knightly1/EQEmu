CREATE TABLE quest_globals (
  id int(11) NOT NULL auto_increment,
  charid int(11) NOT NULL default 0,
  npcid int(11) NOT NULL default 0,
  zoneid int(11) NOT NULL default 0,
  name varchar(65) NOT NULL,
  value varchar(65) NOT NULL default "?",
  expdate int(11) NOT NULL default 0,
  PRIMARY KEY  (id),
  UNIQUE KEY qname (name,charid,npcid,zoneid)
) TYPE=MyISAM;

