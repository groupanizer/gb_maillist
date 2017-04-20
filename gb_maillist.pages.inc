<?php

  /** Page to show the original e-mail source to admins */ 
  function gb_maillist_view_source_page($tid, $nid, $cid) {
    if (!is_numeric($tid)) {
      return t('Invalid tid specified');
    }
    if (!is_numeric($nid)) {
      return t('Invalid nid specified');
    }
    if (!is_numeric($cid) && !is_null($cid)) {
      return t('Invalid cid specified');
    }
    
    if (!$cid) $cid = NULL;
    
    $source = _fetch_received_email_source($tid, $nid, $cid);
    if (!$source) {
      return t('This message has no incoming e-mail source. It was probably' .
          ' posted through the message boards.');
    }
     
    drupal_add_http_header('Content-Type', 
        'text/plain; charset="ISO-8859-1"');
    drupal_add_http_header('Content-Disposition', 
        'attachment; filename="source.txt"');
    print($source);
    drupal_exit();
  }

  /** Page to show the emails of a list to the admin */   
  function gb_maillist_view_list_page($tid) {
    if (!is_numeric($tid)) {
      return t('Invalid tid specified');
    }
    
    $variables = array(
      'header' => array(t('E-mail')),
      'empty' => t("No mailing list associated with tid '{$tid}'"),
      'rows' => array(),
      'attributes' => array(),
      'caption' => '',
      'colgroups' => array(),
      'sticky' => FALSE,
    );
    
    // If a mailing list is attached add the addresses to output
    $addresses = array();
    $items = _fetch_list_items_for_tid($tid);
    if (!empty($items)) {
      $addresses = _fetch_outgoing_email_addresses($tid, $items);
      if (!empty($addresses)) {
        // Convert each element to an array containing that element 
        $variables['rows'] = array_chunk($addresses, 1);
      }
    }
    
    // Render e-mails
    $text = t('There are @count e-mail(s) in the mailing list',
        array('@count' => count($addresses)));
    $output = "<br /><p>{$text}</p>";
    $output .= theme_table($variables);
    $output .= '<br />';
    $output .= l(t('Export to CSV'), "g/comm/maillist/export/{$tid}");
    return $output;
  }

  /** Page to export the emails of a list to CSV */
  function gb_maillist_export_list_page($tid) {
    if (!is_numeric($tid)) {
      return t('Invalid tid specified');
    }
    
    // If a mailing list is attached generate the CSV output
    $items = _fetch_list_items_for_tid($tid);
    if (empty($items)) {
      return t("No mailing list associated with tid '@tid'",
          array('@tid' => $tid));
    }
    
    $addresses = _fetch_outgoing_email_addresses($tid, $items);
    $list = implode(",\r\n", $addresses); // stringify
     
    drupal_add_http_header('Content-Type', 
        'application/csv; charset="ISO-8859-1"');
    drupal_add_http_header('Content-Disposition', 
        'attachment; filename="emails.csv"');
    print("E-mail\r\n{$list}\r\n");
    drupal_exit();
  }
  
  /** Page to unsubscribe from a list */
  function gb_maillist_unsubscribe_from_list_page($tid, $email) {
    if (!is_numeric($tid)) {
      return t('Invalid tid specified');
    }
    
    $email = urldecode($email);
    $email = filter_var($email, FILTER_SANITIZE_EMAIL);
    $email = filter_var($email, FILTER_VALIDATE_EMAIL);
    if (!$email) {
      return t('Invalid e-mail address');
    }
    
    $result = entity_load('taxonomy_term', array($tid));
    if (empty($result)) {
      return t("No mailing list associated with tid '@tid'",
          array('@tid' => $tid));
    }
    
    /** @todo unsubscribe from $result[$tid]->name */   
    
    return t('Failed to unsubscribe, please contact an administrator');
  }

/**
 * Upgrade message and such
 */
function gb_maillist_upgrade_form() {
  return array( 'upgrade' => array(
    '#type' => 'submit',
    '#value' => "Upgrade now",
  ));
}
function gb_maillist_upgrade_form_submit($form, &$form_state) {
  module_enable(array('gb_mail'));
  drupal_set_message("The new email system has been enabled");
  drupal_set_message("Emails will be migrated over to the new system automatically, over the next 24 hours.");
  drupal_set_message("Please take the following steps immediately, to enable your mail system:<ol><li>Click on <b>Convert old lists</b> below, to migrate your lists over to the new system</li><li>Check that they are configured correctly, make any necessary adjustments</li><li>Click on the <b>Mail Setup</b> tab below and follow the steps to configure your DNS</li></ol>");
  $form_state['redirect'] = 'g/admin/settings/mail';
}



  /** Page to see forum hierarchy and details */
  function gb_maillist_forums_page() {
 
    module_load_include('inc', 'gb_maillist', 'gb_maillist.queries');

    if( user_access('administer email settings') ) {
     $upgrade = drupal_get_form('gb_maillist_upgrade_form');
     drupal_set_message(t("IMPORTANT NOTICE: This is the &quot;old&quot; email system and it will be unsupported on June 1, 2017. Please upgrade to the new system at a convenient time before then. Besides fixing several stubborn bugs, the new system adds new features like integrated spam filtering, open and click tracking, and a cleaner, simpler user interface. Your existing emails and lists will be moved over automatically, but you will have to configure DNS before the system will be fully operational.<br>!form", array('!form'=>drupal_render($upgrade))), 'warning');
    }

    $user = g3core_get_user();
    $domain = g3core_get_domain();
    
    $is_admin = user_access('administer forums');    
    if ($is_admin) {
      // Display MX record warnings (missing or not pointed to us)
      $mxhosts = array();
      if (!getmxrr($domain, $mxhosts)) {
        $message = t('No MX records were found for @domain. Until this is' .
            ' resolved you will be unable to receive e-mail to this site.' .
            'Contact your DNS provider/host for assistance.',
            array('@domain' => $domain));
        drupal_set_message($message, 'warning');
      } else if (array_search('vps605.getcadre.com', $mxhosts) === FALSE &&
                 array_search('mail2.groupanizer.com', $mxhosts) === FALSE) {
        $message = t('No MX records were found for @domain that point to the' .
            ' Groupanizer server. Until this is resolved you will be unable' .  
            ' to receive e-mail to this site. Contact your DNS provider/host' . 
            ' for assistance. (Current records point to @record)',
            array('@domain' => $domain, '@record' => implode(', ', $mxhosts)));
        drupal_set_message($message, 'warning'); 
      }
                         
      // Display warnings if incoming or outgoing mail is disabled
      if (variable_get('gb_maillist_incoming_disabled', FALSE)) {
        $message = t('Incoming e-mail are turned off. You can turn' .
            ' them on <a href="@url">here</a>.',
            array('@url' => url('admin/config/groupanizer/maillists')));
        drupal_set_message($message, 'warning');
      }
      
      if (variable_get('gb_maillist_outgoing_disabled', FALSE)) {
        $message = t('Outgoing e-mail are turned off. You can turn' .
            ' them on <a href="@url">here</a>.',
            array('@url' => url('admin/config/groupanizer/maillists')));
        drupal_set_message($message, 'warning');
      }
      
    }
    
    $action_markup = function($tid, $admin, $container) {
      $actions = array(
        array(
          'path' => 'node/add/forum/%',
          'class' => 'g-maillist-post',
          'title' => t('Post'),
          'admin' => FALSE,
          'container' => FALSE,
        ),
        array(
          'path' => 'admin/structure/forum/edit/forum/%',
          'class' => 'g-maillist-edit',
          'title' => t('Edit'),
          'admin' => TRUE,
          'container' => FALSE,
        ),
        array(
          'path' => 'g/comm/maillist/view/%',
          'class' => 'g-maillist-export',
          'title' => t('View & export e-mail list'),
          'admin' => TRUE,
          'container' => FALSE,
        ),
        array(
          'path' => 'admin/structure/forum/edit/container/%',
          'class' => 'g-maillist-edit',
          'title' => t('Edit'),
          'admin' => TRUE,
          'container' => TRUE,
        ),
      );
     
      $i = 0; 
      // Count of applicable $actions
      $n = array_reduce($actions, function ($n, $action) use ($container) {
        return ($action['container'] === $container) ? ++$n : $n;
      }, -1);
      // Generate an action markup string
      $markup = '';
      foreach ($actions as $action) {
        if (($action['admin'] && $admin || !$action['admin']) && 
            ($action['container'] === $container)) {
          $options = array(
            'attributes' => array(
              'title' => $action['title'], 
              'class' => array($action['class'],),
            ),
          );
          $markup .= l('', str_replace('%', (string) $tid, $action['path']), 
              $options);
          if ($i++ < $n) $markup .= '&nbsp;';
          
        }
      }
            
      return $markup;
    };
    
    $alias_markup = function ($tid) use ($domain) {
      $markup = '<br />';
      
      $aliases = _fetch_list_aliases_for_tid($tid);
      $n = count($aliases);
      for ($i = 0; $i < $n; $i++) {
        $alias = $aliases[$i]['alias'];
        $markup .= l("<em>{$alias}</em>@{$domain}", "mailto:{$alias}@{$domain}",
            array('html' => TRUE));
        if ($i + 1 < $n) $markup .= ',<br />';
      }
      
      return $markup;
    };
    
    $last_post_markup = function ($forum) {
      if (property_exists($forum->last_post, 'created')) {
        return t('By <a href="@url">@name</a> on @date',
            array('@url' => url("user/{$forum->last_post->uid}"),
            '@name' => $forum->last_post->name, 
            '@date' => format_date($forum->last_post->created, 'short')));     
      }
      
      return '-';
    };
    
    $variables = array(
      'header' => array(
        t('Board'), 
        array('data'=>t('Topics'), 'class'=>array('coldrop')),
        array('data'=>t('Posts'), 'class'=>array('coldrop')),
        array('data'=>t('Last post'), 'class'=>array('coldrop')),
        t('Actions')),
      'empty' => t("There are currently no forums."),
      'rows' => array(),
      'attributes' => array(),
      'caption' => '',
      'colgroups' => array(),
      'sticky' => FALSE,
    );

    $forum_term = forum_forum_load(0);
    foreach ($forum_term->forums as $forum) {
      if (property_exists($forum, 'container') && $forum->container) {
        $row = array(
          'no_striping' => FALSE,
          'data' => array(
            array(
              'data' => $forum->name,
              'header' => TRUE,
              'colspan' => $is_admin ? 4 : 5,
            ),
          ),
        );
        
        if ($is_admin) {
          $row['data'][] = array(
            'header' => TRUE, 
            'data' => $action_markup($forum->tid, $is_admin, TRUE),
          );
        }
      } else { // not a container
        $name_markup = l("<strong>{$forum->name}</strong>", 
            "forum/{$forum->tid}", array('html' => TRUE));
        
        $row = array(
          'no_striping' => FALSE,
          'data' => array(
            $name_markup . $alias_markup($forum->tid),
            array('data'=> $forum->num_topics, 'class'=>array('coldrop')),
            array('data'=> $forum->num_posts, 'class'=>array('coldrop')),
            array('data'=> $last_post_markup($forum), 'class'=>array('coldrop')),
            $action_markup($forum->tid, $is_admin, FALSE),
          ),
        );
      }

      $variables['rows'][] = $row;
    }
    
    $output = '';
    $output .= theme_table($variables);
    return $output;
  }

  /** @todo fix that delete forum re-directs to admin/structure/forum */
  
