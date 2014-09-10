<?php

/**
 * @file
 * Drush hooks for the module.
 */

/**
 * Implements hook_drush_command().
 */
function boston_college_drush_command() {
  return array(
    'boston_college_batch_process' => array(
      'description' => t('Prepare a directory for ingest.'),
      'drupal dependencies' => array('boston_college'),
      'options' => array(
        'target' => array(
          'description' => t('The directory to prepare.'),
          'required' => TRUE,
        ),
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
      'examples' => array('drush -v boston_college_batch_process --target=/home/user/site_data'),
    ),
  );
}

/**
 * Run the directory processing.
 */
function drush_boston_college_batch_process() {
  $preprocessor = new BostonCollegeBatchPreprocessor(
    islandora_get_tuque_connection(),
    array(
      'type' => 'directory',
      'target' => drush_get_option('target'),
      'parent' => variable_get('islandora_repository_pid', 'islandora:root'),
      'parent_relationship_uri' => FEDORA_RELS_EXT_URI,
      'parent_relationship_pred' => 'isMemberOfCollection',
      'namespace' => 'bc-ir',
    )
  );
  $preprocessed = islandora_batch_handle_preprocessor($preprocessor);
}
