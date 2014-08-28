<?php

/**
 * @file
 * Derivative callbacks and associated functions used in building splash page.
 */

function boston_college_build_pdf($object, $force = FALSE, $hook) {
  if (!$force && isset($object['PDF'])) {
    return;
  }

  $to_return = array(
    'success' => TRUE,
    'messages' => array(),
  );

  // Build splash page PDF.
  $splash_page = boston_college_run_fop($object['MODS']->content);
  if (!$splash_page) {
    $to_return['success'] = FALSE;
    $to_return['messages'][] = array(
      'message' => t('Failed to generate PDF splash page for @id; unable to generate PDF datastream.'),
      'message_sub' => array(
        '@id' => $object->id,
      ),
      'type' => 'dsm',
      'severity' => 'error',
    );
    return $to_return;
  }

  // Create copy w/ splash & ARCHIVE-PDF concatted
  $archive_pdf_uri = file_create_filename('archive-pdf.pdf', 'temporary://');
  touch(drupal_realpath($archive_pdf_uri));
  $object[BOSTON_COLLEGE_ARCHIVAL_PDF_DATASTREAM_ID]->getContent($archive_pdf_uri);
  module_load_include('inc', 'islandora', 'includes/utilities');
  $archive_pdf = islandora_temp_file_entry($archive_pdf_uri, 'application/pdf');

  $concatted_pdf = boston_college_concat_pdfs(array(
    $splash_page->uri,
    $archive_pdf->uri,
  ));

  file_delete($splash_page);
  file_delete($archive_pdf);

  if (!$concatted_pdf) {
    $to_return['success'] = FALSE;
    $to_return['messages'][] = array(
      'message' => t('Failed to concatenate generated splash page and "@archival" datastream for @id; unable to generate PDF datastream.'),
      'message_sub' => array(
        '@id' => $object->id,
        '@archival' => BOSTON_COLLEGE_ARCHIVAL_PDF_DATASTREAM_ID,
      ),
      'type' => 'dsm',
      'severity' => 'error',
    );
    return $to_return;
  }

  // Upload concatted copy as PDF.
  $datastream = isset($object['PDF']) ?
    $object['PDF'] :
    $object->constructDatastream('PDF', 'M');

  $datastream->label = 'PDF datastream';
  $datastream->mimetype = $concatted_pdf->filemime;
  $datastream->setContentFromFile($concatted_pdf->uri);

  if (!isset($object['PDF'])) {
    $object->ingestDatastream($datastream);
  }
  file_delete($concatted_pdf);

  $to_return['messages'][] = array(
    'message' => t('Generated, concatenated and ingested PDF for @id.'),
    'message_sub' => array(
      '@id' => $object->id,
    ),
    'type' => 'dsm',
    'severity' => 'info',
  );
  return $to_return;
}

/**
 * Run Apache FOP to transform the input MODS.
 *
 * @param string $input_mods
 *   A string containing MODS XML.
 *
 * @return object|bool
 *   Either a Drupal file object on success; otherwise, boolean FALSE on
 *   failure.
 */
function boston_college_run_fop($input_mods) {
  $output = file_create_filename('splash.pdf', 'temporary://');
  touch(drupal_realpath($output));
  $args = array(
    variable_get('boston_college_fop_executable', 'fop'),
    '-xml',
    '-',
    '-xsl',
    variable_get('boston_college_splash_xsl', '/opt/BC-Islandora-Implementation/SplashPageStylesheets/splash.xsl'),
    '-pdf',
    drupal_realpath($output),
  );
  $escaped_args = array_map('escapeshellarg', $args);
  $descriptors = array(
    0 => array('pipe', 'r'),
    1 => array('pipe', 'w'),
    2 => array('pipe', 'w'),
  );
  $pipes = array();
  $proc = proc_open(implode(' ', $escaped_args), $descriptors, $pipes);

  // Not sure we need stdout & err... Let's close 'em to get 'em out of the way.
  fclose($pipes[1]);
  fclose($pipes[2]);
  fwrite($pipes[0], $input_mods);
  fclose($pipes[0]);

  $status = proc_get_status($proc);
  while ($status['running'] === TRUE) {
    $status = proc_get_status($proc);
  }
  proc_close($proc);

  if ($status['exitcode'] !== 0) {
    unlink($output);
    return FALSE;
  }
  else {
    module_load_include('inc', 'islandora', 'includes/utilities');
    return islandora_temp_file_entry($output, 'application/pdf');
  }
}

function boston_college_concat_pdfs(array $input_pdfs) {
  if (empty($input_pdfs)) {
    // Nothing to join, you silly.
    return FALSE;
  }

  $pdfs = array_map('drupal_realpath', $input_pdfs);

  $output = file_create_filename('concatted.pdf', 'temporary://');
  touch(drupal_realpath($output));
  $args = array(
    variable_get('boston_college_pdftk_executable', 'pdftk'),
  );
  $args = array_merge($args, $pdfs);
  $args = array_merge($args, array(
    'cat',
    'output',
    drupal_realpath($output),
  ));
  $escaped_args = array_map('escapeshellarg', $args);
  $descriptors = array(
    0 => array('pipe', 'r'),
    1 => array('pipe', 'w'),
    2 => array('pipe', 'w'),
  );
  $pipes = array();
  $proc = proc_open(implode(' ', $escaped_args), $descriptors, $pipes);

  // Close ALL the pipes! :V
  fclose($pipes[0]);
  fclose($pipes[1]);
  fclose($pipes[2]);

  $status = proc_get_status($proc);
  while ($status['running'] === TRUE) {
    $status = proc_get_status($proc);
  }
  proc_close($proc);

  if ($status['exitcode'] !== 0) {
    unlink($output);
    return FALSE;
  }
  else {
    module_load_include('inc', 'islandora', 'includes/utilities');
    return islandora_temp_file_entry($output, 'application/pdf');
  }
}
