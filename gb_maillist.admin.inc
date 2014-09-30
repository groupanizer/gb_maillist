<?php
  
  module_load_include('inc', 'g3core', 'g3core.utils');
  
  /** Form callback for notifications settings form */
  function gb_maillist_settings_form($form, &$form_state) {
    $form = array(
      'toggle' => array(
        '#type' => 'fieldset',
        '#title' => 'Disable E-mail',
        'gb_maillist_outgoing_disabled' => array(
          '#type' => 'checkbox',
          '#title' => t('Disable outgoing e-mail'),
          '#description' => t('If set the mailing lists will not send e-mail'),
          '#default_value' => variable_get('gb_maillist_outgoing_disabled'),
        ),
        'gb_maillist_incoming_disabled' => array(
          '#type' => 'checkbox',
          '#title' => t('Disable incoming e-mail'),
          '#description' => t('If set the mailing lists will not accept incoming e-mail'),
          '#default_value' => variable_get('gb_maillist_outgoing_disabled'),
        ),
      ),
    );
    
    if (g3core_user_is_admin()) {
      $form['logging'] = array(
        '#type' => 'fieldset',
        '#title' => 'Logging',
        'gb_maillist_log_outgoing' => array(
          '#type' => 'checkbox',
          '#title' => t('Log outgoing e-mail'),
          '#description' => t('If set the mailing lists will log outgoing e-mail'),
          '#default_value' => variable_get('gb_maillist_log_outgoing'),
        ),
        'gb_maillist_log_incoming' => array(
          '#type' => 'checkbox',
          '#title' => t('Log incoming e-mail'),
          '#description' => t('If set the mailing lists will log incoming e-mail'),
          '#default_value' => variable_get('gb_maillist_log_incoming'),
        ),
      );
    }
    
    return system_settings_form($form);
  }
