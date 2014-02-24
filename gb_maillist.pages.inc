<?php

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
    $output .= "<a href =\"{$base}/g/comm/maillist/export/{$tid}\">" . 
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
    $user = _get_user();
    $base = _get_base_url();
    $domain = _get_domain();
    
    $is_admin = FALSE;
    if (in_array('Communications', $user->roles) ||
        in_array('Site Admin', $user->roles) ||
        in_array('Executive', $user->roles) ||
        $user->uid == 1) {
      $is_admin = TRUE;
    }
        
    if ($is_admin) {
      $mxhosts = array();
      if (!getmxrr($domain, $mxhosts)) {
        drupal_set_message(t("No MX records were found for {$domain}." .
            " Until this is resolved you will be unable to receive e-mail to" . 
            " this site. Contact your DNS provider/host for assistance."), 
            'warning');
      }
    }
    
    $action_markup = function($tid, $admin, $container) use ($base) {
      $actions = array(
        array(
          'path' => 'forum/%',
          'name' => t('View'),
          'admin' => FALSE,     // requires admin?
          'container' => FALSE, // action for container?
        ),
        array(
          'path' => 'node/add/forum/%',
          'name' => t('Post'),
          'admin' => FALSE,
          'container' => FALSE,
        ),
        array(
          // Admin paths are automatically routed to the overlays it seems
          // no need to add '#overlay=admin/structure/forum/edit/forum/%'
          // to the base URL
          'path' => 'admin/structure/forum/edit/forum/%',
          'name' => t('Edit'),
          'admin' => TRUE,
          'container' => FALSE,
        ),
        array(
          'path' => 'g/comm/maillist/view/%',
          'name' => t('See e-mail(s)'),
          'admin' => TRUE,
          'container' => FALSE,
        ),
        array(
          'path' => 'admin/structure/forum/edit/container/%',
          'name' => t('Edit'),
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
          $name = $action['name'];
            
          $markup .= "<a href=\"{$base}/{$path}\">{$name}</a>";
          if ($i++ < $n) $markup .= '&nbsp;&nbsp;/&nbsp;&nbsp;';
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
          $markup .= "<em>{$alias['alias']}</em>@{$domain}";
          if ($i++ < $n) $markup .= ',<br />';
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
      'header' => array('', t('Board'), t('Topics'), t('Posts'), 
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
              'colspan' => 5,
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
        $name_markup = "<a href=\"{$base}/forum/{$tid}\">{$forum->name}</a>";
        
        $row = array(
          'no_striping' => FALSE,
          'data' => array(
            '', /** @todo icons */
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
  