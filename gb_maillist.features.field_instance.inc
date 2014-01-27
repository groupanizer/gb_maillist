<?php
/**
 * @file
 * gb_maillist.features.field_instance.inc
 */

/**
 * Implements hook_field_default_field_instances().
 */
function gb_maillist_field_default_field_instances() {
  $field_instances = array();

  // Exported field_instance: 'comment-comment_node_forum-field_attachment'
  $field_instances['comment-comment_node_forum-field_attachment'] = array(
    'bundle' => 'comment_node_forum',
    'deleted' => 0,
    'description' => '',
    'display' => array(
      'default' => array(
        'label' => 'above',
        'module' => 'file',
        'settings' => array(),
        'type' => 'file_default',
        'weight' => 1,
      ),
    ),
    'entity_type' => 'comment',
    'field_name' => 'field_attachment',
    'label' => 'Attachment',
    'required' => 0,
    'settings' => array(
      'description_field' => 0,
      'file_directory' => 'attachments',
      'file_extensions' => 'txt rtf pdf jpg jpeg png bmp doc docx ppt pptx',
      'filefield_paths' => array(
        'active_updating' => 0,
        'file_name' => array(
          'options' => array(
            'pathauto' => FALSE,
            'transliterate' => 0,
          ),
          'value' => '[file:ffp-name-only-original].[file:ffp-extension-original]',
        ),
        'file_path' => array(
          'options' => array(
            'pathauto' => FALSE,
            'transliterate' => 0,
          ),
          'value' => 'attachments/[comment:node:nid]',
        ),
        'retroactive_update' => 0,
      ),
      'filefield_paths_enabled' => 1,
      'max_filesize' => '9 MB',
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 1,
      'module' => 'file',
      'settings' => array(
        'progress_indicator' => 'bar',
      ),
      'type' => 'file_generic',
      'weight' => 2,
    ),
  );

  // Exported field_instance: 'node-forum-field_attachment'
  $field_instances['node-forum-field_attachment'] = array(
    'bundle' => 'forum',
    'deleted' => 0,
    'description' => '',
    'display' => array(
      'default' => array(
        'label' => 'above',
        'module' => 'file',
        'settings' => array(),
        'type' => 'file_default',
        'weight' => 12,
      ),
      'teaser' => array(
        'label' => 'above',
        'settings' => array(),
        'type' => 'hidden',
        'weight' => 0,
      ),
    ),
    'entity_type' => 'node',
    'field_name' => 'field_attachment',
    'label' => 'Attachment',
    'required' => 0,
    'settings' => array(
      'description_field' => 0,
      'file_directory' => 'attachments',
      'file_extensions' => 'txt rtf pdf jpg jpeg png bmp doc docx ppt pptx',
      'filefield_paths' => array(
        'active_updating' => 0,
        'file_name' => array(
          'options' => array(
            'pathauto' => FALSE,
            'transliterate' => 0,
          ),
          'value' => '[file:ffp-name-only-original].[file:ffp-extension-original]',
        ),
        'file_path' => array(
          'options' => array(
            'pathauto' => FALSE,
            'transliterate' => 0,
          ),
          'value' => 'attachments/[node:nid]',
        ),
        'retroactive_update' => 0,
      ),
      'filefield_paths_enabled' => 1,
      'max_filesize' => '9 MB',
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 1,
      'module' => 'file',
      'settings' => array(
        'progress_indicator' => 'bar',
      ),
      'type' => 'file_generic',
      'weight' => 33,
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('Attachment');

  return $field_instances;
}
