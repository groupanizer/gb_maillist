<?php

  /** Page to show the emails of a list to the admin */   
  function gb_maillist_view_list_page($tid) {
    $variables = array(
      'header' => array(t('Email')),
      'empty' => 'No mailing list associated with tid ' . $tid,
      'rows' => array(),
      'attributes' => array(),
      'caption' => '',
      'colgroups' => array(),
      'sticky' => FALSE,
    );
    
    $addresses = array();
    
    // If a mailing list is attached add the addresses to output
    $items = _fetch_items_for_tid($tid);
    if (!empty($items)) {
      $addresses = _fetch_outgoing_email_addresses($items);
      if (!empty($addresses)) {
        // Convert each element to an array containing that element 
        $variables['rows'] = array_chunk($addresses, 1);
      }
    }
    
    $output = '<p>' . (string) count($addresses) . '&nbsp;emails in the list.' .
        '</p>' . "\r\n";
    $output .= theme_table($variables); // theme('table', $variables);
    global $base_url;
    $output .= "\r\n" . '<a class="CSV" href ="' . $base_url . 
            '/mailing-list/export/' . $tid . '">Export to CSV</a>';
    return $output;
  }

  /** Page to export the emails of a list to CSV */
  function gb_maillist_export_list_page($tid) {
    // If a mailing list is attached generate the csv
    $items = _fetch_items_for_tid($tid);
    if (empty($items)) {
      return 'No mailing list associated with tid ' . $tid;
    }
    
    $addresses = _fetch_outgoing_email_addresses($items);
      
    drupal_add_http_header('Content-Type', 
        'application/csv; charset="ISO-8859-1"');
    drupal_add_http_header('Content-Disposition', 
        'attachment; filename="emails.csv"');
    
    // Export CSV file
    print('Email' . "\r\n" . implode(",\r\n", $addresses) . "\r\n");
  }
