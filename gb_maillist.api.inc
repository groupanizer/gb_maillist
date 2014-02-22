<?php
  
  function gb_maillist_insert_new_forum_container($name, $description = '', 
      $parent = 0) {
    $vocabulary = taxonomy_vocabulary_load(
        variable_get('forum_nav_vocabulary', 0));
    
    $term = new stdClass();
    $term->name = $name;
    $term->description = $description;
    $term->parent = array($parent);
    $term->vid = $vocabulary->vid;
    taxonomy_term_save($term);
    
    $containers = variable_get('forum_containers', array());
    $containers[] = $term->tid;
    variable_set('forum_containers', $containers);
    
    return $term->tid;
  }
  
  function gb_maillist_insert_new_forum($name, $description = '',
      $parent = 0) {
    $vocabulary = taxonomy_vocabulary_load(
        variable_get('forum_nav_vocabulary', 0));

    $term = new stdClass();
    $term->name = $name;
    $term->description = $description;
    $term->parent = array($parent);
    $term->vid = $vocabulary->vid;
    taxonomy_term_save($term);
    
    return $term->tid;
  }
  
  /**
   * Class for building the descriptor array of each module's mailing lists
   * for the main form
   * 
   * @api
   */
  class DescriptorArrayBuilder implements ArrayAccess {
    private $data = array();
    
    /**
     * This method adds a storage class to the array
     * 
     * @param string $class_id the class id to use
     * @param string $title the title of the storage class
     * @param string $description an optional description of the storage class
     * 
     * @throws Exception if the storage class already exists OR the class id
     * contains illegal characters
     * 
     * @return string the class id
     */
    public function add_storage_class($class_id, $title, $description = NULL) {
      if (strlen($class_id) > _VARCHAR_MAX_LENGTH) {
        throw new Exception('$class_id length exceeds _VARCHAR_MAX_LENGTH');
      }
      if (strlen($title) > _VARCHAR_MAX_LENGTH) {
        throw new Exception('$title length exceeds _VARCHAR_MAX_LENGTH');
      }
      if (strlen($description) > _VARCHAR_MAX_LENGTH) {
        throw new Exception('$description length exceeds _VARCHAR_MAX_LENGTH');
      }
      
      if (!$this->validate_id($class_id)) {
        throw new Exception('$class_id contains illegal characters.');
      }
      if (isset($this->data[$class_id])) {
        throw new Exception("Storage class {$class_id} already exists");
      }
      
      $this->data[$class_id]['title'] = $title;
      $this->data[$class_id]['description'] = $description;
      
      return $class_id;
    }
    
    /**
     * This method adds an item to a storage class
     * 
     * @param string $class_id the class id which the item is added to
     * @param string $item_id the item id to use
     * @param string $title the title of the item
     * @param string $description an optional description of the item
     * 
     * @throws Exception if the storage class doesn't exist OR the item id
     * contains illegal characters
     * 
     * @return void
     */
    public function add_item_to_class($class_id, $item_id, $title, 
        $description = NULL) {
      if (strlen($class_id) > _VARCHAR_MAX_LENGTH) {
        throw new Exception('$class_id length exceeds _VARCHAR_MAX_LENGTH');
      }
      if (strlen($item_id) > _VARCHAR_MAX_LENGTH) {
        throw new Exception('$item_id length exceeds _VARCHAR_MAX_LENGTH');
      }
      if (strlen($description) > _VARCHAR_MAX_LENGTH) {
        throw new Exception('$description length exceeds _VARCHAR_MAX_LENGTH');
      }
      
      if (!$this->validate_id($item_id)) {
        throw new Exception('$item_id contains illegal characters.');
      }
      if (!isset($this->data[$class_id])) {
        throw new Exception("Storage class {$class_id} does not exist.");
      }

      $this->data[$class_id]['items'][$item_id] = array();
      $item = &$this->data[$class_id]['items'][$item_id];
      
      $item['title'] = $title;
      $item['description'] = $description;
    }
    
    /**
     * This method builds and returns the array
     * 
     * @return array
     */
    public function build() {
      return $this->data;
    }
    
    /* BEGIN ArrayAccess */
    public function offsetGet($offset) {
      if (is_null($offset)) {
        return $this->data;
      } else {
        return $this->data[$offset];
      }
    }
    
    public function offsetSet($offset, $value) {
      if (is_null($offset)) {
          $this->data[] = $value;
      } else {
          $this->data[$offset] = $value;
      }
    }
  
    public function offsetExists($offset) {
      return isset($this->data[$offset]);
    }
    
    public function offsetUnset($offset) {
      unset($this->data[$offset]);
    }
    /* END ArrayAccess */
    
    // Make sure ID doesn't contain illegal characters
    protected function validate_id($id) {
      return preg_match('/^[a-z0-9_]+$/', $id);
    }
  }
  
  /*
   * Example of manual list building:
   *
  function fake_members_mailing_list_descriptors_alter(&$data) {
    $juniors = array();
    $seniors = array();
    
    $juniors['title'] = 'Juniors';
    $juniors['description'] = 'Junior member mailing lists';
    $juniors['items'] = array(
      'id_candidates' => array(
        'title' => t('Candidates'),
        'description' => NULL,
      ),
      'id_volunteers' => array(
        'title' => t('Volunteers'),
        'description' => NULL,
      ),
    );
    
    $seniors['title'] = 'Seniors';
    $seniors['description'] = NULL;
    $seniors['items'] = array(
      'item_id_members' => array('title' => t('Members')),
      'item_id_directors' => array('title' => t('Directors')),
      'item_id_admins' => array('title' => t('Administrators')),
    );
    
    // insert our groups
    $data['xx_members']['storage_class_juniors'] = $juniors;
    $data['xx_members']['storage_class_seniors'] = $seniors;
  }
  */
   
  /*
   * Example of list-building using the helper class
   *
  function fake_contacts_mailing_list_descriptors_alter(&$data) {
   // Use helper class
   $builder = new DescriptorArrayBuilder();
   
   $class = $builder->add_storage_class('gid_contacts', 'Contacts');
   $builder->add_item_to_class($class, 'iid_fans', 'Fans');
   $builder->add_item_to_class($class, 'iid_friends', 'Friends');
   $builder->add_item_to_class($class, 'iid_family', 'Family');
   
   $class = $builder->add_storage_class('gid_inactive', 'Inactive Members');
   $builder->add_item_to_class($class, 'iid_former', 'Former Members');
   $builder->add_item_to_class($class, 'iid_retired', 'Retired Members');
   
   // Insert the descriptors under our module
   $data['xx_contacts'] = $builder->build(); 
  }
  */
 
  