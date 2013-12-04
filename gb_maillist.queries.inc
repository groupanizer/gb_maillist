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
        
        drupal_set_message('Caught exception: ' .  $e->getMessage(), 'error');
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
    $items = array();
    
    foreach ($values as $key => $value) {
      // Check if key is has the storage class id prefix
      if (strpos($key, 'scid:') !== 0) {
        continue;
      }
      
      foreach ($value as $item) {
        if (gettype($item) !== 'string') {
          continue;
        }
        
        // Check if element has the item id prefix
        if (strpos($item, 'iid:') !== 0) {
          continue;
        }
        
        // We have an item
        // IID:MODULE:CLASS:ITEM
        $pieces = explode(':', $item);
        array_shift($pieces);
        
        /**
         * @todo in PHP > 5.5
         * yield array(
         *   'module' => $pieces[0],
         *   'class' => $pieces[1],
         *   'item' => $pieces[2]
         * );
         */
      
        $module = $pieces[0];
        $class = $pieces[1];
        $item = $pieces[2];
        
        $items[] = array(
          'tid' => $tid,
          'module' => $module,
          'class' => $class,
          'item' => $item,
        );
      } // foreach ($value as $item)
    } // foreach ($values as $key => $value)
    
    return $items;
  }
  
  function _delete_all_list_records_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      $result = db_delete('gb_maillist_metadata')
        ->condition('tid', $tid)
        ->execute();
      // $result = n rows deleted, 0 is still success
        
      $result = db_delete('gb_maillist_items')
        ->condition('tid', $tid)
        ->execute();
      // $result = n rows deleted, 0 is still success
      
      return TRUE;
    });
  }
  
  /**
   * @todo InsertQuery::execute return value is undefined for multi-inserts.
   *       How can we check for success?
   */
  function _build_and_insert_list_records_for_tid($tid, &$values) {
    return _safe_closure(function () use ($tid, $values) {
      // We are attaching a list
      // build and write our query for gb_maillist_metadata
      // db_merge will handle update & insert
      $result = db_merge('gb_maillist_metadata')
        ->key(array('tid' => $tid))
        ->fields(array(
          'list_type' => $values['list_type'],
          'reply_to_type' => $values['reply_to_type'],
          'reply_to_email' => $values['reply_to_email'],
          'from_email' => $values['from_email'],
        ))
        ->execute();
      // $result = MergeQuery::STATUS_UPDATE or MeryQuery::STATUS_INSERT
      
      // Delete the old items from the database
      $result = db_delete('gb_maillist_items')
        ->condition('tid', $tid)
        ->execute();
      // $result = n rows deleted, 0 is still success
      
      // Build the multi-insert query
      $query = db_insert('gb_maillist_items')
        ->fields(array('tid', 'module', 'class', 'item'));
      
      // Insert each record into the query
      foreach (_each_item_record($values, $tid) as $record) {
        $query->values($record);
      }
      
      // Perform the query
      $n = $query->execute();
      // $n = ?
      
      return TRUE;
    });
  }
  
  function _fetch_list_metadata_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      // Do we have metadata attached to this forum?
      $result = db_select('gb_maillist_metadata', 'meta')
        ->fields('meta')
        ->condition('tid', $tid, '=')
        ->execute()
        ->fetchAssoc();
      
      return $result;
    });
  }
  
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
  
  function _insert_blacklist_record($tid, $email) {
    return _safe_closure(function () use ($tid, $email) {
      $result = db_insert('gb_maillist_blacklist')
        ->fields(array(
          'tid' => $tid,
          'email' => $email,
        ))
        ->execute();
        // $result = last insert ID of the query
        
        return TRUE;
    });
  }
  
  function _fetch_matching_blacklist_record($tid, $email) {
    return _safe_closure(function () use ($tid, $email) {
      $result = db_select('gb_maillist_blacklist', 'black')
        ->fields('black')
        ->condition('tid', $tid, '=')
        ->condition('email', $email, '=')
        ->execute()
        ->fetch();
        
        return $result;
    });
  }
  
  function _fetch_blacklisted_emails_for_tid($tid) {
    return _safe_closure(function () use ($tid) {
      return $query = db_select('gb_maillist_blacklist', 'black')
        ->fields('black', array('email'))
        ->condition('tid', $tid, '=')
        ->execute()
        ->fetchCol();
    });
  }
  
  