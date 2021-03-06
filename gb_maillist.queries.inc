<?php

  /**
   * Wrapper class for handling exceptions in a function (Closure).
   * 
   * @todo object state isn't multithreaded / concurrency safe.
   *       is it even possible for the an object's method (or any function)
   *       to be called from multiple places at the same time in PHP?
   * @api
   */
  class SafeClosure {
    private $_closure;
    
    private /* readonly */ $caught = FALSE;
    private /* readonly */ $result = FALSE;
    
    /**
     * @param Closure $closure the function to wrap
     */
    public function __construct(Closure $closure) {
      $this->_closure = $closure;
    }
    
    public function call() {
      try {
        $closure = $this->_closure;
        
        $this->result = $closure();
        $this->caught = FALSE;
      } catch (Exception $e) {
        $this->caught = TRUE;
        $this->result = FALSE;
        
        watchdog_exception('gb_maillist', $e);
      }
      
      return $this->result;
    }
    
    public function __invoke() {
      return $this->call();
    }
    
    public function __get($property) {
      if (property_exists($this, $property)) {
        return $this->{$property};
      }
    }
  }
  
  function _safe_closure(Closure $closure) {
    $safe_closure = new SafeClosure($closure);
    return $safe_closure(); // __invoke()
  }

  // Build an array of items for insertion into the database
  function _each_item_record($values, $tid) {
    $records = array();
    
    $list_classes = _extract_values_with_keys_that_match_pattern(
        $values, '/^scid:.*$/');
    foreach ($list_classes as $list_class) {
      foreach ($list_class as $item_key => $item_value) {
        if ($item_key === $item_value) { // checked if value === key
          // IID:MODULE:CLASS:ITEM
          $pieces = explode(':', $item_value);
          array_shift($pieces);
          
          /**
           * @todo in PHP > 5.5
           * yield array(
           *   'module' => $pieces[0],
           *   'class' => $pieces[1],
           *   'item' => $pieces[2]
           * );
           */
          $records[] = array(
            'tid' => $tid,
            'module' => $pieces[0],
            'class' => $pieces[1],
            'item' => $pieces[2]
          );
        }
      }
    }
    
    return $records;
  }

  // Build an array of aliases for insertion into the database
  function _each_alias_record($values, $tid) {
    $records = array();
    
    $aliases = $values['verified_aliases'];
    foreach ($aliases as $alias) {
      $records[] = array('tid' => $tid, 'alias' => $alias);
    }
    
    return $records;
  }
  
  /** @return boolean */
  function _delete_all_list_records_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      db_delete('gb_maillist_metadata')
        ->condition('tid', $tid)
        ->execute();

      db_delete('gb_maillist_items')
        ->condition('tid', $tid)
        ->execute();
      
      db_delete('gb_maillist_aliases')
        ->condition('tid', $tid)
        ->execute();
        
     db_delete('gb_maillist_received_email')
      ->condition('tid', $tid)
      ->execute();
        
      
      return TRUE;
    });
  }
  
  /**
   * @todo InsertQuery::execute return value is undefined for multi-inserts.
   *       How can we check for success?
   * 
   * @return boolean
   */
  function _build_and_merge_list_records_for_tid($tid, &$values) {
    return _safe_closure(function () use ($tid, $values) {
      // We are attaching a list
      // build and write our query for gb_maillist_metadata
      // db_merge will handle update & insert
      db_merge('gb_maillist_metadata')
        ->key(array('tid' => $tid))
        ->fields(array(
          'list_type' => $values['list_type'],
          'prefix' => $values['prefix'],
        ))
        ->execute();
      // MergeQuery::STATUS_UPDATE or MeryQuery::STATUS_INSERT
      
      db_delete('gb_maillist_items')
        ->condition('tid', $tid)
        ->execute();
      db_delete('gb_maillist_aliases')
        ->condition('tid', $tid)
        ->execute();
      
      // Build the multi-insert queries
      $query = db_insert('gb_maillist_items')
        ->fields(array('tid', 'module', 'class', 'item'));
      foreach (_each_item_record($values, $tid) as $record) {
        $query->values($record);
      }
      $query->execute();
      
      // Insert aliases
      $query = db_insert('gb_maillist_aliases')
        ->fields(array('tid', 'alias'));
      foreach (_each_alias_record($values, $tid) as $record) {
        $query->values($record);
      }
      $query->execute();
      
      return TRUE;
    });
  }
  
  /** @return a metadata record */
  function _fetch_list_metadata_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      // Do we have metadata attached to this forum?
      return db_select('gb_maillist_metadata', 'meta')
        ->fields('meta')
        ->condition('tid', $tid, '=')
        ->execute()
        ->fetchAssoc();
    });
  }
  
  /** @return an array of item records */
  function _fetch_list_items_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      $query = db_select('gb_maillist_items', 'items')
        ->fields('items')
        ->condition('tid', $tid, '=')
        ->execute();
        
      $result = array();
      $row = $query->fetchAssoc();     // first
      if ($row) {
        do {
          $result[] = $row;
          
          $row = $query->fetchAssoc(); // next
        } while ($row);
      }
      
      return empty($result) ? FALSE : $result;
    });
  }
  
  /** @return an array of alias records */
  function _fetch_list_aliases_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      $query = db_select('gb_maillist_aliases', 'a')
        ->fields('a')
        ->condition('tid', $tid, '=')
        ->execute();
        
      $result = array();
      $row = $query->fetchAssoc();     // first
      if ($row) {
        do {
          $result[] = $row;
          
          $row = $query->fetchAssoc(); // next
        } while ($row);
      }
      
      return empty($result) ? FALSE : $result;
    });
  }
  
  /** @return boolean test can be used */
  function _check_for_alias_record($alias, $tid_to_exclude = NULL) {
    return _safe_closure(function () use ($alias, $tid_to_exclude) {
      $query = db_select('gb_maillist_aliases', 'a')
        ->fields('a')
        ->condition('alias', $alias, '=');
        
      if (!is_null($tid_to_exclude)) {
        $query->condition('tid', $tid_to_exclude, '!=');
      }
        
      return $query->execute()->fetch();
    });
  }
  
  /** @return boolean */
  function _insert_received_email_record($tid, $nid, $cid, $source, $html) {
    return _safe_closure(function () use ($tid, $nid, $cid, $source, $html) {
      db_insert('gb_maillist_received_email')
        ->fields(array(
          'tid' => $tid,
          'nid' => $nid,
          'cid' => $cid,
          'source' => $source,
          'html' => $html,
        ))
        ->execute();
        
      return TRUE;
    });
  }
  
  /** @return boolean test can be used */
  function _check_for_received_email_record($tid, $nid, $cid = NULL) {
    return _safe_closure(function () use ($tid, $nid, $cid) {
      $query = db_select('gb_maillist_received_email', 'r')
        ->fields('r', array('tid'))
        ->condition('tid', $tid, '=')
        ->condition('nid', $nid, '=');

      if (is_null($cid)) {
        $query->isNull('cid');
      } else {
        $query->condition('cid', $cid, '=');
      }
        
      return $query->execute()->fetch();
    });
  }

  /** @return the source text of the received e-mail */
  function _fetch_received_email_source($tid, $nid, $cid = NULL) {
    return _safe_closure(function () use ($tid, $nid, $cid) {
      $query = db_select('gb_maillist_received_email', 'r')
        ->fields('r', array('source'))
        ->condition('tid', $tid, '=')
        ->condition('nid', $nid, '=');
        
      if (is_null($cid)) {
        $query->isNull('cid');
      } else {
        $query->condition('cid', $cid, '=');
      }
        
      return $query->execute()->fetchField();
    });
  }  