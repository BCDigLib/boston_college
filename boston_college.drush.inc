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
    'boston_college_update_handles' => array(
      'description' => dt('Update existing Handles to point at new Islandora object URLs. Subsquent runs of this command should only go back and update Handles that do not return a success code from the initial run.'),
      'drupal dependencies' => array(
        'islandora',
        'islandora_handle',
        'boston_college',
      ),
      'examples' => array(
        'drush -u 1 boston_college_update_handles' => dt('Updating existing Handles.'),
      ),
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
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

/**
 * Update Handles to point at new URLs.
 */
function drush_boston_college_update_handles() {
  batch_set(boston_college_update_handles_create_batch());
  drush_backend_batch_process();
}

/**
 * Constructs a batch used to update things via Drush.
 */
function boston_college_update_handles_create_batch() {
  return array(
    'operations' => array(
      array('boston_college_update_handles_batch_operation', array()),
    ),
    'title' => t('Updating Handles for objects...'),
    'init_message' => t('Preparing to update Handles.'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaning @estimate.'),
    'error_message' => t('An error has occurred.'),
    'file' => drupal_get_path('module', 'boston_college') . '/includes/handle.batch.inc',
  );
}
