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
    $t1 = t("{$n} emails in the mailing list");
    $t2 = t('Export to CSV');
    
    $output = "<p>{$t1}</p>\r\n";
    $output .= theme_table($variables) . "\r\n";// theme('table', $variables);
    $output .= "<a href =\"{$base_url}/mailing-list/export/{$tid}\">{$t2}</a>";
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
  
