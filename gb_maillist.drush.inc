<?php

  /**
   * Incoming mail handling for the site using MimeMail
   */

  // These are postfix-specfic errors
  define('EX_SOFTWARE', 70);
  define('EX_NOUSER', 67);
  define('EX_NOPERM', 77);
  
  /** Implements hook_drush_command */
  function gb_maillist_drush_command() {
    return array(
      'gb_maillist' => array(
        'callback' => 'gb_maillist_drush_callback',
        'description' => dt('Drush command for handling incoming mail'),
      )
    );
  }
  
  /** Implements hook_drush_help */
  function gb_maillist_drush_help($section) {
    switch ($section){
    case 'drush:gb_maillist':
        return dt('Handles incoming mail');
    }
  }
  
  /** Implements hook_mimemail_incoming */
  function gb_maillist_mimemail_incoming($email) {
    //
  }
  
  /** Implements hook_mimemail_incoming_alter */
  function gb_maillist_mimemail_incoming_alter($email) {
    //
  }
  
  function _parse_email_for_body($email) {
    /** If a boundary is present then mimemail has failed in its parsing
     *  and ['html'] contains the conents of the e-mail that lay between the
     *  the multipart/mixed boundary. 
     * 
     *  @todo Why does it fail? Multiple boundaries?
     */
    $matches = array();
    $body = $email['html'];
    
    if (preg_match("/Content-Type: multipart\/alternative;[\s]+boundary=\"?" . 
        "([\w'()+,\-.\/\?=:]+)\"?\r?\n/ms", $body, $matches)) {
      print($matches);
      $boundary = $matches[1];
      print("BOUNDARY: {$boundary}\n");
      preg_match_all("/Content-Type: text\/(html|plain); [\w\"\-=]+\s?\s". 
          "(Content-Transfer-Encoding: [\w\"\-]+\s?\s)?\s?\s(.*?)\s" . 
          "\-{2,}{$boundary}/ms", $body, $matches);
      $keys = $matches[1];
      $text = $matches[3];
      // Concatenate like parts
      $types = array_unique($keys);
      foreach ($types as $type) {
        foreach ($keys as $index => $key) {
          if ($key === $type) {
            $textparts[$key] .= $text[$index];
          }
        }
      }
      
      $body = isset($textparts['html']) ? 
          $textparts['html'] : $textparts['plain'];
    } else {
      $body = $email['html']; // mimemail parsed the e-mail properly
    }
    
    return $body;
       
  }
  
  // Extract only the e-mails from any RFC compliant header
  // e.g. Smith, John <(john@cox.net)>
  function _parse_string_for_emails($string) {
    $matches = array();
    preg_match_all("/([\w\$\?\^\+\{\}\|\-\*\/!#%&'=~]+)@[\w\-]+.[a-z]+/",
        $string, $matches);
    return array(
      'emails' => $matches[0],
      'aliases' => $matches[1],
      'count' => count($matches[1]),
    );
  }
  
  function _save_attachments_and_generate_file_fields($user, $email) {
    $file_fields = array();
    
    for ($i = 0; $i < count($email['attachments']); $i++) {
      $attachment = $email['attachments'][$i];
      
      // MimeMail doesn't parse some filenames correctly, so we'll adjust
      $matches = array();
      if (preg_match(
          '/Content-Disposition: attachment; (filename|name)="?([^";]+)"?/',
          $email['parts'][$i], $matches)) {
        $filename = $matches[2];
      } else {
        $filename = $attachment['filename'];
      }
      
      // Save file data
      /** @todo save where? */
      $destination = file_default_scheme() . '://' . $filename;
      if ($uri = file_unmanaged_save_data($attachment['content'], 
          $destination, FILE_EXISTS_RENAME)) {
        // Create and save file object
        $file = new stdClass();
        $file->fid = NULL;
        $file->uri = $uri;
        $file->filename = $filename;
        $file->filemime = $attachment['filemime'];
        $file->uid = $user->uid;
        $file->status = FILE_STATUS_PERMANENT;
    
        $file = file_save($file);
        
        $file_fields[$i] = array(
          'fid' => $file->fid,
          'display' => 1, /** @todo investigate */
          'description' => '',
          'uid' => $user->uid,
          'uri' => $file->uri,
          'filename' => $file->filename,
          'filemime' => $file->filemime,
          'filesize' => $file->filesize,
          'status' => 1, /** either FILE_STATUS_PERMANENT or published.. both 1  
                           * @todo investigate
                           */
          'timestamp' => $file->timestamp,
          'origname' => $filename,
          'rdf_mapping' => NULL,
        );
      }
    }

    return $file_fields;
  }

  function _post_new_topic($tid, $user, $email) {
    try {
      // Create & save node object
      $node = new stdClass();
      $node->is_new = TRUE;
      $node->type = 'forum';
      node_object_prepare($node);         // sets some defaults & calls hooks
      $node->uid = $user->uid;
      $node->name = $user->name;
      $node->picture = $user->picture;
      $node->comment = 2;                 // 0 = disabled, 1 = RO, 2 = R/W
      $node->title = $email['headers']['Subject'];
      $node->status = NODE_PUBLISHED;     // 0 = unpublished, 1 = published
      //$node->language = LANGUAGE_NONE
      $node->taxonomy_forums[LANGUAGE_NONE][0]['tid'] = $tid;
      $node->body[LANGUAGE_NONE][0]['format'] = variable_get(
          'gb_maillist_filter_format');
      $node->body[LANGUAGE_NONE][0]['summary'] = '';
      // MimeMail creates the element html even for plaintext e-mails
      // and wraps the text in <p> tags.
      $node->body[LANGUAGE_NONE][0]['value'] = $email['html'];
      $file_fields = _save_attachments_and_generate_file_fields($user->uid, 
          $email);
      for ($i = 0; $i < count($file_fields); $i++) {
        $node->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      node_submit($node);
      node_save($node);
    } catch (Exception $e) {
      watchdog_exception('gb_maillist', $e);
    }
  }

  function _post_comment_to_topic($nid, $user, $email) {
    try {
      // Create & save comment obj
      $comment = new stdClass();
      $comment->nid = $nid;
      $comment->node_type = 'comment_node_forum';
      $comment->cid = 0;
      $comment->pid = 0;
      $comment->uid = $user->uid;
      $comment->name = $user->name;
      $comment->picture = $user->picture;
      $comment->is_anonymous = 0; /** @todo change when actually anonymous? */
      $comment->status = COMMENT_PUBLISHED;
      //$comment->language = LANGUAGE_NONE
      $comment->subject = $email['headers']['Subject'];
      $comment->comment_body[LANGUAGE_NONE][0]['format'] = variable_get(
          'gb_maillist_filter_format');
      $comment->comment_body[LANGUAGE_NONE][0]['value'] = $email['html'];
      $file_fields = _save_attachments_and_generate_file_fields($user->uid, 
          $email);
      for ($i = 0; $i < count($file_fields); $i++) {
        $comment->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      comment_submit($comment);
      comment_save($comment);
    } catch (Exception $e) {
      watchdog_exception('gb_maillist', $e);
    }
    
  }

  function _post_to_forum($tid, $user, $email) {
    
    /** @todo check for in-reply-to */
    
    /** @todo check subject */
    
    if (0) {
      _post_comment_to_topic($tid, $user, $email);
    } else {
      _post_new_topic($tid, $user, $email);
    }
  }
  
  /** @todo check multi email */
  /** @todo send multi email, in spreadsheet/requirements*/
  function gb_maillist_drush_callback() {
    if (!module_exists('gb_maillist')) {
      exit(EX_SOFTWARE);
    }
    
    module_load_include('inc', 'mimemail', 'includes/mimemail.incoming');
    
    $email = stream_get_contents(STDIN);            // read encoded raw e-mail
    $email = base64_decode($email);                 // decode e-mail
    $email = mimemail_parse($email);                // parse e-mail into array
    $email['html'] = _parse_email_for_body($email); // re-parse to fix bugs
    
    $to = $email['headers']['To']; /** @todo 'Delivered-To' ? */
    $from = $email['headers']['From'];

    watchdog('gb_maillist', "{$from} -> {$to}:\n{$email['html']}", NULL);
    
    // Sanitize
    $parsed_to = _parse_string_for_emails($to);
    $alias = $parsed_to['aliases'][0];
    
    $parsed_from = _parse_string_for_emails($from);
    $sender = $parsed_from['emails'][0];
    
    // Does a forum with alias $alias exist?
    $record = _check_for_alias_record($alias);
    if (!$record) {
      exit(EX_NOUSER);
    }
    $tid = $record->tid;
    
    // Does the user exist?
    $user = user_load_by_mail($sender);
    if (!$user) {           // No, so we'll try and proceed as anoynmous
      $user = user_load(0); // anonymous
    }

    // Check permissions
    if (!forum_access_access('create', $tid, $user)) {
      exit(EX_NOPERM);
    }

    _post_to_forum($tid, $user, $email);
  }
