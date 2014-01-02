<?php

/**
 * Implements hook_gfeature()
 */
function gb_maillist_gfeature() {
  return array(
    'icon' => drupal_get_path('module', 'gs_member') . '/Icon-Members.png',
    'description' => t("Manage and organize your mailing lists"),
    'settings_path' => url('<front>'),
    'help_path' => url('<front>'),
    'learn_path' => url('http://groupanizer.com/feature/mailinglists'),
  );
}
