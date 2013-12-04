<?php

  // see: http://isemail.info/about
  define('_EMAIL_ADDRESS_MAX_LENGTH', 254); 
  
  // see: https://drupal.org/node/1852454
  if (!defined('DRUPAL_EXTENSION_NAME_MAX_LENGTH')) {
    define('DRUPAL_EXTENSION_NAME_MAX_LENGTH', 50);
  }
  
  // see: http://dev.mysql.com/doc/refman/5.0/en/string-type-overview.html
  define('_VARCHAR_MAX_LENGTH', 255);
  
  define('_WATCHDOG', 0);
  define('_TEST_LISTS', 1);
  
  define('_MAILING_LIST_TYPE_STANDARD', 0);
  define('_MAILING_LIST_TYPE_BROADCAST', 1);
  
  define('_REPLY_TO_TYPE_DO_NOT_REPLY', 0);
  define('_REPLY_TO_TYPE_USE_POSTERS_EMAIL', 1);
  define('_REPLY_TO_TYPE_USER_DEFINED', 2);
  define('_REPLY_TO_TYPE_MAILING_LIST', 3);

?>