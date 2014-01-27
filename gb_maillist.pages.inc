<?php

  /** Page to show the emails of a list to the admin */   
  function gb_maillist_view_list_page($tid) {
    if (!is_numeric($tid)) {
      return t('Invalid tid specified');
    }
    
    $variables = array(
      'header' => array(t('E-mail')),
      'empty' => t("No mailing list associated with tid {$tid}"),
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
    
    global $base_url;
    
    // Render e-mails
    $n = count($addresses);
    $translatable1 = t("{$n} e-mail(s) in the mailing list");
    $translatable2 = t('Export to CSV');
    
    $output = "<p>{$translatable1}</p>\r\n";
    $output .= theme_table($variables) . "\r\n";// theme('table', $variables);
    $output .= "<a href =\"{$base_url}/g/comm/maillist/export/{$tid}\">" . 
        "{$translatable2}</a>";
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
      return t("No mailing list associated with tid {$tid}");
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
      return t("No mailing list associated with tid {$tid}");
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
    global $base_url;
    global $user;
    
    $output = '';
    
    dpm($user);
    
    $is_admin = FALSE;
    if (in_array('Communications', $user->roles) ||
        in_array('Site Admin', $user->roles) ||
        $user->uid == 1) {
      $is_admin = TRUE;
    }

    $variables = array(
      'header' => array(t('Forum'), t('Topics'), t('Posts'), t('Last post')),
      'empty' => t("There are currently no forums."),
      'rows' => array(),
      'attributes' => array(),
      'caption' => '',
      'colgroups' => array(),
      'sticky' => FALSE,
    );
    
    if ($is_admin) {
      array_unshift($variables['header'], t('Notifications'));
      array_push($variables['header'], t('Actions'));
    }
    
    $forum_term = forum_forum_load(0);
    dpm($forum_term);
    foreach ($forum_term->forums as $forum) {
      $forum_name = '';
      $num_topics = '';
      $num_posts = '';
      $last_post_info = '';
      $header = FALSE;
      
      if (property_exists($forum, 'container') && $forum->container) {
        $forum_name .= "{$forum->name}";
        
        $row = array(
          'no_striping' => FALSE,
          'data' => array(
            array(
              'data' => $forum_name,
              'header' => TRUE,
              'colspan' => $is_admin ? 5 : 4,
            ),
          ),
        );
        
        if ($is_admin) {
          $uri = "{$base_url}/g/comm/maillist/forums" .
              "#overlay=admin/structure/forum/edit/container/{$forum->tid}";
          $row['data'][] = array(
            'header' => TRUE, 
            'data' => "<a href=\"{$uri}\">EDIT</a>",
          );
        }
      } else { // not a container
        $furi = "{$base_url}/forum/{$forum->tid}";
        $forum_name .= "<a href=\"{$furi}\">$forum->name</a>";
        
        $num_topics = $forum->num_topics;
        $num_posts = $forum->num_posts;
        
        if (!property_exists($forum->last_post, 'created')) {
          $last_post_info = "-";
        } else {
          $uuri = "{$base_url}/user/{$forum->last_post->uid}";
          $uname = $forum->last_post->name;
          $date = format_date($forum->last_post->created, 'short');
          $last_post_info = t("By <a href=\"{$uuri}\">{$uname}</a> on {$date}");
        }
        
        $row = array(
          'no_striping' => FALSE,
          'data' => array(
            $forum_name, 
            $num_topics, 
            $num_posts, 
            $last_post_info,
          ),
        );
        
        if ($is_admin) {
          /** @todo insert notifications */
          array_unshift($row['data'], 'PLACEHOLDER');
          
          /** @todo insert actions */
          $uri = "{$base_url}/g/comm/maillist/forums" .
              "#overlay=admin/structure/forum/edit/forum/{$forum->tid}";
         $row['data'][] = "<a href=\"{$uri}\">EDIT</a>";
        }
      } 

      $variables['rows'][] = $row;
    }
    
    $output .= theme_table($variables);
    return $output;
  }

  /** @todo View Forum Permission */
  /** @todo delete forum re-directs to admin/structure/forum */
  
