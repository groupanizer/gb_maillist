<?php
/**
 * @file
 * gb_maillist_feature.features.menu_links.inc
 */

/**
 * Implements hook_menu_default_menu_links().
 */
function gb_maillist_feature_menu_default_menu_links() {
  $menu_links = array();

  // Exported menu link: menu-groupanizer-menu:forum
  $menu_links['menu-groupanizer-menu:forum'] = array(
    'menu_name' => 'menu-groupanizer-menu',
    'link_path' => 'forum',
    'router_path' => 'forum',
    'link_title' => 'Discussions',
    'options' => array(
      'attributes' => array(
        'title' => '',
      ),
    ),
    'module' => 'menu',
    'hidden' => 0,
    'external' => 0,
    'has_children' => 0,
    'expanded' => 0,
    'weight' => 4,
  );
  // Translatables
  // Included for use with string extractors like potx.
  t('Discussions');


  return $menu_links;
}
