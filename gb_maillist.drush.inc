<?php
  // These are postfix-specfic errors
  define('EX_SOFTWARE', 70);
  define('EX_NOUSER', 67);
  define('EX_NOPERM', 77);
  
  /** Implements hook_drush_command */
  function gb_maillist_drush_command() {
    return array(
      'gb_maillist' => array(
        'callback' => 'gb_maillist_drush_callback',
        'description' => dt('Drush command for handling incoming mail'),
      )
    );
  }
  
  /** Implements hook_drush_help */
  function gb_maillist_drush_help($section ){
    switch ($section){
    case 'drush:gb_maillist':
        return dt('Handles incoming mail');
    }
  }
  
  function gb_maillist_drush_callback() {;
    if (!module_exists('gb_maillist')) {
      exit(EX_SOFTWARE);
    }
    
    $email = stream_get_contents(STDIN);
    print($email);
    
    exit(0);
    return 0;
  }
