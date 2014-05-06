<?php

  /** Callback called by e-mail batch operation */
  function _dispatch_email($dispatcher, $email, &$context) {
    $result = $dispatcher->dispatch($email);
    
    $context['results'][] = array('result' => $result, 'to' => $email->to);
    $context['message'] = $email->to;
  }
  
  /** Callback called at the end of the e-mail batch operation */
  function _dispatch_email_finished($success, $results, $operations) {
    if ($success) {
      // Count successful dispatches and log failures
      $dispatched = 0;
      foreach ($results as $result) {
        if ($result['result']) {
          $dispatched += 1;
        } else {
          $message = t('Failed to send e-mail to %to.', 
              array('%to' => $result['to']));
          watchdog('gb_maillist', $message);
        }
      }
      
      $message = t('Dispatched @count of @total e-mail.', 
          array('@count' => $dispatched, '@total' => count($results)));
      drupal_set_message($message);
      watchdog('gb_maillist', $message);
    } else {
      // An error occurred.
      // $operations contains the operations that remained unprocessed.
      $error_operation = reset($operations);
      $message = t('An error occurred while processing %error_operation with' .
          'arguments: @arguments', array(
          '%error_operation' => $error_operation[0], 
          '@arguments' => print_r($error_operation[1], TRUE)));
      watchdog('gb_maillist', $message);
    }
  }
