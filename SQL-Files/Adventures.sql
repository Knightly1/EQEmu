CREATE TABLE `adventures` (
  `QuestID` int(11) NOT NULL default '0',
  `NPCID` int(11) NOT NULL default '0',
  `Type` tinyint(4) NOT NULL default '0',
  `Text` text NOT NULL,
  `Objetive` int(11) NOT NULL default '0',
  `ObjetiveValue` int(11) NOT NULL default '0',
  `Minutes` int(11) NOT NULL default '0',
  `Points` int(11) NOT NULL default '0',
  `ShowCompass` tinyint(1) NOT NULL default '0',
  `zoneid` int(11) NOT NULL default '0',
  `zonedungeonid` int(11) NOT NULL default '0',
  `X` float NOT NULL default '0',
  `Y` float NOT NULL default '0',
  `status` int(11) NOT NULL default '0',
  `char1` int(11) NOT NULL default '0',
  `char2` int(11) NOT NULL default '0',
  `char3` int(11) NOT NULL default '0',
  `char4` int(11) NOT NULL default '0',
  `char5` int(11) NOT NULL default '0',
  `char6` int(11) NOT NULL default '0',
  `in_use` tinyint(1) NOT NULL default '0'
) TYPE=MyISAM; 

CREATE TABLE `adventures_maintext` (
  `NPCID` int(11) NOT NULL default '0',
  `Text` text NOT NULL
) TYPE=MyISAM; 

alter table zone add column ldondungeon bool not null default 0;