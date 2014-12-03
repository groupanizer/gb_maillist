<?php

  // see: http://isemail.info/about
  define('_EMAIL_ADDRESS_MAX_LENGTH', 254);
  define('_ALIAS_MAX_LENGTH', 64);  // local part max length is 64

  // see: https://drupal.org/node/1852454
  if (!defined('DRUPAL_EXTENSION_NAME_MAX_LENGTH')) {
    define('DRUPAL_EXTENSION_NAME_MAX_LENGTH', 50);
  }
  
  // legacy, 
  // see: http://dev.mysql.com/doc/refman/5.0/en/string-type-overview.html
  define('_VARCHAR_MAX_LENGTH', 255);
  
  // list types
  define('_MAILING_LIST_TYPE_STANDARD', 0);
