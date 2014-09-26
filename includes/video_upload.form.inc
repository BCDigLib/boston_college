<?php

/**
 * @file
 * Handles the uploading of the video file.
 */

/**
 * Defines a file upload form for uploading the video.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 *
 * @return array
 *   The Drupal form definition.
 */
function boston_college_video_upload_form(array $form, array &$form_state) {
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('mp4');
  return array(
    'file' => array(
      '#title' => t('Video'),
      '#type' => 'managed_file',
      '#required' => FALSE,
      '#description' => t('Select video to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $extensions[0])),
      '#default_value' => isset($form_state['values']['file']) ? $form_state['values']['file'] : NULL,
      '#upload_location' => 'temporary://',
      '#upload_validators' => array(
        'file_validate_extensions' => $extensions,
         // Assume its specified in MB.
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );
}

/**
 * Submit handler, adds uploaded file to the video object.
 *
 * Can't have #submit entries on forms in ingest steps.
 *
 * @param array $form
 *   The Drupal form.
 * @param array $form_state
 *   The Drupal form state.
 */
function boston_college_video_upload_form_submit(array $form, array &$form_state) {
  form_load_include($form_state, 'inc', 'islandora_video', 'includes/video_upload.form');
  if ($form_state['values']['file']) {
    return islandora_video_upload_form_submit($form, $form_state);
  }
}
