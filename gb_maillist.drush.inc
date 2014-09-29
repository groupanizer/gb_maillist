<?php
  
  /** Implements hook_drush_command */
  function gb_maillist_drush_command() {
    return array(
      'setup_boards' => array(
        'description' => dt('Drush command for setuping up initial boards'),
        'aliases' => array('g3defboards'),
      ),
    );
  }
  
  /** Implements hook_drush_help */
  function gb_maillist_drush_help($section) {
    switch ($section){
    case 'drush:setup_boards':
        return dt('Sets up the default message boards/mailing lists');
    }
  }

  function drush_gb_maillist_setup_boards() {
    $roles = db_select('role', 'r')
      ->fields('r', array('rid', 'machine_name'))
      ->condition('rid', 1,'>')
      ->execute()
      ->fetchAllAssoc('machine_name');

    $create_board = function($name, $alias, $role, $item, 
        $parent = 0) use ($roles) {
      $tid = gb_maillist_insert_new_forum($name, '', $parent);
      
      db_insert('gb_maillist_metadata')
        ->fields(array(
          'tid' => $tid,
          'list_type' => _MAILING_LIST_TYPE_STANDARD,
          'reply_to_type' => _STANDARD_REPLY_TO_TYPE_MAILING_LIST,
          'reply_to_email' => NULL,
          'from_email' => NULL,
        ))
        ->execute();
      
      db_insert('gb_maillist_items')
        ->fields(array(
          'tid' => $tid,
          'module' => $item['module'],
          'class' => $item['class'],
          'item' => $item['item'],
      ))
      ->execute();
      
      db_insert('gb_maillist_aliases')
        ->fields(array(
          'tid' => $tid,
          'alias' => $alias,
        ))
        ->execute();
      
      db_insert('forum_access')
        ->fields(array(
          'tid', 
          'rid', 
          'grant_view', 
          'grant_update', 
          'grant_delete', 
          'grant_create',
          'priority',
      ))
      ->values(array(
        'tid' => $tid,
        'rid' => $role->rid,
        'grant_view'   => 1,
        'grant_update' => 0,
        'grant_delete' => 0,
        'grant_create' => 1,
        'priority'     => 0,
      ))
      ->values(array(
        'tid' => $tid,
        'rid' => $roles['forum_admin']->rid,
        'grant_view'   => 1,
        'grant_update' => 1,
        'grant_delete' => 1,
        'grant_create' => 1,
        'priority'     => 0,
      ))
      ->values(array(
        'tid' => $tid,
        'rid' => $roles['site_admin']->rid,
        'grant_view'   => 1,
        'grant_update' => 1,
        'grant_delete' => 1,
        'grant_create' => 1,
        'priority'     => 0,
      ))
      ->execute();
      
      acl_create_new_acl('forum_access', NULL, $tid);
    };
    
    // Create a default set of forums
    $create_board(t('Members'), t('members'), $roles['member'], array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_member',
    ));
    
    $create_board(t('Music Team'), t('musicteam'), $roles['music_team_member'], array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_music_team',
    ));
    
    $create_board(t('Board Members'), t('board'), $roles['board_member'], array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_executive',
    ));
    
    // Load sections vid
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'taxonomy_vocabulary')
      ->propertyCondition('machine_name', 'sections')
      ->execute();
    $vocab = array_shift($result['taxonomy_vocabulary']);
    // Load the terms
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'taxonomy_term')
      ->propertyCondition('vid', $vocab->vid)
      ->execute();
    $tids = array_keys($result['taxonomy_term']);
    $terms = entity_load('taxonomy_term', $tids);
    
    $tid = gb_maillist_insert_new_forum_container(t('Voice parts'));
    
    foreach ($terms as $term) {
      $item = array(
        'module' => 'gs_member',
        'class' => 'gs_member_sections',
        'item' => "gs_member_section_{$term->tid}",
      );
      $create_board($term->name, strtolower($term->name), 
          $roles['member'], $item, $tid);
    }
  }
