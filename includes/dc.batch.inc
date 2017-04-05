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
function boston_college_update_dc_batch_operation(&$context) {
  $sandbox = &$context['sandbox'];

  $query = <<<EOQ
SELECT DISTINCT ?obj ?lmd FROM <#ri> WHERE {
  ?obj <fedora-view:disseminates> ?ds ;
       <info:fedora/fedora-system:def/model#createdDate> ?lmd .
  ?ds <fedora-view:disseminationType> <info:fedora/*/MODS> .
!filter
}
ORDER BY ?lmd

EOQ;
  $tuque = islandora_get_tuque_connection();
  if (!isset($sandbox['total'])) {
    $sandbox['offset'] = 0;
    $sandbox['total'] = $tuque->repository->ri->countQuery(format_string($query, array(
      '!filter' => '',
    )), 'sparql');

    if ($sandbox['total'] == 0) {
      return;
    }
  }

  $offset = 10;
  $limited_query = format_string($query, array(
    '!filter' => (isset($sandbox['lmd']) ?
        "FILTER(?lmd > '{$sandbox['lmd']}'^^xsd:dateTime)" :
        ''),
    )) . <<<EOQ
LIMIT $offset
EOQ;
  $context['message'] = t('Processing results @start to @end.', array(
    '@start' => $sandbox['offset'],
    '@end' => min($sandbox['offset'] + $offset, $sandbox['total']),
  ));
  $results = $tuque->repository->ri->sparqlQuery($limited_query);
  foreach ($results as $result) {
    $object = islandora_object_load($result['obj']['value']);
    $doc = new DOMDocument();
    $doc->loadXML($object['MODS']->content);
    xml_form_builder_update_dc_datastream($object, 'mods_to_dc.xsl', $doc);
    drush_log(dt('Updated the Dublin Core for @pid.', array('@pid' => $object->id)));
    $sandbox['lmd'] = $result['lmd']['value'];
  }
  $sandbox['offset'] += $offset;
  $context['finished'] = $sandbox['offset'] / $sandbox['total'];
}
