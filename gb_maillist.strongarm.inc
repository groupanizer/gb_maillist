<?php
/**
 * @file
 * gb_maillist.strongarm.inc
 */

/**
 * Implements hook_strongarm().
 */
function gb_maillist_strongarm() {
  $export = array();

  $strongarm = new stdClass();
  $strongarm->disabled = FALSE; /* Edit this to true to make a default strongarm disabled initially */
  $strongarm->api_version = 1;
  $strongarm->name = 'content_access_forum';
  $strongarm->value = array(
    'view_own' => array(
      0 => 57539251,
      1 => 36427370,
    ),
    'view' => array(
      0 => 57539251,
      1 => 36427370,
    ),
  );
  $export['content_access_forum'] = $strongarm;

  return $export;
}
