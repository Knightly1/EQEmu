#
# Table structure for table 'bugs'
#

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