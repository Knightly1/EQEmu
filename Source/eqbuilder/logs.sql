CREATE TABLE `logs` (
`zone` varchar(16) NOT NULL default '',
`name` varchar(128) NOT NULL default '',
`type` int(10) unsigned NOT NULL default '0',
KEY `zone` (`zone`),
KEY `name` (`name`)
) TYPE=MyISAM;