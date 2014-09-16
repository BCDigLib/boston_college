<?php
/**
 * @file
 * Migration batching code for Boston using the Batch interface.
 */

class BostonCollegeBatchPreprocessor extends IslandoraScanBatch {
  /**
   * Get the associated object class.
   *
   * @see IslandoraScanBatch:getObjectClass()
   */
  protected static function getObjectClass() {
    return 'BostonCollegeBatchObject';
  }

  /**
   * Refine files into a form we expect.
   */
  protected function groupFiles($files) {
    $allowed_usages = array(
      'VIEW_MAIN' => 'VIEW_MAIN',
    );
    // Firstly let's make a reference array.
    $ref = array();
    foreach ($files as $uri => $file_object) {
      $ref[$file_object->filename] = $file_object->uri;
    }
    $grouped = array();
    // Only process XML files.
    foreach ($files as $uri => &$file_object) {
      $extension = pathinfo($file_object->filename, PATHINFO_EXTENSION);
      if ($extension == 'xml') {
        $xml = simplexml_load_file($file_object->uri);
        $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
        // Only want files with descriptive metadata as our base objects.
        $usage_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/usage_type');
        if ($usage_xpath) {
          $usage = (string) reset($usage_xpath);
          if (isset($allowed_usages[$usage])) {
            $file_object->ext = $extension;
            $grouped[$file_object->name][$file_object->ext] = $file_object;
            $entity_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/entity_type');
            $entity_type = FALSE;
            if ($entity_xpath) {
              $entity_type = (string) reset($entity_xpath);
            }
            // Let's build up a nice helper array now for re-use.
            $relations = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/relations/relation');
            foreach ($relations as $relation) {
              $pid_xpath = $relation->xpath('pid');
              $pid = FALSE;
              $related_file = FALSE;
              if ($pid_xpath) {
                $pid = (string) reset($pid_xpath);
              }
              else {
                // If this fails run away wildly.
                drush_log(t('Unable to determine relation pid for @file, skipping.', array('@file' => $file_object->uri)));
                break;
              }
              $relation_entity_xpath = $relation->xpath('entity_type');
              $relation_type_xpath = $relation->xpath('usage_type');
              if ($relation_entity_xpath && $relation_type_xpath) {
                $rel_usage_type = (string) reset($relation_type_xpath);
                $rel_entity_type = (string) reset($relation_entity_xpath);
                // Only care about related items that are documents that are
                // archives.
                if ($rel_entity_type == 'DOCUMENT' && $rel_usage_type == 'ARCHIVE') {
                  // Need to retrieve the filename from the linked xml.
                  if (isset($ref["$pid.xml"])) {
                    $linked_file = simplexml_load_file($ref["$pid.xml"]);
                    $linked_file->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
                    $file_xpath = $linked_file->xpath('/xb:digital_entity_result/xb:digital_entity/stream_ref/file_name');
                    if ($file_xpath) {
                      $file = (string) reset($file_xpath);
                      // Need to do a kind of dumb find and replace to account
                      // for a case where there is an apostrophe in the file.
                      $file = str_replace("'", '', $file);
                      if (isset($ref[$file])) {
                        $related_file = $file;
                      }
                      else {
                        drush_log(t('The related file @related is not present for @file.', array(
                          '@related' => $file,
                          '@file' => $file_object->uri,
                        )));
                      }
                    }
                  }
                  else {
                    drush_log(t('Unable to find linked document for @file.', array('@file' => $file_object->uri)));
                  }
                }
              }
              if ($pid && $related_file) {
                if (isset($ref[$related_file])) {
                  if (!isset($file_object->linked['archive'])) {
                    $file_object->linked['archive'] = array();
                  }
                  $file_object->linked['archive'][] = $ref[$related_file];
                }
              }
              // For videos we need retrieve data from a linked file, there will
              // only be one of these.
              if ($entity_type == 'VIDEO_STREAM') {
                $linked_xml = simplexml_load_file($ref["$pid.xml"]);
                $linked_xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
                $vid_xpath = $linked_xml->xpath('/xb:digital_entity_result/xb:digital_entity/stream_ref[mime_type/text() = "video/mp4"]/file_name');
                if ($vid_xpath) {
                  $vid_file = (string) reset($vid_xpath);
                  if (isset($ref[$vid_file])) {
                    $file_object->linked['video'] = $ref[$vid_file];
                  }
                }
                else {
                  drush_log(t('Unable to match the linked file @linked with a scanned file for @file', array(
                    '@linked' => $pid,
                    '@file' => $uri,
                  )));
                }
              }
            }
          }
        }
      }
    }
    return $grouped;
  }
}

class BostonCollegeBatchObject extends IslandoraScanBatchObject {
  protected $sourceXML;
  protected $type;
  protected $pdfURI;
  protected static $typeMappings;
  /**
   * Constructs an object for Boston College.
   *
   * @see IslandoraScanBatch::__construct
   */
  public function __construct(IslandoraTuque $connection, $base_name, $object_info, $preprocessor_parameters) {
    parent::__construct($connection, $base_name, $object_info, $preprocessor_parameters);
    if (!static::$typeMappings) {
      static::$typeMappings = array(
        'dc' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:honorsThesesCollection',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/DC_MODS3-4_XSLT1-0.xsl',
        ),
        'etd-ms' => array(
          'content_model' => 'ir:graduateETDCModel',
          'collection' => 'ir:GraduateThesesCollection',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/ETDMS_MODS3-4_XSLT1-0.xsl',
        ),
        'modstext' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:honorsThesesCollection',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
        'modsvideo' => array(
          'content_model' => BOSTON_COLLEGE_VIDEO_CONTENT_MODEL,
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
      );
    }
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $type = $this->determineType($xml);
    $this->type = $type;
    if ($type) {
      if (isset(static::$typeMappings[$type])) {
        $this->preprocessorParameters['content_models'] = array(static::$typeMappings[$type]['content_model']);
      }
    }
    else {
      drush_log(t('Unable to determine the type for @file.', array('@file' => $this->objectInfo['xml']->uri)));
    }
    $archival_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/stream_ref[file_extension/text() = "pdf"]/file_name');
    if ($archival_xpath) {
      $pdf_name = (string) reset($archival_xpath);
      $this->pdfURI = $this->preprocessorParameters['target'] . '/' . $pdf_name;
    }
    else {
      if ($type != 'modsvideo') {
        drush_log(t('Unable to match a PDF for @file', array('@file' => $this->objectInfo['xml']->uri)));
      }
    }
  }

  /**
   * Wakey, wakey.
   */
  public function __wakeup() {
    parent::__wakeup();
    if (!static::$typeMappings) {
      static::$typeMappings = array(
        'dc' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/DC_MODS3-4_XSLT1-0.xsl',
        ),
        'etd-ms' => array(
          'content_model' => 'ir:graduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/ETDMS_MODS3-4_XSLT1-0.xsl',
        ),
        'modstext' => array(
          'content_model' => 'ir:undergraduateETDCModel',
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
        'modsvideo' => array(
          'content_model' => BOSTON_COLLEGE_VIDEO_CONTENT_MODEL,
          'collection' => 'ir:citation',
          'transform' => drupal_get_path('module', 'boston_college') . '/data/transforms/MODS-digitool_MODS3-4-islandora_XSLT1-0.xsl',
        ),
      );
    }
  }

  /**
   * Process the batch.
   */
  public function batchProcess() {
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $this->sourceXML = $xml;
    $this->getArchivalPDF();
    if ($this->type == 'dc') {
      $this->getEventHistory();
    }
    if ($this->type == 'etd-ms') {
      $this->getArchivalRelease();
    }
    if ($this->type == 'modsvideo') {
      $this->getArchivalVideo();
    }
    return parent::batchProcess();
  }

  /**
   * Determines the content model based upon the source XML from DigiTools.
   *
   * @param SimpleXMLElement $xml
   *   A SimpleXMLElement containing our loaded source xml.
   *
   * @return bool|string
   *   FALSE if the XPath fails, the content model string otherwise.
   */
  public function determineType($xml) {
    $type_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[name/text() = "descriptive"]/type');
    if ($type_xpath) {
      $type = (string) reset($type_xpath);
      if ($type == 'mods') {
        $entity_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/entity_type/text()');
        if ($entity_xpath) {
          $entity = (string) reset($entity_xpath);
          if ($entity == 'VIDEO_STREAM') {
            $type = 'modsvideo';
          }
          else {
            $type = 'modstext';
          }
        }
      }
      return $type;
    }
    return FALSE;
  }

  /**
   * Go out and create the MODS.
   */
  protected function getMods() {
    if (!isset($this['MODS'])) {
      // Get the data to be transformed.
      $source_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[name/text() = "descriptive"]/value');
      if ($source_xpath) {
        // Take the source XML and transform it using the supplied XSLs.
        $source = (string) reset($source_xpath);
        $mods_xml = static::runXslTransform(array(
          'input' => $source,
          'xsl' => static::$typeMappings[$this->type]['transform'],
        ));
        $mods_doc = new DOMDocument();
        $mods_doc->loadXML($mods_xml);
        $mods_to_add = array();

        $digi_pid = FALSE;
        // Add the digiTool PID as an identifier.
        $digi_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/pid/text()');
        if ($digi_xpath) {
          $digi_pid = (string) reset($digi_xpath);
          $mods_to_add[] = array(
            'field' => 'mods:identifier',
            'attribute' => array(
              'name' => 'type',
              'value' => 'digitool',
            ),
            'value' => $digi_pid,
          );
        }
        // Go get the Handle value from PREMIS if it exists and append.
        $premis_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[type/text() = "preservation_md"]/value');
        if ($premis_xpath) {
          $premis = (string) reset($premis_xpath);
          $premis_xml = simplexml_load_string($premis);
          $premis_xml->registerXPathNamespace('premis', 'http://www.loc.gov/standards/premis');
          $handle_result = $premis_xml->xpath('/premis:object/premis:objectIdentifier[premis:objectIdentifierType/text() = "handle"]/premis:objectIdentifierValue/text()');
          if ($handle_result) {
            $handle = (string) reset($handle_result);
            $handle_url = "http://hdl.handle.net/$handle";
            $mods_to_add[] = array(
              'field' => 'mods:identifier',
              'attribute' => array(
                'name' => 'type',
                'value' => 'hdl',
              ),
              'value' => $handle_url,
            );
            // Go out and resolve the old URL for safety's sake.
            $request = drupal_http_request($handle_url);
            $old_url = FALSE;
            if (isset($request->redirect_url)) {
              $old_url = $request->redirect_url;
            }
            // Add the Handle to be updated at a later time.
            db_insert('boston_college_handles')
              ->fields(array(
                'handle' => $handle,
                'islandora_pid' => $this->id,
                'digitool_pid' => $digi_pid ? $digi_pid : NULL,
                'old_url' => $old_url ? $old_url : NULL,
              ))
              ->execute();
          }
        }
        foreach ($mods_to_add as $values) {
          $element = $mods_doc->createElement($values['field']);
          if (isset($values['attribute'])) {
            $attribute = $mods_doc->createAttribute($values['attribute']['name']);
            $attribute->value = $values['attribute']['value'];
            $element->appendChild($attribute);
          }
          $text = $mods_doc->createTextNode($values['value']);
          $element->appendChild($text);
          $mods_doc->documentElement->appendChild($element);
        }
        $mods_xml = $mods_doc->saveXML();
        $mods_datastream = $this->constructDatastream('MODS', 'M');
        $mods_datastream->mimetype = 'text/xml';
        $mods_datastream->label = 'MODS Record';
        $mods_datastream->content = $mods_xml;
        $this->ingestDatastream($mods_datastream);
        return $this['MODS']->content;
      }
      else {
        drush_log(t('Unable to retrieve source metadata for @file.', array('@file' => $this->objectInfo['xml']->uri)));
      }
    }
    else {
      return $this['MODS']->content;
    }
  }

  /**
   * Gets the archival PDF to be added.
   */
  protected function getArchivalPDF() {
    if ($this->pdfURI) {
      $pdf_datastream = $this->constructDatastream(BOSTON_COLLEGE_ARCHIVAL_PDF_DATASTREAM_ID, 'M');
      $pdf_datastream->mimetype = 'application/pdf';
      $pdf_datastream->label = 'PDF Datastream';
      $pdf_datastream->setContentFromFile($this->pdfURI);
      $this->ingestDatastream($pdf_datastream);
    }
  }

  /**
   * Creates the event history datastream for graduate objects.
   */
  protected function getEventHistory() {
    $event_xpath = $this->sourceXML->xpath('/xb:digital_entity_result/xb:digital_entity/mds/md[type/text() = "changehistory_md"]/value');
    if ($event_xpath) {
      $history = (string) reset($event_xpath);
      $history = simplexml_load_string($history);
      $history->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
      $deposit_xpath = $history->xpath('/xb:history/events/event[eventIdentifier/eventIdentifierType/text() = "DEPOSIT ID"]');
      if ($deposit_xpath) {
        $deposit = reset($deposit_xpath);
        $archive_datastream = $this->constructDatastream('ARCHIVE', 'M');
        $archive_datastream->mimetype = 'application/xml';
        $archive_datastream->label = 'Event History';
        $archive_datastream->content = $deposit->saveXML();
        $this->ingestDatastream($archive_datastream);
      }
    }
  }

  /**
   * Creates the archival video datastream for video objects.
   */
  protected function getArchivalVideo() {
    if (isset($this->objectInfo['xml']->linked['video'])) {
      $vid_datastream = $this->constructDatastream('OBJ', 'M');
      $vid_datastream->mimetype = 'video/mp4';
      $vid_datastream->label = basename($this->objectInfo['xml']->linked['video']);
      $vid_datastream->setContentFromFile($this->objectInfo['xml']->linked['video']);
      $this->ingestDatastream($vid_datastream);
    }
  }

  /**
   * Creates the archival datastream for release conditions on documents.
   */
  protected function getArchivalRelease() {
    if (isset($this->objectInfo['xml']->linked['archive'])) {
      $first = TRUE;
      $count = 1;
      foreach ($this->objectInfo['xml']->linked['archive'] as $archive) {
        if ($first) {
          $ds_name = "ARCHIVE";
          $first = FALSE;
        }
        else {
          $count++;
          $ds_name = "ARCHIVE_$count";
        }
        $archival_datastream = $this->constructDatastream($ds_name, 'M');
        $archival_datastream->mimetype = file_get_mimetype($archive);
        $archival_datastream->label = basename($archive);
        $archival_datastream->setContentFromFile($archive);
        $this->ingestDatastream($archival_datastream);
      }
    }
  }

  /**
   * Add relationships to the object.
   *
   * @see IslandoraScanBatch::addRelationships()
   */
  public function addRelationships() {
    parent::addRelationships();
    // Grab the digiTools creation date to slam into the RELS-EXT.
    $xml = simplexml_load_file($this->objectInfo['xml']->uri);
    $xml->registerXPathNamespace('xb', 'http://com/exlibris/digitool/repository/api/xmlbeans');
    $creation_xpath = $xml->xpath('/xb:digital_entity_result/xb:digital_entity/control/creation_date/text()');
    if ($creation_xpath) {
      $date = (string) reset($creation_xpath);
      $formatted_date = gmdate("Y-m-d\TH:i:s\Z", strtotime($date));
      $this->relationships->add(ISLANDORA_RELS_EXT_URI, 'digiToolCreationDate', $formatted_date, RELS_TYPE_DATETIME);
    }
    else {
      drush_log(t('Unable to add creation date to the RELS-EXT for @file', array('@file' => $this->objectInfo['xml']->uri)));
    }
  }

  /**
   * No resources needed.
   *
   * @return array
   *   The empty array of no resources.
   */
  public function getResources() {
    return array();
  }
}
