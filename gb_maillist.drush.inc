<?php
  
  require_once DRUPAL_ROOT . '/sites/all/modules/g3/g3core/g3core.utils.inc';

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
      'incoming_mail' => array(
        'description' => dt('Drush command for handling incoming mail'),
        'aliases' => array('g3incomingmail'),
      ),
      'setup_boards' => array(
        'description' => dt('Drush command for setuping up initial boards'),
        'aliases' => array('g3defboards'),
      ),
    );
  }
  
  /** Implements hook_drush_help */
  function gb_maillist_drush_help($section) {
    switch ($section){
    case 'drush:incoming_mail':
        return dt('Handles incoming mail. cat email | drush g3incoming');
    case 'drush:setup_boards':
        return dt('Sets up the default message boards/mailing lists.');
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
      watchdog('gb_maillist', 'else');
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
      
      /*
      // MimeMail doesn't parse some filenames correctly, so we'll adjust
      $matches = array();
      if (preg_match(
          '/Content-Disposition: attachment; (filename|name)="?([^";]+)"?/',
          $email['parts'][$i], $matches)) {
        $filename = $matches[2];
      } else {
        $filename = $attachment['filename'];
      }*/
      
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
      watchdog('gb_maillist', 'post new topic');
      watchdog('gb_maillist', print_r($email, TRUE));
      
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
      $node->status = NODE_PUBLISHED;     // 0 = unpublished, 1 = published
      //$node->language = LANGUAGE_NONE
      $node->taxonomy_forums[LANGUAGE_NONE][0]['tid'] = $tid;
      $node->body[LANGUAGE_NONE][0]['format'] = variable_get(
          'gb_maillist_filter_format');
      $node->body[LANGUAGE_NONE][0]['summary'] = '';
      // MimeMail creates the element html even for plaintext e-mails
      // and wraps the text in <p> tags.
      $node->body[LANGUAGE_NONE][0]['value'] = $email['html'];
      $node->validated = TRUE;
            watchdog('gb_maillist', 'first save');
      node_save($node);
      // Now we have a node id and can write the attachments
      // into the proper directory structure
      $node = node_load($node->nid);
      watchdog('gb_maillist', print_r($node, TRUE));
      $file_fields = _save_attachments_and_generate_file_fields(
          $node->nid, $email, $account);
      for ($i = 0; $i < count($file_fields); $i++) {
        $node->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      // Re-save
      watchdog('gb_maillist', "second save: " .  count($file_fields));
      node_save($node);
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
      watchdog('gb_maillist', 'inreplyto: ' . $inreplyto);
      
      $matches = array();
      // nid.tid@domain
      if (preg_match("/^<([\d]+).([\d]+)@{$domain}>.*$/", 
          $inreplyto, $matches)) {
        $inreplyto_nid = $matches[1];
        $inreplyto_tid = $matches[2];
        watchdog('gb_maillist', '_tid: ' . $inreplyto_tid . ' _nid: ' . $inreplyto_nid);
        watchdog('gb_maillist', 'passed tid: ' . $tid);
        if ($inreplyto_tid == $tid) { // this check is to prevent spoofing
        watchdog('gb_maillist', 'tid matches');
          $query = new EntityFieldQuery();
          $result = $query->entityCondition('entity_type', 'node')
              ->entityCondition('bundle', 'forum')
              ->propertyCondition('nid', $inreplyto_nid)
              ->fieldCondition('taxonomy_forums', 'tid', $inreplyto_tid)
              ->execute();
          if ($result && count($result)) {
            watchdog('gb_maillist', 'post comment in reply to');
            return _post_comment_to_topic($inreplyto_nid, $email, $account); 
          }
        }
      }
    }
          
    // Try and locate the forum topic via the subject
    $subject = $email['headers']['Subject'];
    $subject = preg_replace("/^(RE:\s|FWD:\s)?(\[{$alias}\]\s)?/i", 
        '', $subject);
    watchdog('gb_maillist', 'preg replace subject: ' . $subject);
    
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'forum')
        ->propertyCondition('title', $subject)
        ->fieldCondition('taxonomy_forums', 'tid', $tid)
        ->execute();
    if ($result && count($result)) {
      watchdog('gb_maillist', 'post comment subject');
      $node = reset($result['node']); // take the first match
      return _post_comment_to_topic($node->nid, $email, $account);
    }    
   
    // Couldn't find the topic!
    _post_new_topic($tid, $email, $account);
  }
  
  /** @todo check multi email */
  /** @todo send multi email, in spreadsheet/requirements*/
  function drush_gb_maillist_incoming_mail() {
    if (!module_exists('gb_maillist')) {
      exit(EX_SOFTWARE);
    }
    
    module_load_include('inc', 'mimemail', 'includes/mimemail.incoming');
    
    $email = stream_get_contents(STDIN);            // read encoded raw e-mail
    $email = base64_decode($email);                 // decode e-mail
    $email = mimemail_parse($email);                // parse e-mail into array
    // $email['html'] = _parse_email_for_body($email); // re-parse to fix bugs
    
    $to = $email['headers']['To']; /** @todo 'Delivered-To' ? */
    $from = $email['headers']['From'];
    $reply_to = $email['headers']['Reply-To'];

    watchdog('gb_maillist', 
        "Incoming {$from} -> {$to}:\n{$email['html']}", NULL);
    watchdog('gb_maillist', 'Verbose: ' . print_r($email, TRUE));
    
    // Parse the alias & from
    $parsed_to = _parse_string_for_emails($to);
    $alias = $parsed_to['aliases'][0];
    
    $parsed_from = _parse_string_for_emails($from);
    $sender = $parsed_from['emails'][0];
    
    $parsed_reply_to = _parse_string_for_emails($reply_to);
    $reply_to = $parsed_reply_to['emails'][0];
    
    // If the e-mail is from our own domain, there is an erroneous loop
    $domain = g3core_get_domain();
    if ($parsed_from['domain names'][0] == $domain ||
        $parsed_reply_to['domain names'][0] == $domain) {
      watchdog('gb_maillist', 'Terminated potential infinite loop');
      return; // Don't bounce nor reply
    }
    
    // Does a forum with alias $alias exist?
    $record = _check_for_alias_record($alias);
    if (!$record) {
      watchdog('gb_maillist', "Mail rejected: forum does not exist {$alias}");
      exit(EX_NOUSER);
    }
    $tid = $record->tid;
    
    // Does the user exist?
    $account = user_load_by_mail($sender);
    if (!$account) {           // no, so we'll try and proceed as anoynmous
      $account = user_load(0); // anonymous
    }
    
    // Check for posting permission
    if (!forum_access_access('create', $tid, $account)) {
      watchdog('gb_maillist', "Mail rejected: user does not have permission {$sender}, {$account->uid}");
      exit(EX_NOPERM);
    }

    // Maillist depends on the global $user object
    global $user;
    $user = $account;
    
    _post_to_forum($tid, $alias, $email, $account);
  }

  function drush_gb_maillist_setup_boards() {
    $roles = db_select('role', 'r')
      ->fields('r', array('rid', 'machine_name'))
      ->condition('rid', 1,'>')
      ->execute()
      ->fetchAllAssoc('machine_name');

    $create_board = function($name, $alias, $role, $item, 
        $parent = 0) use ($roles) {
      $tid = gb_maillist_insert_new_forum($name, '', $parent);
      
      db_insert('gb_maillist_metadata')
        ->fields(array(
          'tid' => $tid,
          'list_type' => _MAILING_LIST_TYPE_STANDARD,
          'reply_to_type' => _STANDARD_REPLY_TO_TYPE_MAILING_LIST,
          'reply_to_email' => NULL,
          'from_email' => NULL,
        ))
        ->execute();
      
      db_insert('gb_maillist_items')
        ->fields(array(
          'tid' => $tid,
          'module' => $item['module'],
          'class' => $item['class'],
          'item' => $item['item'],
      ))
      ->execute();
      
      db_insert('gb_maillist_aliases')
        ->fields(array(
          'tid' => $tid,
          'alias' => $alias,
        ))
        ->execute();
      
      db_insert('forum_access')
        ->fields(array(
          'tid', 
          'rid', 
          'grant_view', 
          'grant_update', 
          'grant_delete', 
          'grant_create',
          'priority',
      ))
      ->values(array(
        'tid' => $tid,
        'rid' => $role->rid,
        'grant_view'   => 1,
        'grant_update' => 0,
        'grant_delete' => 0,
        'grant_create' => 1,
        'priority'     => 0,
      ))
      ->values(array(
        'tid' => $tid,
        'rid' => $roles['forum_admin']->rid,
        'grant_view'   => 1,
        'grant_update' => 1,
        'grant_delete' => 1,
        'grant_create' => 1,
        'priority'     => 0,
      ))
      ->values(array(
        'tid' => $tid,
        'rid' => $roles['site_admin']->rid,
        'grant_view'   => 1,
        'grant_update' => 1,
        'grant_delete' => 1,
        'grant_create' => 1,
        'priority'     => 0,
      ))
      ->execute();
      
      acl_create_new_acl('forum_access', NULL, $tid);
    };
    
    // Create a default set of forums
    $create_board(t('Members'), t('members'), $roles['member'], array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_member',
    ));
    
    $create_board(t('Music Team'), t('musicteam'), $roles['music_team_member'], array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_music_team',
    ));
    
    $create_board(t('Board Members'), t('board'), $roles['board_member'], array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_executive',
    ));
    
    // Load sections vid
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'taxonomy_vocabulary')
      ->propertyCondition('machine_name', 'sections')
      ->execute();
    $vocab = array_shift($result['taxonomy_vocabulary']);
    // Load the terms
    $query = new EntityFieldQuery();
    $result = $query->entityCondition('entity_type', 'taxonomy_term')
      ->propertyCondition('vid', $vocab->vid)
      ->execute();
    $tids = array_keys($result['taxonomy_term']);
    $terms = entity_load('taxonomy_term', $tids);
    
    $tid = gb_maillist_insert_new_forum_container(t('Voice parts'));
    
    foreach ($terms as $term) {
      $item = array(
        'module' => 'gs_member',
        'class' => 'gs_member_sections',
        'item' => "gs_member_section_{$term->tid}",
      );
      $create_board($term->name, strtolower($term->name), 
          $roles['member'], $item, $tid);
    }
  }
