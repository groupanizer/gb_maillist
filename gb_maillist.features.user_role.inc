<?php
/**
 * @file
 * gb_maillist.features.user_role.inc
 */

/**
 * Implements hook_user_default_roles().
 */
function gb_maillist_user_default_roles() {
  $roles = array();

  // Exported role: Forum Admin.
  $roles['Forum Admin'] = array(
    'name' => 'Forum Admin',
    'weight' => 9,
    'machine_name' => 'forum_admin',
  );

  return $roles;
}
