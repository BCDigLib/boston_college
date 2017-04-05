<?php
/**
 * @file
 * Creates and executes a batch for updating DC for Boston's objects.
 */

/**
 * Update operation for the batch.
 *
 * @param array $context
 *   An array containing the context of the current batch.
 */
function boston_college_update_dc_identifier_batch_operation(&$context) {
  $sandbox = &$context['sandbox'];

  $query = <<<EOQ
SELECT ?obj FROM <#ri> WHERE {
  ?obj <fedora-view:disseminates> ?ds .
  ?ds <fedora-view:disseminationType> <info:fedora/*/DC> .
}
ORDER BY ?obj

EOQ;
  $tuque = islandora_get_tuque_connection();
  if (!isset($sandbox['total'])) {
    $sandbox['offset'] = 0;
    $sandbox['total'] = $tuque->repository->ri->countQuery($query, 'sparql');

    if ($sandbox['total'] == 0) {
      return;
    }
  }

  $offset = 10;
  $limited_query = $query . <<<EOQ
LIMIT $offset
OFFSET {$sandbox['offset']}
EOQ;
  $context['message'] = t('Processing results @start to @end.', array(
    '@start' => $sandbox['offset'],
    '@end' => min($sandbox['offset'] + $offset, $sandbox['total']),
  ));
  $results = $tuque->repository->ri->sparqlQuery($limited_query);
  $identifier_query = <<<EOQ
ASK {
  <info:fedora/!pid> <dc:identifier> '!pid'
}
EOQ;
  foreach ($results as $result) {
    // This is silly but our subset of SPARQL with Fedora makes it not possible
    // to do nicely in a query.
    $pid = $result['obj']['value'];
    $object_query = format_string($identifier_query, array('!pid' => $pid));
    $result = $tuque->repository->ri->sparqlQuery($object_query);
    $answer = reset($result);
    $existence = $answer['k0']['value'] == 'true';
    if (!$existence) {
      $object = islandora_object_load($pid);
      $dom = new DOMDocument();
      $dom->formatOutput = TRUE;
      $dom->preserveWhiteSpace = FALSE;
      $dom->loadXML($object['DC']->content);
      $element = $dom->createElementNS('http://purl.org/dc/elements/1.1/', 'dc:identifier', $pid);
      $dom->documentElement->appendChild($element);
      $object['DC']->content = $dom->saveXML();
      drush_log(dt('Updated the dc:identifier for @pid.', array('@pid' => $pid)));
    }
    else {
      drush_log(dt('@pid already has a dc:identifier of itself, skipping.', array('@pid' => $pid)));
    }
  }
  $sandbox['offset'] += $offset;
  $context['finished'] = $sandbox['offset'] / $sandbox['total'];
}

/**
 * Update operation for the batch.
 *
 * @param array $context
 *   An array containing the context of the current batch.
 */
function boston_college_update_dc_identifier_again_batch_operation(&$context) {
  $sandbox = &$context['sandbox'];

  $query = <<<EOQ
SELECT ?obj FROM <#ri> WHERE {
  ?obj <fedora-view:disseminates> ?ds .
  ?ds <fedora-view:disseminationType> <info:fedora/*/DC> .
}
ORDER BY ?obj

EOQ;
  $tuque = islandora_get_tuque_connection();
  if (!isset($sandbox['total'])) {
    $sandbox['offset'] = 0;
    $sandbox['total'] = $tuque->repository->ri->countQuery($query, 'sparql');

    if ($sandbox['total'] == 0) {
      return;
    }
  }

  $offset = 10;
  $limited_query = $query . <<<EOQ
LIMIT $offset
OFFSET {$sandbox['offset']}
EOQ;
  $context['message'] = t('Processing results @start to @end.', array(
    '@start' => $sandbox['offset'],
    '@end' => min($sandbox['offset'] + $offset, $sandbox['total']),
  ));
  $results = $tuque->repository->ri->sparqlQuery($limited_query);
  foreach ($results as $result) {
    $pid = $result['obj']['value'];
    $object = islandora_object_load($pid);
    $dom = new DOMDocument();
    $dom->formatOutput = TRUE;
    $dom->preserveWhiteSpace = FALSE;
    $dom->loadXML($object['DC']->content);
    $xpath = new DOMXPath($dom);
    $xpath->registerNamespace('dc', 'http://purl.org/dc/elements/1.1/');
    $results = $xpath->query('dc:identifier');
    foreach ($results as $result) {
      if ($result->lookupnamespaceURI(NULL)) {
        $parent = $result->parentNode;
        $parent->removeChild($result);
        $element = $dom->createElementNS('http://purl.org/dc/elements/1.1/', 'dc:identifier', $pid);
        $parent->appendChild($element);
      }
    }
    $object['DC']->content = $dom->saveXML();
    drush_log(dt('Updated the dc:identifier for @pid.', array('@pid' => $pid)));
  }
  $sandbox['offset'] += $offset;
  $context['finished'] = $sandbox['offset'] / $sandbox['total'];
}
