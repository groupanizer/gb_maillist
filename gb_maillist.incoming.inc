<?php
  
  require_once DRUPAL_ROOT . '/sites/all/modules/g3/g3core/g3core.utils.inc';
  
  /** Deprecated */
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
      $boundary = $matches[1];
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
    preg_match_all("/([\w\$\?\^\+\{\}\|\-\*\.\/!#%&'=~]+)@([\w\-]+(\.[a-z]+)+)/",
        $string, $matches);
    return array(
      'emails' => $matches[0],
      'aliases' => $matches[1],
      'domain names' => $matches[2],
      'top-level domains' => $matches[3],
      'count' => count($matches[1]),
    );
  }
  
  function _save_attachments_and_generate_file_fields($nid, $email, $account) {
    $file_fields = array();
    
    for ($i = 0; $i < count($email['attachments']); $i++) {
      $attachment = $email['attachments'][$i];
      
      $filename = $attachment['filename'];
      
      // Save file data
      $destination = "private://attachments/{$nid}/";
      // Make sure the directory exists and is writable
      file_prepare_directory($destination, 
          FILE_MODIFY_PERMISSIONS | FILE_CREATE_DIRECTORY);
      $destination .= $filename;
      if ($uri = file_unmanaged_save_data($attachment['content'], 
          $destination, FILE_EXISTS_RENAME)) {
        // Create and save file object
        $file = new stdClass();
        $file->fid = NULL;
        $file->uri = $uri;
        $file->filename = $filename;
        $file->filemime = $attachment['filemime'];
        $file->uid = $account->uid;
        $file->status = FILE_STATUS_PERMANENT;
    
        $file = file_save($file);
        
        $file_fields[$i] = array(
          'fid' => $file->fid,
          'display' => 1, /** @todo investigate */
          'description' => '',
          'uid' => $account->uid,
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

  function _post_new_topic($tid, $email, $account) {
    try {
      // Create & save node object
      $node = new stdClass();
      $node->is_new = TRUE;
      $node->type = 'forum';
      node_object_prepare($node);         // sets some defaults & calls hooks
            
      $node->uid = $account->uid;
      $node->name = $account->name;
      $node->picture = $account->picture;
      $node->comment = 2;                 // 0 = disabled, 1 = RO, 2 = R/W
      $node->title = $email['headers']['Subject'];
      $node->status = 0;   // 0 = unpublished, 1 = published
      //$node->language = LANGUAGE_NONE
      $node->taxonomy_forums[LANGUAGE_NONE][0]['tid'] = $tid;
      $node->body[LANGUAGE_NONE][0]['format'] = variable_get(
          'gb_maillist_filter_format');
      $node->body[LANGUAGE_NONE][0]['summary'] = '';
      // MimeMail creates the element html even for plaintext e-mails
      // and wraps the text in <p> tags.
      $node->body[LANGUAGE_NONE][0]['value'] = $email['html'];
      $node->validated = TRUE;
      node_save($node);
      // Now we have a node id and can write the attachments
      // into the proper directory structure
      $node = node_load($node->nid);
      $file_fields = _save_attachments_and_generate_file_fields(
          $node->nid, $email, $account);
      for ($i = 0; $i < count($file_fields); $i++) {
        $node->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      // Re-save
      $node->status = NODE_PUBLISHED; // publish the node
      node_save($node);
      if (count($file_fields) > 0) {
        // Because we're updating a field and updating the forum node
        // hook_pre_storage_field_update is getting called again in forum.module
        // which is inserting the forum_index row twice
        // TODO: report, fix, etc       
        db_query('ALTER IGNORE TABLE forum_index ADD UNIQUE INDEX dupidx (nid, tid, created)');
        db_query('ALTER TABLE forum_index DROP INDEX dupidx');
      }
    } catch (Exception $e) {
      watchdog_exception('gb_maillist', $e);
    }
  }

  function _post_comment_to_topic($nid, $email, $account) {
    try {
      // Create & save comment obj
      $comment = new stdClass();
      $comment->nid = $nid;
      $comment->node_type = 'comment_node_forum';
      $comment->cid = 0;
      $comment->pid = 0;
      $comment->uid = $account->uid;
      $comment->name = $account->name;
      $comment->picture = $account->picture;
      $comment->is_anonymous = 0; /** @todo change when actually anonymous? */
      $comment->status = COMMENT_PUBLISHED;
      //$comment->language = LANGUAGE_NONE
      $comment->subject = db_query('SELECT title FROM {node} WHERE nid = :nid', 
          array(':nid' => $nid))->fetchField();
      $comment->comment_body[LANGUAGE_NONE][0]['format'] = variable_get(
          'gb_maillist_filter_format');
      $comment->comment_body[LANGUAGE_NONE][0]['value'] = $email['html'];
      $file_fields = _save_attachments_and_generate_file_fields(
          $nid, $email, $account);
      for ($i = 0; $i < count($file_fields); $i++) {
        $comment->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      comment_submit($comment);
      comment_save($comment);
    } catch (Exception $e) {
      watchdog_exception('gb_maillist', $e);
    } 
  }

  function _post_to_forum($tid, $alias, $email, $account) {
    $domain = g3core_get_domain();
    
    // Try and locate the forum topic via In-Reply-To: <$tid.$nid@$domain>
    if (isset($email['headers']['In-Reply-To'])) {
      $inreplyto = $email['headers']['In-Reply-To'];
      
      $matches = array();
      // nid.tid@domain
      if (preg_match("/^<([\d]+).([\d]+)@{$domain}>.*$/", 
          $inreplyto, $matches)) {
        $inreplyto_nid = $matches[1];
        $inreplyto_tid = $matches[2];
        if ($inreplyto_tid == $tid) { // this check is to prevent spoofing
          $query = new EntityFieldQuery();
          $result = $query->entityCondition('entity_type', 'node')
              ->entityCondition('bundle', 'forum')
              ->propertyCondition('nid', $inreplyto_nid)
              ->fieldCondition('taxonomy_forums', 'tid', $inreplyto_tid)
              ->execute();
          if ($result && count($result)) {
            return _post_comment_to_topic($inreplyto_nid, $email, $account); 
          }
        }
      }
    }
          
    // Try and locate the forum topic via the subject
    $subject = $email['headers']['Subject'];
    $subject = preg_replace("/^(RE:\s|FWD:\s)?(\[{$alias}\]\s)?/i", 
        '', $subject);
    
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'forum')
        ->propertyCondition('title', $subject)
        ->fieldCondition('taxonomy_forums', 'tid', $tid)
        ->execute();
    if ($result && count($result)) {
      $node = reset($result['node']); // take the first match
      return _post_comment_to_topic($node->nid, $email, $account);
    }    
   
    // Couldn't find the topic!
    _post_new_topic($tid, $email, $account);
  }
  
  /** Implements hook_incoming_mail */
  function gb_maillist_incoming_mail(&$email, $raw) {
    $logging_enabled = variable_get('gb_maillist_log_incoming', FALSE);
    $conditional_log = function($message) use ($logging_enabled) {
      if ($logging_enabled) {
        watchdog('gb_maillist', $message);
      }
    };
    
    $to = $email['headers']['To']; /** @todo 'Delivered-To' ? */
    $from = $email['headers']['From'];
    $reply_to = $email['headers']['Reply-To'];
    
    $conditional_log("New incoming {$from} -> {$to}");
    
    if (variable_get('gb_maillist_incoming_disabled', FALSE)) {
      $conditional_log('Rejected e-mail due to policy');
      return MAIL_ERROR_PERMISSION_DENIED;
    } 
    
    // Parse the alias & from
    $parsed_to = _parse_string_for_emails($to);
    $alias = $parsed_to['aliases'][0];
    
    $parsed_from = _parse_string_for_emails($from);
    $sender = $parsed_from['emails'][0];
    
    $parsed_reply_to = _parse_string_for_emails($reply_to);
    $reply_to = $parsed_reply_to['emails'][0];
    
    // Does a forum with alias $alias exist?
    $record = _check_for_alias_record($alias);
    if (!$record) {
      $conditional_log("E-mail rejected, forum does not exist {$alias}");
      return MAIL_ERROR_USER_NOT_FOUND;
    }
    $tid = $record->tid;
    
    // Does the user exist?
    $account = user_load_by_mail($sender);
    if (!$account) {           // no, so we'll try and proceed as anoynmous
      $account = user_load(0); // anonymous
      
      // Since the user is anonymous lets append something to the e-mail body
      // to let people know who this e-mail came from
      $encoded_subject = rawurlencode('RE: ' . $email['headers']['Subject']);
      $mailto = "<a href=\"mailto:{$sender}?subject={$encoded_subject}\"". 
          ">{$sender}</a>";
      $email['html'] = "{$mailto} (anonymous user) sent:</strong><br /><br />" .
          $email['html'];
    }
    
    // Check for posting permission
    if (!forum_access_access('create', $tid, $account)) {
      $conditional_log("E-mail rejected, user does not have" .
          " permission ({$sender}, {$account->uid})");
      return MAIL_ERROR_PERMISSION_DENIED;
    }

    // Maillist depends on the global $user object
    global $user;
    $user = $account;
    
    _post_to_forum($tid, $alias, $email, $account);
    
    return TRUE; // success
  }
