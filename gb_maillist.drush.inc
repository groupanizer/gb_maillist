<?php
  
  require_once DRUPAL_ROOT . '/sites/all/modules/g3/g3core/g3core.utils.inc';

  /**
   * Incoming mail handling for the site using MimeMail
   */

  // These are postfix-specfic errors
  define('EX_DATAERR', 65);
  define('EX_NOUSER', 67);
  define('EX_SOFTWARE', 70);
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
  
  // Extract only the e-mails from any RFC compliant header
  // e.g. Smith, John <(john@cox.net)>
  function _parse_string_for_emails($string) {
    $matches = array();
    preg_match_all(
        "/([\w\$\?\^\+\{\}\|\-\*\.\/!#%&'=~]+)@([\w\-]+(\.[a-z]+)+)/",
        $string, $matches);
    return array(
      'emails' => $matches[0],
      'aliases' => $matches[1],
      'domain names' => $matches[2],
      'top-level domains' => $matches[3],
      'count' => count($matches[1]),
    );
  }
  
  /**
   * Saves the attachments in the e-mail to the file system and post parses
   * the e-mail body to contain the corrected inline image paths if present.
   */
  function _save_attachments_and_parse_inline_images($nid, &$email, $account) {
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
        
        if (stristr($file->filemime, 'image') && $attachment['cid'] != NULL) {
          $email['html'] = str_replace("cid:{$attachment['cid']}", 
              file_create_url($file->uri), $email['html']);
        }
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
      $node->title = $email['headers']['subject'] ?
          $email['headers']['subject'] : t('(no subject)');
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
      $file_fields = _save_attachments_and_parse_inline_images(
          $node->nid, $email, $account);
      for ($i = 0; $i < count($file_fields); $i++) {
        $node->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      // Re-save
      $node->body[LANGUAGE_NONE][0]['value'] = $email['html']; // Updated
      $node->status = NODE_PUBLISHED; // publish the node
      node_save($node);
      if (count($file_fields) > 0) {
        // Because we're updating a field and updating the forum node
        // hook_pre_storage_field_update is getting called again in forum.module
        // which is inserting the forum_index row twice
        // TODO: report, fix, etc       
        db_query('ALTER IGNORE TABLE forum_index' .
            ' ADD UNIQUE INDEX dupidx (nid, tid, created)');
        db_query('ALTER TABLE forum_index DROP INDEX dupidx');
      }
      
      return $node->nid;
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
      $file_fields = _save_attachments_and_parse_inline_images(
          $nid, $email, $account);
      $comment->comment_body[LANGUAGE_NONE][0]['value'] = $email['html'];
      for ($i = 0; $i < count($file_fields); $i++) {
        $comment->field_attachment[LANGUAGE_NONE][$i] = $file_fields[$i];
      }
      comment_submit($comment);
      comment_save($comment);
      
      return $comment->cid;
    } catch (Exception $e) {
      watchdog_exception('gb_maillist', $e);
    } 
  }

  function _post_to_forum($tid, $alias, $source, $email, $account) {
    $domain = g3core_get_domain();
    
    // Try and locate the forum topic via In-Reply-To: <$tid.$nid@$domain>
    if (isset($email['headers']['in-reply-to'])) {
      $inreplyto = $email['headers']['in-reply-to'];
      
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
            $cid = _post_comment_to_topic($inreplyto_nid, $email, $account);
            if ($cid) {
              _insert_received_email_record($tid, $inreplyto_nid, $cid, 
                  $source, $email['html']);
            }
            return; 
          }
        }
      }
    }
          
    // Try and locate the forum topic via the subject
    $subject = $email['headers']['subject'] ?
          $email['headers']['subject'] : t('(no subject)');
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
      $cid = _post_comment_to_topic($node->nid, $email, $account);
      if ($cid) {
        _insert_received_email_record($tid, $node->nid, $cid, 
            $source, $email['html']);
      }
      return;
    }    
   
    // Couldn't find the topic!
    $nid = _post_new_topic($tid, $email, $account);
    if ($nid) {
      _insert_received_email_record($tid, $nid, NULL, $source, $email['html']);
    }
  }

  function _check_for_illegal_attachments($email) {
    if (!empty($email['attachments'])) {
      $executable_mimetypes = array(
        'application/x-msdownload',     // microsoft executable
        'application/octet-stream',     // binary file
        'application/x-msdos-program',  // dos executable
        'application/x-dosexec',        // dos executable
        'application/java',             // java applet
        'application/javascript',       // javascript
        'application/x-executable',     // generic executable
      );
    
      // Read allowed file extensions from forum attachment field instance
      $info = field_info_instance('node', 'field_attachment', 'forum');
      $extensions = explode(' ', $info['settings']['file_extensions']);
      
      mb_internal_encoding('utf-8');
      
      foreach ($email['attachments'] as $attachment) {  
        // Check for executable mimetypes
        if (in_array(strtolower($attachment['filemime']), 
            $executable_mimetypes)) {
          return "Invalid MIME type '{$attachment['filemime']}'";
        }
        
        // Check if file extension is allowed
        $extension = pathinfo(strtolower($attachment['filename']), 
            PATHINFO_EXTENSION);
        if (!in_array($extension, $extensions)) {
          return "Invalid file extension '{$extension}'"; 
        }

        // Check for PE file
        // could theoritically give false positives in very unlikely situations
        // where a text file contains the exact bytes in order with a valid
        // offset
        if (is_string($attachment['content']) && 
            !ctype_print($attachment['content'])) {
          $content = $attachment['content'];
          
          // Check for DOS Signature which is a WORD 0x5A4D ('MZ')
          // at offset 0 of the IMAGE_DOS_HEADER
          $e_magic = reset(unpack('S*', mb_strcut($content, 0, 2, 'UTF-8')));
          if ($e_magic == 0x5A4D) {
            // Read $e_lfanew which is the offset to the NT headers
            // which is a LONG located at 0x3C of the IMAGE_DOS_HEADER structure
            $e_lfanew = reset(unpack('l*', 
                mb_strcut($content, 0x3C, 4, 'UTF-8')));
            
            // Check the IMAGE_NT_HEADERS offset for the PE Signature
            // which is a DWORD at offset 0 of the structure
            // should be 0x00004550 ('PE\0\0')
            $Signature = reset(unpack('L*', 
                mb_strcut($content, $e_lfanew, 4, 'UTF-8')));
            if ($Signature == 0x4550) {;
              return 'An executable file has been detected';
            }
          } 
        }
      }
    }
    
    return FALSE;
  }
  
  function drush_gb_maillist_incoming_mail() {
    if (variable_get('gb_maillist_incoming_disabled', FALSE)) {
      watchdog('gb_maillist', 'Rejecting incoming mail because incoming' .
          ' mail is disabled.');
      exit(EX_NOPERM);
    }
    
    watchdog('gb_maillist', 'Accepting incoming e-mail.');
    
    $file_uri = stream_get_contents(STDIN);
    // Hack.. OSX 'echo' utility claims to support -n and '\c' but annoyingly
    // it supports neither...
    if (substr($file_uri, -2) == "\x03\n") {
      $file_uri = substr($file_uri, 0, strlen($file_uri) - 2);
    }
    
    // Check if e-mail is too large
    if (filesize($file_uri) > variable_get('gb_maillist_email_max_size')) {
      watchdog('gb_maillist', 'Mail rejected: e-mail is too large.');
      exit(EX_DATAERR);
    }
    
    // Read filename from STDIN as it was piped in
    $source = file_get_contents($file_uri);
    module_load_include('inc', 'mimemail', 'includes/mimemail.incoming');
    $email = mimemail_parse($source); // parse e-mail into array

    $to = isset($email['headers']['delivered-to']) ? 
        $email['headers']['delivered-to'] : 
        $email['headers']['to'];
    $from = $email['headers']['from'];
    $reply_to = $email['headers']['reply-to'];

    watchdog('gb_maillist', "Incoming {$from} -> {$to}.");
    
    // Parse the alias & from
    // TODO: need to loop and choose the right one for multiple 'to' e-mail
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
      watchdog('gb_maillist', 'Terminated potential infinite loop.');
      return; // Don't bounce nor reply
    }

    // Does a forum with alias $alias exist?
    $record = _check_for_alias_record($alias);
    if (!$record) {
      watchdog('gb_maillist', "Mail rejected: forum does not exist {$alias}.");
      exit(EX_NOUSER);
    }
    $tid = $record->tid;
    
    // Try and load a user from the sender's e-mail
    $account = user_load_by_mail($sender);
    if (!$account) { // try and load from multiple_email
      $uid = db_query('SELECT m.uid FROM {multiple_email} m' .
          ' WHERE m.email = :email', array(':email' => $sender))->fetchField();
      if ($uid) {
        $account = user_load($uid);
      } 
      if (!$account) { // give up and load anonymous user
        $account = user_load(0);
      }
    }
    
    // Check for posting permission
    if (!forum_access_access('create', $tid, $account)) {
      watchdog('gb_maillist', "Mail rejected: user does not have permission " .
          "{$sender}, {$account->uid}.");
      exit(EX_NOPERM);
    }
    
    $result = _check_for_illegal_attachments($email);
    if ($result !== FALSE) {
       watchdog('gb_maillist', "Mail rejected:" . 
          " e-mail contains illegal attachment(s). {$result}.");
      exit(EX_DATAERR);     
    }
    
    // Since the user is anonymous lets append something to the e-mail body
    // to let people know who this e-mail came from
    if ($account->uid == 0) {
      $encoded_subject = rawurlencode('RE: ' . $email['headers']['subject']);
      $mailto = "<a href=\"mailto:{$sender}?subject={$encoded_subject}\"". 
          ">{$sender}</a>";
      $email['html'] = "{$mailto} (anonymous user) sent:</strong><br /><br />" .
          $email['html'];
    }
    
    // Maillist depends on the global $user object
    global $user;
    $user = $account;
    
    // Store the parsed body in a global object so we can send it out
    // without putting it through the node filters
    global $_unfiltered_html;
    $_unfiltered_html = $email['html'];
    
    // Post it 
    _post_to_forum($tid, $alias, $source, $email, $account);
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
    $create_board(t('Members'), t('members'), $roles['member'], 
    array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_member',
    ));
    
    $create_board(t('Music Team'), t('musicteam'), $roles['music_team_member'], 
    array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_music_team_member',
    ));
    
    $create_board(t('Board Members'), t('board'), $roles['board_member'], 
    array(
      'module' => 'gb_member',
      'class' => 'gb_member_roles',
      'item' => 'gb_member_role_board_member',
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
