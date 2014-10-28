<?php

  module_load_include('inc', 'g3core', 'g3core.utils');
  

  /** Page to show the original e-mail source to admins */ 
  function gb_maillist_view_source_page($tid, $nid, $cid) {
    if (!is_numeric($tid)) {
      return t('Invalid tid specified');
    }
    if (!is_numeric($nid)) {
      return t('Invalid nid specified');
    }
    if (!is_numeric($cid) && $cid !== 'NULL') {
      return t('Invalid cid specified');
    }
    
    if ($cid == 0 || $cid === 'NULL') $cid = NULL;
    $source = db_query('SELECT r.source FROM {gb_maillist_received_email} r' .
        ' WHERE r.tid = :tid AND r.nid = :nid AND r.cid = :cid', 
        array(':tid' => $tid, ':nid' => $nid, ':cid' => $cid,))->fetchField();
    if (!$source) {
      return t('Oops...');
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
    $base = _get_base_url();
    $n = count($addresses);
    $count_text = t("There are {$n} e-mail(s) in the mailing list");
    $export_text = t('Export to CSV');
    
    $output = "<br /><p>{$count_text}</p>";
    $output .= theme_table($variables) . "\r\n"; // theme('table', $variables);
    $output .= "<br /><a href =\"{$base}/g/comm/maillist/export/{$tid}\">" . 
        "{$export_text}</a>";
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
      return t("No mailing list associated with tid '{$tid}'");
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
      return t("No mailing list associated with tid '{$tid}'");
    }
    
    $name = $result[$tid]->name;
    $text = t("{$email} has been unsubscribed from the '{$name}' mailing list");
    $already = t('already');
    
    // Already blacklisted?
    if (_check_for_matching_blacklist_record($tid, $email)) {
      return "{$text}&nbsp;<em>{$already}</em>";
    }
    
    // Unsubscribe
    if (!_insert_blacklist_record($tid, $email)) {
      return t('Failed to unsubscribe, please contact an administrator');
    }
    
    return $text;
  }
  
  /** Page to see forum hierarchy and details */
  function gb_maillist_forums_page() {
    $user = g3core_get_user();
    $base = _get_base_url();
    $domain = g3core_get_domain();
    
    $is_admin = user_access('administer forums');
        
    if ($is_admin) {
      // Display MX record warnings (missing or not pointed to us)
      $mxhosts = array();
      if (!getmxrr($domain, $mxhosts)) {
        drupal_set_message(t("No MX records were found for {$domain}." .
            " Until this is resolved you will be unable to receive e-mail to" . 
            " this site. Contact your DNS provider/host for assistance."), 
            'warning');
      } else if (array_search('vps605.getcadre.com', $mxhosts) === FALSE &&
                 array_search('mail2.groupanizer.com', $mxhosts) === FALSE) {
        drupal_set_message(t("No MX records were found for {$domain}" .
            " that point to the Groupanizer server." .
            " Until this is resolved you will be unable to receive e-mail to" . 
            " this site. Contact your DNS provider/host for assistance."), 
            'warning'); 
      }
                         
      // Display warnings if incoming or outgoing mail is disabled
      if (variable_get('gb_maillist_incoming_disabled', FALSE)) {
        drupal_set_message(
            t('Incoming e-mail are turned off. You can turn them on !here.', 
            array('!here' => l('here', 'admin/config/groupanizer/maillists'))), 
            'warning');
      }
      
      if (variable_get('gb_maillist_outgoing_disabled', FALSE)) {
        drupal_set_message(
            t('Outgoing e-mail are turned off. You can turn them on !here.', 
            array('!here' => l('here', 'admin/config/groupanizer/maillists'))), 
            'warning');
      }
      
    }
    
    $action_markup = function($tid, $admin, $container) use ($base) {
      $actions = array(
        array(
          'path' => 'node/add/forum/%',
          'class' => 'g-maillist-post',
          'title' => t('Post'),
          'admin' => FALSE,
          'container' => FALSE,
        ),
        array(
          // Admin paths are automatically routed to the overlays it seems
          // no need to add '#overlay=admin/structure/forum/edit/forum/%'
          // to the base URL
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
      // count of applicable $actions
      $n = array_reduce($actions, function ($n, $action) use ($container) {
        return ($action['container'] === $container) ? ++$n : $n;
      }, -1);
      // reduce $actions to single markup string
      return array_reduce($actions, function ($markup, $action) 
          use ($admin, $container, $base, $tid, $n, &$i) {
        if (($action['admin'] && $admin || !$action['admin']) && 
            ($action['container'] === $container)) {
          $path = str_replace('%', (string) $tid, $action['path']);
          $class = $action['class'];
          $title = $action['title'];
            
          $markup .= "<a href=\"{$base}/{$path}\" title=\"{$title}\"" .
              " class=\"{$class}\"></a>";
          if ($i++ < $n) $markup .= '&nbsp;';
        }
            
        return $markup;
      });
    };
    
    $alias_markup = function ($tid) use ($domain) {
      $aliases = _fetch_list_aliases_for_tid($tid);
      if (!empty($aliases)) {
        $i = 0;
        $n = count($aliases) - 1;
        return array_reduce($aliases, function ($markup, $alias) 
            use ($domain, $n, &$i) {
          $alias = $alias['alias'];
          $markup .= "<a href=\"mailto:{$alias}@{$domain}\">" . 
              "<em>{$alias}</em>@{$domain}";
          if ($i++ < $n) $markup .= ',<br />';
          $markup .= '</a>';
          return $markup;
        }, '<br />');
      }
      
      return '';
    };
    
    $last_post_markup = function ($forum) use ($base) {
      $markup = '-';
      
      if (property_exists($forum->last_post, 'created')) {
        $url = "{$base}/user/{$forum->last_post->uid}";
        $name = $forum->last_post->name;
        $date = format_date($forum->last_post->created, 'short');
        
        return t("By <a href=\"{$url}\">{$name}</a> on {$date}");
      }
      
      return $markup;  
    };
    
    $variables = array(
      'header' => array(t('Board'), t('Topics'), t('Posts'), 
          t('Last post'), t('Actions')),
      'empty' => t("There are currently no forums."),
      'rows' => array(),
      'attributes' => array(),
      'caption' => '',
      'colgroups' => array(),
      'sticky' => FALSE,
    );

    $forum_term = forum_forum_load(0);
    foreach ($forum_term->forums as $forum) {
      $tid = $forum->tid;
      
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
            'data' => $action_markup($tid, $is_admin, TRUE),
          );
        }
      } else { // not a container
        $name_markup = "<a href=\"{$base}/forum/{$tid}\">" .
            "<strong>{$forum->name}</strong></a>";
        
        $row = array(
          'no_striping' => FALSE,
          'data' => array(
            $name_markup . $alias_markup($tid),
            $forum->num_topics, 
            $forum->num_posts, 
            $last_post_markup($forum),
            $action_markup($tid, $is_admin, FALSE),
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
  