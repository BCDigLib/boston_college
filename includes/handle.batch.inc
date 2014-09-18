<?php
/**
 * @file
 * Batch for updating all Handles present in the migration.
 */

/**
 * Constructs and performs the citation batch operation.
 *
 * @param string $base_url
 *   The base url to be used when constructing Handle URLs.
 * @param array $context
 *   The context of the Drupal batch.
 */
function boston_college_update_handles_batch_operation($base_url, &$context) {
  module_load_include('inc', 'islandora_handle', 'includes/handle');

  $sandbox = &$context['sandbox'];
  if (!isset($sandbox['last_pid'])) {
    $last_pid = FALSE;
  }
  else {
    $last_pid = $sandbox['last_pid'];
  }
  $context['message'] = t('Processing results...');
  $op_results = db_select('boston_college_handles', 'u')
    ->fields('u', array('handle', 'islandora_pid'))
    ->condition('updated', 0, '=')
    ->range(0, 10)
    ->orderBy('islandora_pid');
  if ($last_pid) {
    $op_results->condition('islandora_pid', $last_pid, '>');
  }
  $op_results = $op_results->execute();
  foreach ($op_results as $result) {
    $handle = $result->handle;
    $target = url("islandora/object/{$result->islandora_pid}", array(
      'language' => (object) array('language' => FALSE),
      'absolute' => TRUE,
      'base_url' => $base_url,
    ));
    $response = islandora_handle_update_handle_url($handle, $target);
    if ($response->code == 201 || $response->code == 204) {
      // Success we updated the Handle so let's update the value in the db
      // and log accordingly.
      if ($response->code == 201) {
        drush_log(dt('Successfully created a new Handle (@handle) for @pid.',
          array(
            '@handle' => $handle,
            '@pid' => $result->islandora_pid,
          )
        ), 'success');
      }
      else {
        drush_log(dt('Successfully updated the Handle (@handle) for @pid.',
          array(
            '@handle' => $handle,
            '@pid' => $result->islandora_pid,
          )
        ), 'success');
      }
      db_update('boston_college_handles')
        ->fields(array(
          'updated' => 1,
        ))
        ->condition('handle', $result->handle, '=')
        ->execute();
    }
    else {
      // Oh no! Something broke, we should log it.
      drush_log(dt('Unable to update the Handle (@handle) for @pid. Response code: @code. Error: @error',
        array(
          '@handle' => $handle,
          '@pid' => $result->pid,
          '@ds' => $result->ds,
          '@code' => $response->code,
          '@error' => $response->error,
        ),
        'error'));
    }
    $last_pid = $result->islandora_pid;
  }
  $sandbox['last_pid'] = $last_pid;
  $context['finished'] = $op_results->rowCount() ? 0 : 1;
}
