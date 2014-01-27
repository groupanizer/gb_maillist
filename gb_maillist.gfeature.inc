<?php


  /** 
   * Implements hook_gfeature() 
   * @todo finish 
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
  
   /** Implements hook_context_widgets */
  function gb_maillist_context_widgets() {
    return array(
      'g/comm/maillist' => array( 
        'view' => array(
          'title' => t('Discussion Boards'), 
          'href' => 'g/comm/maillist/board'
        ),
        'help' => array(
          'title '=> t('Maillist Help'), 
          'href' => 'http://support.groupanizer.com' 
        ),
      ),
    );
  }
