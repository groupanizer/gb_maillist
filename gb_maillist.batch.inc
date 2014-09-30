<?php

  /** Callback called by e-mail batch operation */
  function _dispatch_email($dispatcher, $email, &$context) {
    $result = $dispatcher->dispatch($email);
    
    $context['results'][] = array('result' => $result, 'to' => $email->to);
    $context['message'] = $email->to;
  }
  
  /** Callback called at the end of the e-mail batch operation */
  function _dispatch_email_finished($success, $results, $operations) {
    $log = variable_get('gb_maillist_log_outgoing', FALSE);

    if ($success) {
      // Count successful dispatches and aggregate e-mails
      $dispatched = 0;
      $recepients = array('succeeded' => array(), 'failed' => array());
      
      $results[] = array('result' => FALSE, 'to' => 'fuckyou');
      
      foreach ($results as $result) {
        if ($result['result']) {
          $recepients['succeeded'][] = $result['to'];
          
          $dispatched += 1;
        } else {
          $recepients['failed'][] = $result['to'];
        }
      }
      
      $message = t('Dispatched @count of @total e-mail.', 
          array('@count' => $dispatched, '@total' => count($results)));
      drupal_set_message($message, $dispatched == count($results) ? 
          'status' : 'warning');
      
      if ($log) {
        watchdog('gb_maillist', $message);
        
        $message = t('Succeeded in sending e-mail to: @arguments', array(
            '@arguments' => print_r($recepients['succeeded'], TRUE)));
        watchdog('gb_maillist', $message);
        
        if (count($recepients['failed'])) {
          $message = t('Failed to send e-mail to: @arguments', array(
              '@arguments' => print_r($recepients['failed'], TRUE)));
          watchdog('gb_maillist', $message, array(), WATCHDOG_WARNING);
        }
        
      }
    } else {
      if ($log) {
        // An error occurred.
        // $operations contains the operations that remained unprocessed.
        $error_operation = reset($operations);
        $message = t('An error occurred while processing %error_operation ' .
            'with arguments: @arguments', array(
            '%error_operation' => $error_operation[0], 
            '@arguments' => print_r($error_operation[1], TRUE)));
        watchdog('gb_maillist', $message, array(), WATCHDOG_ERROR);
      }
    }
  }
